#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <optional>

namespace puddin::core {

enum class NineBallPlayer {
    Player1 = 1,
    Player2 = 2
};

enum class NineBallFoulType {
    None,
    CueBallScratch,
    WrongFirstContact,     // Did not hit lowest-numbered ball first
    NoRailAfterContact,    // No ball pocketed and no ball drove to cushion
    OffTable,
    ThreeConsecutiveFouls
};

enum class NineBallGameStatus {
    RackInProgress,
    Player1Win,
    Player2Win,
    PushOutPendingResponse // Break shooter called push out; opponent deciding
};

struct NineBallBallState {
    int id;                // 0 = cue, 1-9 = object balls
    double x;              // Table coordinates in meters
    double y;
    bool isSunk;
};

struct NineBallShotInput {
    NineBallPlayer shooter;
    bool isBreakShot;
    bool isPushOutCall;
    int firstContactBallId; // Ball ID contacted first by cue ball (-1 if no contact)
    bool railHitAfterContact;
    std::vector<int> ballsPocketed; // IDs of balls pocketed during the shot
    bool cueScratch;
};

struct NineBallShotResult {
    bool isLegal;
    NineBallFoulType foulType;
    NineBallGameStatus gameStatus;
    NineBallPlayer nextShooter;
    bool ballInHand;
    bool nineBallRespotted;
    int player1FoulCount;
    int player2FoulCount;
    bool coachPuddinWarningTriggered; // True on 2nd consecutive foul
    std::string coachPuddinMessage;
};

class NineBallRulesArbitrator {
public:
    NineBallRulesArbitrator();

    // Initialize/Reset 9-ball diamond rack:
    // 1-ball at apex (foot spot), 9-ball in center, remaining randomly placed
    void resetRack();

    // Evaluate completed shot against official WPA/BCA 9-ball rules
    NineBallShotResult evaluateShot(const NineBallShotInput& shot);

    // Opponent accepts or returns push-out shot
    void resolvePushOutResponse(bool acceptShot);

    // Accessors
    const std::vector<NineBallBallState>& getBalls() const { return balls_; }
    int getLowestBallOnTable() const;
    int getFoulCount(NineBallPlayer player) const;
    NineBallPlayer getCurrentPlayer() const { return currentPlayer_; }
    NineBallGameStatus getGameStatus() const { return gameStatus_; }
    bool isPushOutAvailable() const { return pushOutAvailable_; }

    // Table foot spot coordinates (meters)
    static constexpr double FOOT_SPOT_X = 2.24 * 0.70;
    static constexpr double FOOT_SPOT_Y = 1.12 * 0.50;
    static constexpr double BALL_RADIUS = 0.028575;

private:
    std::vector<NineBallBallState> balls_;
    NineBallPlayer currentPlayer_{NineBallPlayer::Player1};
    NineBallGameStatus gameStatus_{NineBallGameStatus::RackInProgress};

    int player1ConsecutiveFouls_{0};
    int player2ConsecutiveFouls_{0};

    bool isFirstShotAfterBreak_{false};
    bool pushOutAvailable_{false};
    bool pushOutActive_{false};

    void respotNineBall();
    NineBallPlayer getOpponent(NineBallPlayer player) const;
};

} // namespace puddin::core
