#include "puddin/core/OfficialRules.h"
#include <algorithm>
#include <sstream>

namespace puddin::core {

OfficialRulesArbitrator::OfficialRulesArbitrator(GameVariant variant) {
    resetRack(variant);
}

void OfficialRulesArbitrator::resetRack(GameVariant variant) {
    variant_ = variant;
    currentPlayer_ = PlayerId::Player1;
    matchOutcome_ = MatchOutcome::InProgress;
    p1Group_ = BallGroup::Unassigned;
    p2Group_ = BallGroup::Unassigned;
    isOpenTable_ = true;
    p1ConsecutiveFouls_ = 0;
    p2ConsecutiveFouls_ = 0;
    isFirstShotAfterBreak_ = false;
    pushOutAvailable_ = false;
}

BallGroup OfficialRulesArbitrator::getPlayerGroup(PlayerId player) const {
    return (player == PlayerId::Player1) ? p1Group_ : p2Group_;
}

int OfficialRulesArbitrator::getFoulCount(PlayerId player) const {
    return (player == PlayerId::Player1) ? p1ConsecutiveFouls_ : p2ConsecutiveFouls_;
}

PlayerId OfficialRulesArbitrator::getOpponent(PlayerId player) const {
    return (player == PlayerId::Player1) ? PlayerId::Player2 : PlayerId::Player1;
}

int OfficialRulesArbitrator::getLowestBall(const std::vector<int>& unsunkBalls) const {
    int lowest = 999;
    for (int id : unsunkBalls) {
        if (id > 0 && id < lowest) {
            lowest = id;
        }
    }
    return (lowest == 999) ? -1 : lowest;
}

void OfficialRulesArbitrator::resolvePushOut(bool acceptShot) {
    if (matchOutcome_ != MatchOutcome::PushOutDecisionPending) return;

    if (acceptShot) {
        currentPlayer_ = getOpponent(currentPlayer_);
    }
    matchOutcome_ = MatchOutcome::InProgress;
    pushOutAvailable_ = false;
    isFirstShotAfterBreak_ = false;
}

ShotOutcome OfficialRulesArbitrator::evaluateShot(const ShotInput& shot, const std::vector<int>& unsunkBalls) {
    ShotOutcome out{};
    out.coachPuddinWarning = false;
    out.eightOrNineRespotted = false;

    bool eightPocketed = std::find(shot.ballsPocketed.begin(), shot.ballsPocketed.end(), 8) != shot.ballsPocketed.end();
    bool ninePocketed = std::find(shot.ballsPocketed.begin(), shot.ballsPocketed.end(), 9) != shot.ballsPocketed.end();

    // ==========================================
    // 1. 9-BALL RULES ARBITRATION
    // ==========================================
    if (variant_ == GameVariant::NineBall) {
        // Push-Out Handler
        if (pushOutAvailable_ && shot.isPushOutCall) {
            pushOutAvailable_ = false;
            isFirstShotAfterBreak_ = false;

            if (shot.cueScratch) {
                out.isLegal = false;
                out.foulReason = FoulReason::CueBallScratch;
                out.ballInHand = true;
                out.nextShooter = getOpponent(shot.shooter);
                currentPlayer_ = out.nextShooter;
                if (shot.shooter == PlayerId::Player1) p1ConsecutiveFouls_++;
                else p2ConsecutiveFouls_++;

                if (ninePocketed) out.eightOrNineRespotted = true;
                out.coachPuddinSpeech = "Coach Puddin: 'Scratch on push-out! Ball-in-hand to your opponent.'";
                return out;
            }

            if (ninePocketed) out.eightOrNineRespotted = true;
            out.isLegal = true;
            out.foulReason = FoulReason::None;
            out.matchOutcome = MatchOutcome::PushOutDecisionPending;
            matchOutcome_ = MatchOutcome::PushOutDecisionPending;
            out.nextShooter = getOpponent(shot.shooter);
            out.ballInHand = false;
            out.coachPuddinSpeech = "Coach Puddin: 'Push-out played! Opponent can accept or pass back.'";
            return out;
        }

        pushOutAvailable_ = false;
        int lowest = getLowestBall(unsunkBalls);

        FoulReason foul = FoulReason::None;
        if (shot.cueScratch) {
            foul = FoulReason::CueBallScratch;
        } else if (shot.firstContactBallId != lowest) {
            foul = FoulReason::WrongBallFirstContact;
        } else if (shot.ballsPocketed.empty() && !shot.railHitAfterContact) {
            foul = FoulReason::NoRailAfterContact;
        }

        if (foul != FoulReason::None) {
            out.isLegal = false;
            out.foulReason = foul;
            out.ballInHand = true;
            out.nextShooter = getOpponent(shot.shooter);
            currentPlayer_ = out.nextShooter;
            if (ninePocketed) out.eightOrNineRespotted = true;

            int& fCount = (shot.shooter == PlayerId::Player1) ? p1ConsecutiveFouls_ : p2ConsecutiveFouls_;
            fCount++;

            if (fCount >= 3) {
                out.foulReason = FoulReason::ThreeConsecutiveFouls;
                out.matchOutcome = (shot.shooter == PlayerId::Player1) ? MatchOutcome::Player2Win : MatchOutcome::Player1Win;
                matchOutcome_ = out.matchOutcome;
                out.coachPuddinSpeech = "Coach Puddin: 'Three consecutive fouls! Rack forfeiture according to official WPA rules.'";
            } else if (fCount == 2) {
                out.coachPuddinWarning = true;
                out.coachPuddinSpeech = "Coach Puddin: 'Careful, son! That is TWO consecutive fouls! One more forfeits the rack.'";
            } else {
                out.coachPuddinSpeech = "Coach Puddin: 'Foul! Ball-in-hand anywhere on the felt.'";
            }
        } else {
            // Legal 9-Ball Shot
            out.isLegal = true;
            out.foulReason = FoulReason::None;
            if (shot.shooter == PlayerId::Player1) p1ConsecutiveFouls_ = 0;
            else p2ConsecutiveFouls_ = 0;

            if (ninePocketed) {
                out.matchOutcome = (shot.shooter == PlayerId::Player1) ? MatchOutcome::Player1Win : MatchOutcome::Player2Win;
                matchOutcome_ = out.matchOutcome;
                out.coachPuddinSpeech = shot.isBreakShot 
                    ? "Coach Puddin: 'GOLDEN BREAK! 9-ball dropped on the break for the WIN!'" 
                    : "Coach Puddin: 'Masterclass combo! 9-ball pocketed legally for the WIN!'";
                out.nextShooter = shot.shooter;
                return out;
            }

            if (shot.isBreakShot) {
                isFirstShotAfterBreak_ = true;
                pushOutAvailable_ = true;
            }

            if (!shot.ballsPocketed.empty()) {
                out.nextShooter = shot.shooter;
                currentPlayer_ = shot.shooter;
                out.coachPuddinSpeech = "Coach Puddin: 'Good shot. Keep your focus on that cue ball.'";
            } else {
                out.nextShooter = getOpponent(shot.shooter);
                currentPlayer_ = out.nextShooter;
                out.coachPuddinSpeech = "Coach Puddin: 'Solid tactical safety. Table passes to opponent.'";
            }
        }
    }
    // ==========================================
    // 2. 8-BALL RULES ARBITRATION
    // ==========================================
    else {
        BallGroup shooterGroup = (shot.shooter == PlayerId::Player1) ? p1Group_ : p2Group_;
        
        // Check if shooter has cleared all their group balls
        bool groupCleared = true;
        if (shooterGroup != BallGroup::Unassigned) {
            for (int id : unsunkBalls) {
                if (shooterGroup == BallGroup::Solids && isSolid(id)) { groupCleared = false; break; }
                if (shooterGroup == BallGroup::Stripes && isStripe(id)) { groupCleared = false; break; }
            }
        } else {
            groupCleared = false;
        }

        FoulReason foul = FoulReason::None;

        if (shot.cueScratch) {
            foul = FoulReason::CueBallScratch;
        } else if (shot.firstContactBallId == -1) {
            foul = FoulReason::WrongBallFirstContact;
        } else if (!isOpenTable_) {
            // First contact must match shooter's group (or 8-ball if group cleared)
            if (groupCleared) {
                if (shot.firstContactBallId != 8) foul = FoulReason::WrongBallFirstContact;
            } else {
                if (shooterGroup == BallGroup::Solids && !isSolid(shot.firstContactBallId)) foul = FoulReason::WrongBallFirstContact;
                if (shooterGroup == BallGroup::Stripes && !isStripe(shot.firstContactBallId)) foul = FoulReason::WrongBallFirstContact;
            }
        }

        if (foul == FoulReason::None && shot.ballsPocketed.empty() && !shot.railHitAfterContact) {
            foul = FoulReason::NoRailAfterContact;
        }

        // 8-Ball Pocketing Scenarios
        if (eightPocketed) {
            if (shot.isBreakShot) {
                // Official WPA 8-Ball break: pocketing 8-ball is rerack or spot
                out.eightOrNineRespotted = true;
                out.coachPuddinSpeech = "Coach Puddin: '8-ball on the break! Respotted on the foot spot.'";
            } else if (foul != FoulReason::None || !groupCleared) {
                // Pocketing 8 early or on foul -> Instant LOSS
                out.matchOutcome = (shot.shooter == PlayerId::Player1) ? MatchOutcome::Player2Win : MatchOutcome::Player1Win;
                matchOutcome_ = out.matchOutcome;
                out.isLegal = false;
                out.foulReason = FoulReason::EarlyEightBallPocketed;
                out.coachPuddinSpeech = "Coach Puddin: 'Early 8-ball pocketed or scratch! That is an instant loss of rack.'";
                return out;
            } else {
                // Legally pocketed 8-ball after clearing group -> WIN!
                out.matchOutcome = (shot.shooter == PlayerId::Player1) ? MatchOutcome::Player1Win : MatchOutcome::Player2Win;
                matchOutcome_ = out.matchOutcome;
                out.isLegal = true;
                out.coachPuddinSpeech = "Coach Puddin: '8-ball down cleanly! Rack goes to the shooter!'";
                return out;
            }
        }

        // Process Foul
        if (foul != FoulReason::None) {
            out.isLegal = false;
            out.foulReason = foul;
            out.ballInHand = true;
            out.nextShooter = getOpponent(shot.shooter);
            currentPlayer_ = out.nextShooter;

            int& fCount = (shot.shooter == PlayerId::Player1) ? p1ConsecutiveFouls_ : p2ConsecutiveFouls_;
            fCount++;

            if (fCount >= 3) {
                out.foulReason = FoulReason::ThreeConsecutiveFouls;
                out.matchOutcome = (shot.shooter == PlayerId::Player1) ? MatchOutcome::Player2Win : MatchOutcome::Player1Win;
                matchOutcome_ = out.matchOutcome;
                out.coachPuddinSpeech = "Coach Puddin: 'Three consecutive fouls forfeits the rack.'";
            } else if (fCount == 2) {
                out.coachPuddinWarning = true;
                out.coachPuddinSpeech = "Coach Puddin: 'Two consecutive fouls! Watch your contact angle.'";
            } else {
                out.coachPuddinSpeech = "Coach Puddin: 'Foul! Opponent receives Ball-in-Hand.'";
            }
        } else {
            // Legal Shot in 8-Ball
            out.isLegal = true;
            out.foulReason = FoulReason::None;
            if (shot.shooter == PlayerId::Player1) p1ConsecutiveFouls_ = 0;
            else p2ConsecutiveFouls_ = 0;

            // Group Assignment on Open Table
            if (isOpenTable_ && !shot.isBreakShot && !shot.ballsPocketed.empty()) {
                int firstPocketed = shot.ballsPocketed[0];
                if (firstPocketed != 8) {
                    isOpenTable_ = false;
                    if (isSolid(firstPocketed)) {
                        p1Group_ = (shot.shooter == PlayerId::Player1) ? BallGroup::Solids : BallGroup::Stripes;
                        p2Group_ = (shot.shooter == PlayerId::Player1) ? BallGroup::Stripes : BallGroup::Solids;
                        out.coachPuddinSpeech = "Coach Puddin: 'Groups assigned! Shooter is Solids (1-7), Opponent is Stripes (9-15).'";
                    } else if (isStripe(firstPocketed)) {
                        p1Group_ = (shot.shooter == PlayerId::Player1) ? BallGroup::Stripes : BallGroup::Solids;
                        p2Group_ = (shot.shooter == PlayerId::Player1) ? BallGroup::Solids : BallGroup::Stripes;
                        out.coachPuddinSpeech = "Coach Puddin: 'Groups assigned! Shooter is Stripes (9-15), Opponent is Solids (1-7).'";
                    }
                }
            }

            if (!shot.ballsPocketed.empty()) {
                out.nextShooter = shot.shooter;
                currentPlayer_ = shot.shooter;
                if (out.coachPuddinSpeech.empty()) {
                    out.coachPuddinSpeech = "Coach Puddin: 'Solid pot. You retain the table.'";
                }
            } else {
                out.nextShooter = getOpponent(shot.shooter);
                currentPlayer_ = out.nextShooter;
                out.coachPuddinSpeech = "Coach Puddin: 'Safe play. Turn passes to opponent.'";
            }
        }
    }

    out.p1ConsecutiveFouls = p1ConsecutiveFouls_;
    out.p2ConsecutiveFouls = p2ConsecutiveFouls_;
    out.p1Group = p1Group_;
    out.p2Group = p2Group_;
    return out;
}

} // namespace puddin::core
