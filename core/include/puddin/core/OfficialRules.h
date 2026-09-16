#pragma once

#include <vector>
#include <string>
#include <optional>
#include <cstdint>

namespace puddin::core {

enum class GameVariant {
    EightBall,
    NineBall
};

enum class PlayerId {
    Player1 = 1,
    Player2 = 2
};

enum class BallGroup {
    Unassigned, // Open table
    Solids,     // 1 - 7
    Stripes     // 9 - 15
};

enum class FoulReason {
    None,
    CueBallScratch,
    WrongBallFirstContact,
    NoRailAfterContact,
    OffTable,
    ThreeConsecutiveFouls,
    EarlyEightBallPocketed,
    EightBallInWrongPocket
};

enum class MatchOutcome {
    InProgress,
    Player1Win,
    Player2Win,
    PushOutDecisionPending
};

struct ShotInput {
    PlayerId shooter;
    bool isBreakShot;
    bool isPushOutCall;
    int calledPocketIndex; // For 8-ball (0-5, or -1 if not called)
    int firstContactBallId; // -1 if no ball hit
    bool railHitAfterContact;
    std::vector<int> ballsPocketed;
    bool cueScratch;
};

struct ShotOutcome {
    bool isLegal;
    FoulReason foulReason;
    MatchOutcome matchOutcome;
    PlayerId nextShooter;
    bool ballInHand;
    bool eightOrNineRespotted;
    int p1ConsecutiveFouls;
    int p2ConsecutiveFouls;
    BallGroup p1Group;
    BallGroup p2Group;
    bool coachPuddinWarning; // Triggered on 2nd consecutive foul
    std::string coachPuddinSpeech;
};

class OfficialRulesArbitrator {
public:
    explicit OfficialRulesArbitrator(GameVariant variant = GameVariant::NineBall);

    void resetRack(GameVariant variant);

    ShotOutcome evaluateShot(const ShotInput& shot, const std::vector<int>& unsunkBalls);

    void resolvePushOut(bool acceptShot);

    GameVariant getVariant() const { return variant_; }
    PlayerId getCurrentPlayer() const { return currentPlayer_; }
    BallGroup getPlayerGroup(PlayerId player) const;
    MatchOutcome getMatchOutcome() const { return matchOutcome_; }
    bool isPushOutAvailable() const { return pushOutAvailable_; }
    int getFoulCount(PlayerId player) const;

private:
    GameVariant variant_;
    PlayerId currentPlayer_{PlayerId::Player1};
    MatchOutcome matchOutcome_{MatchOutcome::InProgress};

    // 8-Ball specific state
    BallGroup p1Group_{BallGroup::Unassigned};
    BallGroup p2Group_{BallGroup::Unassigned};
    bool isOpenTable_{true};

    // Consecutive foul tracking
    int p1ConsecutiveFouls_{0};
    int p2ConsecutiveFouls_{0};

    // 9-Ball push out state
    bool isFirstShotAfterBreak_{false};
    bool pushOutAvailable_{false};

    PlayerId getOpponent(PlayerId player) const;
    int getLowestBall(const std::vector<int>& unsunkBalls) const;
    bool isSolid(int id) const { return id >= 1 && id <= 7; }
    bool isStripe(int id) const { return id >= 9 && id <= 15; }
};

} // namespace puddin::core
