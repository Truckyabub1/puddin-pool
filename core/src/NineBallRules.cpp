#include "puddin/core/NineBallRules.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace puddin::core {

NineBallRulesArbitrator::NineBallRulesArbitrator() {
    resetRack();
}

void NineBallRulesArbitrator::resetRack() {
    balls_.clear();
    currentPlayer_ = NineBallPlayer::Player1;
    gameStatus_ = NineBallGameStatus::RackInProgress;
    player1ConsecutiveFouls_ = 0;
    player2ConsecutiveFouls_ = 0;
    isFirstShotAfterBreak_ = false;
    pushOutAvailable_ = false;
    pushOutActive_ = false;

    // 0: Cue ball at head string
    balls_.push_back({0, 2.24 * 0.25, 1.12 * 0.50, false});

    // 1-9: Diamond Rack
    // Row 0: 1 ball  (1-ball)
    // Row 1: 2 balls (balls 2, 3)
    // Row 2: 3 balls (ball 4, 9-ball in center, ball 5)
    // Row 3: 2 balls (balls 6, 7)
    // Row 4: 1 ball  (ball 8)
    const double apexX = FOOT_SPOT_X;
    const double apexY = FOOT_SPOT_Y;
    const double d = BALL_RADIUS * 2.0;
    const double rowSpacing = d * std::sqrt(3.0) * 0.5;

    // Standard official diamond placement order: 1 at apex, 9 in center
    const std::vector<int> diamondBallIds = {
        1,       // Row 0
        2, 3,    // Row 1
        4, 9, 5, // Row 2 (9-ball in center)
        6, 7,    // Row 3
        8        // Row 4
    };

    int idx = 0;
    for (int row = 0; row < 5; ++row) {
        int countInRow = (row <= 2) ? (row + 1) : (5 - row);
        double rowX = apexX + row * rowSpacing;
        double startY = apexY - (countInRow - 1) * d * 0.5;

        for (int c = 0; c < countInRow; ++c) {
            double ballY = startY + c * d;
            int ballId = diamondBallIds[idx++];
            balls_.push_back({ballId, rowX, ballY, false});
        }
    }

    // Sort by id for quick indexing
    std::sort(balls_.begin(), balls_.end(), [](const NineBallBallState& a, const NineBallBallState& b) {
        return a.id < b.id;
    });
}

int NineBallRulesArbitrator::getLowestBallOnTable() const {
    for (int id = 1; id <= 9; ++id) {
        auto it = std::find_if(balls_.begin(), balls_.end(), [id](const NineBallBallState& b) {
            return b.id == id && !b.isSunk;
        });
        if (it != balls_.end()) {
            return id;
        }
    }
    return -1;
}

int NineBallRulesArbitrator::getFoulCount(NineBallPlayer player) const {
    return (player == NineBallPlayer::Player1) ? player1ConsecutiveFouls_ : player2ConsecutiveFouls_;
}

NineBallPlayer NineBallRulesArbitrator::getOpponent(NineBallPlayer player) const {
    return (player == NineBallPlayer::Player1) ? NineBallPlayer::Player2 : NineBallPlayer::Player1;
}

void NineBallRulesArbitrator::respotNineBall() {
    auto it = std::find_if(balls_.begin(), balls_.end(), [](const NineBallBallState& b) {
        return b.id == 9;
    });
    if (it == balls_.end()) return;

    it->isSunk = false;
    double spotX = FOOT_SPOT_X;
    double spotY = FOOT_SPOT_Y;
    const double d = BALL_RADIUS * 2.0;

    // Check if foot spot is occupied; if so, move straight back along long string
    bool occupied = true;
    while (occupied) {
        occupied = false;
        for (const auto& b : balls_) {
            if (b.id != 9 && !b.isSunk) {
                double dist = std::hypot(b.x - spotX, b.y - spotY);
                if (dist < d) {
                    occupied = true;
                    spotX += d; // Move towards back rail along long string
                    break;
                }
            }
        }
    }

    it->x = spotX;
    it->y = spotY;
}

