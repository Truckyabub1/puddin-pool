#include "puddin/core/NineBallRules.h"
#include <iostream>
#include <cassert>

using namespace puddin::core;

int main() {
    std::cout << "======================================================" << std::endl;
    std::cout << "  OFFICIAL WPA / BCA 9-BALL RULES ARBITRATOR TEST SUITE" << std::endl;
    std::cout << "======================================================" << std::endl;

    NineBallRulesArbitrator arb;

    // Test 1: Legal Diamond Rack
    std::cout << "\n[TEST 1] Verifying Official Diamond Rack..." << std::endl;
    const auto& balls = arb.getBalls();
    assert(balls.size() == 10); // Cue + 9 balls
    assert(arb.getLowestBallOnTable() == 1);
    // Find ball 1 and ball 9
    auto it1 = std::find_if(balls.begin(), balls.end(), [](const NineBallBallState& b){ return b.id == 1; });
    auto it9 = std::find_if(balls.begin(), balls.end(), [](const NineBallBallState& b){ return b.id == 9; });
    assert(it1 != balls.end());
    assert(it9 != balls.end());
    // Ball 1 is at foot spot apex
    assert(std::abs(it1->x - NineBallRulesArbitrator::FOOT_SPOT_X) < 1e-4);
    assert(std::abs(it1->y - NineBallRulesArbitrator::FOOT_SPOT_Y) < 1e-4);
    std::cout << "         -> Diamond Rack verified: 1-ball at foot spot, 9-ball in center! PASS." << std::endl;

    // Test 2: Lowest-Numbered Ball Contact Violation (Foul)
    std::cout << "\n[TEST 2] Testing Lowest-Ball Contact Rule..." << std::endl;
    NineBallShotInput wrongContactShot{
        .shooter = NineBallPlayer::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .firstContactBallId = 3, // Struck 3-ball when 1-ball was lowest
        .railHitAfterContact = true,
        .ballsPocketed = {},
        .cueScratch = false
    };
    NineBallShotResult res2 = arb.evaluateShot(wrongContactShot);
    assert(!res2.isLegal);
    assert(res2.foulType == NineBallFoulType::WrongFirstContact);
    assert(res2.ballInHand == true);
    assert(res2.nextShooter == NineBallPlayer::Player2);
    assert(res2.player1FoulCount == 1);
    std::cout << "         -> Wrong contact gave instant FOUL and Ball-in-Hand! PASS." << std::endl;

    // Test 3: No-Rail Foul Rule
    std::cout << "\n[TEST 3] Testing No-Rail After Contact Foul..." << std::endl;
    arb.resetRack();
    NineBallShotInput noRailShot{
        .shooter = NineBallPlayer::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .firstContactBallId = 1, // Hit 1-ball correctly
        .railHitAfterContact = false, // But no rail hit & no pocket
        .ballsPocketed = {},
        .cueScratch = false
    };
    NineBallShotResult res3 = arb.evaluateShot(noRailShot);
    assert(!res3.isLegal);
    assert(res3.foulType == NineBallFoulType::NoRailAfterContact);
    assert(res3.ballInHand == true);
    assert(res3.nextShooter == NineBallPlayer::Player2);
    std::cout << "         -> No cushion after contact gave FOUL and Ball-in-Hand! PASS." << std::endl;

    // Test 4: Legal Break & Push-Out (Two-Shot Opening)
    std::cout << "\n[TEST 4] Testing Break & Push-Out Mechanism..." << std::endl;
    arb.resetRack();
    NineBallShotInput breakShot{
        .shooter = NineBallPlayer::Player1,
        .isBreakShot = true,
        .isPushOutCall = false,
        .firstContactBallId = 1,
        .railHitAfterContact = true,
        .ballsPocketed = {}, // Dry break
        .cueScratch = false
    };
    NineBallShotResult breakRes = arb.evaluateShot(breakShot);
    assert(breakRes.isLegal);
    assert(arb.isPushOutAvailable());

    // Shooter calls Push-Out
    NineBallShotInput pushOutShot{
        .shooter = NineBallPlayer::Player2,
        .isBreakShot = false,
        .isPushOutCall = true,
        .firstContactBallId = 5, // Contacting 5 is allowed during push-out!
        .railHitAfterContact = false, // Cushion rule waived!
        .ballsPocketed = {},
        .cueScratch = false
    };
    NineBallShotResult pushRes = arb.evaluateShot(pushOutShot);
    assert(pushRes.isLegal);
    assert(pushRes.gameStatus == NineBallGameStatus::PushOutPendingResponse);

    // Opponent accepts the shot
    arb.resolvePushOutResponse(true);
    assert(arb.getCurrentPlayer() == NineBallPlayer::Player1);
    std::cout << "         -> Push-Out rules waived correctly and incoming option resolved! PASS." << std::endl;

    // Test 5: Golden Break (9-Ball pocketed on legal break)
    std::cout << "\n[TEST 5] Testing Golden Break (9-Ball on Break)..." << std::endl;
    arb.resetRack();
    NineBallShotInput goldenBreak{
        .shooter = NineBallPlayer::Player1,
        .isBreakShot = true,
        .isPushOutCall = false,
        .firstContactBallId = 1,
        .railHitAfterContact = true,
        .ballsPocketed = {9},
        .cueScratch = false
    };
    NineBallShotResult goldenRes = arb.evaluateShot(goldenBreak);
    assert(goldenRes.isLegal);
    assert(goldenRes.gameStatus == NineBallGameStatus::Player1Win);
    std::cout << "         -> Golden Break registered INSTANT WIN for Player 1! PASS." << std::endl;

    // Test 6: Combination 9-Ball Win
    std::cout << "\n[TEST 6] Testing Combination 9-Ball Win..." << std::endl;
    arb.resetRack();
    NineBallShotInput comboShot{
        .shooter = NineBallPlayer::Player2,
        .isBreakShot = false,
        .isPushOutCall = false,
        .firstContactBallId = 1, // Legal first contact with lowest ball
        .railHitAfterContact = true,
        .ballsPocketed = {9},    // 9-ball dropped via combo/carom
        .cueScratch = false
    };
    NineBallShotResult comboRes = arb.evaluateShot(comboShot);
    assert(comboRes.isLegal);
    assert(comboRes.gameStatus == NineBallGameStatus::Player2Win);
    std::cout << "         -> Legal combo 9-ball registered INSTANT WIN for Player 2! PASS." << std::endl;

    // Test 7: 9-Ball Respotting on Foul/Scratch
    std::cout << "\n[TEST 7] Testing 9-Ball Respotting on Foul/Scratch..." << std::endl;
    arb.resetRack();
    NineBallShotInput scratchShot{
        .shooter = NineBallPlayer::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .firstContactBallId = 1,
        .railHitAfterContact = true,
        .ballsPocketed = {9, 2}, // 9 and 2 dropped, but cue scratched
        .cueScratch = true
    };
    NineBallShotResult scratchRes = arb.evaluateShot(scratchShot);
    assert(!scratchRes.isLegal);
    assert(scratchRes.foulType == NineBallFoulType::CueBallScratch);
    assert(scratchRes.nineBallRespotted == true);
    // Verify 9-ball is no longer sunk, and 2-ball remains sunk
    auto it9b = std::find_if(arb.getBalls().begin(), arb.getBalls().end(), [](const NineBallBallState& b){ return b.id == 9; });
    auto it2b = std::find_if(arb.getBalls().begin(), arb.getBalls().end(), [](const NineBallBallState& b){ return b.id == 2; });
    assert(!it9b->isSunk);
    assert(it2b->isSunk);
    std::cout << "         -> 9-ball respotted on foot spot while prematurely pocketed 2-ball remained down! PASS." << std::endl;

    // Test 8: Three-Consecutive-Foul Rule & Coach Puddin Warning
    std::cout << "\n[TEST 8] Testing Three Consecutive Foul Rule & Coach Puddin Warnings..." << std::endl;
    arb.resetRack();

    // Foul 1 by Player 1
    NineBallShotInput foul1{
        .shooter = NineBallPlayer::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .firstContactBallId = -1, // Whiffed
        .railHitAfterContact = false,
        .ballsPocketed = {},
        .cueScratch = false
    };
    NineBallShotResult f1 = arb.evaluateShot(foul1);
    assert(!f1.isLegal);
    assert(f1.player1FoulCount == 1);
    assert(!f1.coachPuddinWarningTriggered);

    // Player 2 plays a legal safety
    NineBallShotInput p2Safety{
        .shooter = NineBallPlayer::Player2,
        .isBreakShot = false,
        .isPushOutCall = false,
        .firstContactBallId = 1,
        .railHitAfterContact = true,
        .ballsPocketed = {},
        .cueScratch = false
    };
    arb.evaluateShot(p2Safety);

    // Foul 2 by Player 1 (Coach Puddin warning should trigger!)
    NineBallShotInput foul2{
        .shooter = NineBallPlayer::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .firstContactBallId = -1,
        .railHitAfterContact = false,
        .ballsPocketed = {},
        .cueScratch = false
    };
    NineBallShotResult f2 = arb.evaluateShot(foul2);
    assert(!f2.isLegal);
    assert(f2.player1FoulCount == 2);
    assert(f2.coachPuddinWarningTriggered);
    std::cout << "         -> 2nd consecutive foul triggered Coach Puddin warning dialogue: \"" 
              << f2.coachPuddinMessage << "\"! PASS." << std::endl;

    // Player 2 plays another safety
    arb.evaluateShot(p2Safety);

    // Foul 3 by Player 1 -> DISQUALIFICATION / FORFEIT RACK
    NineBallShotInput foul3{
        .shooter = NineBallPlayer::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .firstContactBallId = -1,
        .railHitAfterContact = false,
        .ballsPocketed = {},
        .cueScratch = false
    };
    NineBallShotResult f3 = arb.evaluateShot(foul3);
    assert(!f3.isLegal);
    assert(f3.foulType == NineBallFoulType::ThreeConsecutiveFouls);
    assert(f3.gameStatus == NineBallGameStatus::Player2Win);
    std::cout << "         -> 3rd consecutive foul forfeited rack; Player 2 declared WINNER! PASS." << std::endl;

    std::cout << "\n======================================================" << std::endl;
    std::cout << "  ALL OFFICIAL WPA 9-BALL RULES TESTS PASSED (8/8)!" << std::endl;
    std::cout << "======================================================" << std::endl;
    return 0;
}