void NineBallRulesArbitrator::resolvePushOutResponse(bool acceptShot) {
    if (gameStatus_ != NineBallGameStatus::PushOutPendingResponse) return;

    NineBallPlayer incoming = getOpponent(currentPlayer_);
    if (acceptShot) {
        currentPlayer_ = incoming;
    } else {
        // Passed back to shooter
        // currentPlayer_ stays same
    }
    gameStatus_ = NineBallGameStatus::RackInProgress;
    pushOutAvailable_ = false;
    isFirstShotAfterBreak_ = false;
}

NineBallShotResult NineBallRulesArbitrator::evaluateShot(const NineBallShotInput& shot) {
    NineBallShotResult res{};
    res.coachPuddinWarningTriggered = false;
    res.nineBallRespotted = false;

    // 1. Mark pocketed balls as sunk in internal state
    bool ninePocketed = false;
    for (int pid : shot.ballsPocketed) {
        if (pid == 9) ninePocketed = true;
        auto it = std::find_if(balls_.begin(), balls_.end(), [pid](NineBallBallState& b) {
            return b.id == pid;
        });
        if (it != balls_.end()) {
            it->isSunk = true;
        }
    }

    if (shot.cueScratch) {
        auto it = std::find_if(balls_.begin(), balls_.end(), [](NineBallBallState& b) {
            return b.id == 0;
        });
        if (it != balls_.end()) {
            it->isSunk = true;
        }
    }

    // 2. Determine lowest numbered ball prior to shot
    int lowestBall = getLowestBallOnTable();

    // Handle Push-Out shot
    if (pushOutAvailable_ && shot.isPushOutCall) {
        if (shot.cueScratch) {
            // Scratch on push out is still a foul!
            res.isLegal = false;
            res.foulType = NineBallFoulType::CueBallScratch;
            res.ballInHand = true;
            res.nextShooter = getOpponent(shot.shooter);
            currentPlayer_ = res.nextShooter;
            pushOutAvailable_ = false;
            isFirstShotAfterBreak_ = false;

            if (shot.shooter == NineBallPlayer::Player1) player1ConsecutiveFouls_++;
            else player2ConsecutiveFouls_++;

            if (ninePocketed) {
                respotNineBall();
                res.nineBallRespotted = true;
            }
            res.coachPuddinMessage = "Coach Puddin: 'Scratch on a push-out gives ball-in-hand! Keep that cue ball disciplined.'";
            return res;
        }

        // Legal push-out execution (lowest ball and cushion rules waived)
        if (ninePocketed) {
            // 9-ball pocketed on push out must be respotted
            respotNineBall();
            res.nineBallRespotted = true;
        }

        res.isLegal = true;
        res.foulType = NineBallFoulType::None;
        res.gameStatus = NineBallGameStatus::PushOutPendingResponse;
        gameStatus_ = NineBallGameStatus::PushOutPendingResponse;
        res.ballInHand = false;
        res.nextShooter = getOpponent(shot.shooter);
        res.coachPuddinMessage = "Coach Puddin: 'Push-out called! Opponent can take the shot or pass it right back.'";
        return res;
    }

    // Reset push out availability after any non-push-out shot
    pushOutAvailable_ = false;

    // 3. Evaluate Fouls
    NineBallFoulType foul = NineBallFoulType::None;

    if (shot.cueScratch) {
        foul = NineBallFoulType::CueBallScratch;
    } else if (shot.firstContactBallId != lowestBall) {
        foul = NineBallFoulType::WrongFirstContact;
    } else if (shot.ballsPocketed.empty() && !shot.railHitAfterContact) {
        foul = NineBallFoulType::NoRailAfterContact;
    }

    // 4. Process Foul vs Legal Shot
    if (foul != NineBallFoulType::None) {
        res.isLegal = false;
        res.foulType = foul;
        res.ballInHand = true;
        res.nextShooter = getOpponent(shot.shooter);
        currentPlayer_ = res.nextShooter;

        // If 9-ball was pocketed illegally or on scratch, respot it!
        if (ninePocketed) {
            respotNineBall();
            res.nineBallRespotted = true;
        }

        // Increment consecutive fouls
        int& fouls = (shot.shooter == NineBallPlayer::Player1) ? player1ConsecutiveFouls_ : player2ConsecutiveFouls_;
        fouls++;

        if (fouls >= 3) {
            res.foulType = NineBallFoulType::ThreeConsecutiveFouls;
            res.gameStatus = (shot.shooter == NineBallPlayer::Player1) ? NineBallGameStatus::Player2Win : NineBallGameStatus::Player1Win;
            gameStatus_ = res.gameStatus;
            res.coachPuddinMessage = "Coach Puddin: 'Three consecutive fouls! That is a rack forfeit according to official WPA rules.'";
        } else if (fouls == 2) {
            res.coachPuddinWarningTriggered = true;
            res.coachPuddinMessage = "Coach Puddin: 'Careful, son! That is TWO consecutive fouls! One more foul forfeits the rack.'";
        } else {
            if (foul == NineBallFoulType::CueBallScratch) {
                res.coachPuddinMessage = "Coach Puddin: 'Scratch! Ball-in-hand anywhere on the table for your opponent.'";
            } else if (foul == NineBallFoulType::WrongFirstContact) {
                std::ostringstream oss;
                oss << "Coach Puddin: 'Foul! You must contact the lowest ball (the " << lowestBall << "-ball) first.'";
                res.coachPuddinMessage = oss.str();
            } else {
                res.coachPuddinMessage = "Coach Puddin: 'Foul! A ball must reach a cushion or drop in a pocket after contact.'";
            }
        }
    } else {
        // Legal Shot
        res.isLegal = true;
        res.foulType = NineBallFoulType::None;

        // Reset shooter's consecutive fouls upon legal shot
        if (shot.shooter == NineBallPlayer::Player1) player1ConsecutiveFouls_ = 0;
        else player2ConsecutiveFouls_ = 0;

        // Check 9-Ball Win condition:
        // Break 9-ball win OR legal combination/carom win
        if (ninePocketed) {
            res.gameStatus = (shot.shooter == NineBallPlayer::Player1) ? NineBallGameStatus::Player1Win : NineBallGameStatus::Player2Win;
            gameStatus_ = res.gameStatus;
            if (shot.isBreakShot) {
                res.coachPuddinMessage = "Coach Puddin: 'GOLDEN BREAK! 9-ball dropped on the break! Pure masterclass.'";
            } else {
                res.coachPuddinMessage = "Coach Puddin: 'Sensational combination! 9-ball pocketed legally for the win!'";
            }
            res.nextShooter = shot.shooter;
            return res;
        }

        // Enable Push-out if this was a legal break shot
        if (shot.isBreakShot) {
            isFirstShotAfterBreak_ = true;
            pushOutAvailable_ = true;
        } else {
            isFirstShotAfterBreak_ = false;
        }

        // Continuing turn if at least one ball pocketed legally
        if (!shot.ballsPocketed.empty()) {
            res.nextShooter = shot.shooter;
            currentPlayer_ = shot.shooter;
            res.ballInHand = false;
            res.coachPuddinMessage = "Coach Puddin: 'Good shot. Stay down on your cue and plan your next angle.'";
        } else {
            // Legal safety or dry shot -> turn passes to opponent
            res.nextShooter = getOpponent(shot.shooter);
            currentPlayer_ = res.nextShooter;
            res.ballInHand = false;
            res.coachPuddinMessage = "Coach Puddin: 'Solid tactical safety. Table passes to the opponent.'";
        }
    }

    res.player1FoulCount = player1ConsecutiveFouls_;
    res.player2FoulCount = player2ConsecutiveFouls_;
    return res;
}

} // namespace puddin::core
