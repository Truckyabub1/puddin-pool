#include "puddin/core/OfficialRules.h"
#include <iostream>
#include <cassert>

using namespace puddin::core;

int main() {
    std::cout << "======================================================" << std::endl;
    std::cout << "  OFFICIAL 8-BALL & 9-BALL RULES ARBITRATOR TEST SUITE" << std::endl;
    std::cout << "======================================================" << std::endl;

    // ----------------------------------------------------
    // TEST 1: 8-BALL OPEN TABLE & GROUP ASSIGNMENT
    // ----------------------------------------------------
    std::cout << "\n[TEST 1] Testing 8-Ball Open Table & Group Assignment..." << std::endl;
    OfficialRulesArbitrator arb8(GameVariant::EightBall);
    assert(arb8.getPlayerGroup(PlayerId::Player1) == BallGroup::Unassigned);

    // Player 1 pockets ball 3 (solid) legally on open table
    std::vector<int> unsunk8 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    ShotInput shot1{
        .shooter = PlayerId::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .calledPocketIndex = 0,
        .firstContactBallId = 3,
        .railHitAfterContact = true,
        .ballsPocketed = {3},
        .cueScratch = false
    };
    ShotOutcome res1 = arb8.evaluateShot(shot1, unsunk8);
    assert(res1.isLegal);
    assert(res1.p1Group == BallGroup::Solids);
    assert(res1.p2Group == BallGroup::Stripes);
    assert(arb8.getPlayerGroup(PlayerId::Player1) == BallGroup::Solids);
    assert(arb8.getPlayerGroup(PlayerId::Player2) == BallGroup::Stripes);
    std::cout << "         -> Open table assigned Solids to P1 and Stripes to P2! PASS." << std::endl;

    // ----------------------------------------------------
    // TEST 2: 8-BALL WRONG GROUP CONTACT FOUL
    // ----------------------------------------------------
    std::cout << "\n[TEST 2] Testing 8-Ball Wrong Group Contact Foul..." << std::endl;
    // P1 (Solids) hits ball 10 (Stripe) first
    ShotInput shot2{
        .shooter = PlayerId::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .calledPocketIndex = 0,
        .firstContactBallId = 10, // Struck stripe first
        .railHitAfterContact = true,
        .ballsPocketed = {},
        .cueScratch = false
    };
    ShotOutcome res2 = arb8.evaluateShot(shot2, unsunk8);
    assert(!res2.isLegal);
    assert(res2.foulReason == FoulReason::WrongBallFirstContact);
    assert(res2.ballInHand);
    assert(res2.nextShooter == PlayerId::Player2);
    std::cout << "         -> Striking opponent's ball first gave FOUL & Ball-in-Hand! PASS." << std::endl;

    // ----------------------------------------------------
    // TEST 3: 8-BALL EARLY 8-BALL LOSS
    // ----------------------------------------------------
    std::cout << "\n[TEST 3] Testing 8-Ball Early 8-Ball Pocketing Loss..." << std::endl;
    // P2 pockets 8-ball while their stripes remain on table
    ShotInput shot3{
        .shooter = PlayerId::Player2,
        .isBreakShot = false,
        .isPushOutCall = false,
        .calledPocketIndex = 0,
        .firstContactBallId = 11,
        .railHitAfterContact = true,
        .ballsPocketed = {8}, // 8-ball dropped prematurely
        .cueScratch = false
    };
    ShotOutcome res3 = arb8.evaluateShot(shot3, unsunk8);
    assert(!res3.isLegal);
    assert(res3.foulReason == FoulReason::EarlyEightBallPocketed);
    assert(res3.matchOutcome == MatchOutcome::Player1Win); // Opponent wins!
    std::cout << "         -> Early 8-ball registered INSTANT LOSS of rack! PASS." << std::endl;

    // ----------------------------------------------------
    // TEST 4: 8-BALL LEGAL 8-BALL WIN
    // ----------------------------------------------------
    std::cout << "\n[TEST 4] Testing 8-Ball Legal Win on Cleared Group..." << std::endl;
    OfficialRulesArbitrator arb8_win(GameVariant::EightBall);
    // Assign groups
    arb8_win.evaluateShot(shot1, unsunk8);
    // P1 only has 8-ball remaining
    std::vector<int> clearedSolids = {8, 9, 10, 11, 12, 13, 14, 15};
    ShotInput shotWin8{
        .shooter = PlayerId::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .calledPocketIndex = 2,
        .firstContactBallId = 8,
        .railHitAfterContact = true,
        .ballsPocketed = {8},
        .cueScratch = false
    };
    ShotOutcome resWin8 = arb8_win.evaluateShot(shotWin8, clearedSolids);
    assert(resWin8.isLegal);
    assert(resWin8.matchOutcome == MatchOutcome::Player1Win);
    std::cout << "         -> Cleared group + 8-ball down legally gave WIN for Player 1! PASS." << std::endl;

    // ----------------------------------------------------
    // TEST 5: 9-BALL LOWEST BALL & PUSH-OUT ARBITRATION
    // ----------------------------------------------------
    std::cout << "\n[TEST 5] Testing 9-Ball Lowest-Ball Contact & Push-Out..." << std::endl;
    OfficialRulesArbitrator arb9(GameVariant::NineBall);
    std::vector<int> unsunk9 = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    // Legal break
    ShotInput break9{
        .shooter = PlayerId::Player1,
        .isBreakShot = true,
        .isPushOutCall = false,
        .calledPocketIndex = -1,
        .firstContactBallId = 1,
        .railHitAfterContact = true,
        .ballsPocketed = {},
        .cueScratch = false
    };
    ShotOutcome breakRes = arb9.evaluateShot(break9, unsunk9);
    assert(breakRes.isLegal);
    assert(arb9.isPushOutAvailable());

    // Push out called
    ShotInput pushOut{
        .shooter = PlayerId::Player2,
        .isBreakShot = false,
        .isPushOutCall = true,
        .calledPocketIndex = -1,
        .firstContactBallId = 7, // Any ball allowed on push-out
        .railHitAfterContact = false, // No rail needed on push-out
        .ballsPocketed = {},
        .cueScratch = false
    };
    ShotOutcome pushRes = arb9.evaluateShot(pushOut, unsunk9);
    assert(pushRes.isLegal);
    assert(pushRes.matchOutcome == MatchOutcome::PushOutDecisionPending);
    arb9.resolvePushOut(true);
    assert(arb9.getCurrentPlayer() == PlayerId::Player1);
    std::cout << "         -> 9-ball break and push-out rules verified! PASS." << std::endl;

    // ----------------------------------------------------
    // TEST 6: THREE-CONSECUTIVE-FOUL RULE & COACH PUDDIN ALERT
    // ----------------------------------------------------
    std::cout << "\n[TEST 6] Testing Three Consecutive Foul Rule & Coach Puddin Warning..." << std::endl;
    OfficialRulesArbitrator arbFoul(GameVariant::NineBall);

    ShotInput foulShot{
        .shooter = PlayerId::Player1,
        .isBreakShot = false,
        .isPushOutCall = false,
        .calledPocketIndex = -1,
        .firstContactBallId = -1, // Whiff
        .railHitAfterContact = false,
        .ballsPocketed = {},
        .cueScratch = false
    };

    // Foul 1
    ShotOutcome f1 = arbFoul.evaluateShot(foulShot, unsunk9);
    assert(f1.p1ConsecutiveFouls == 1);
    assert(!f1.coachPuddinWarning);

    // Opponent legal safety
    ShotInput p2Safe{
        .shooter = PlayerId::Player2,
        .isBreakShot = false,
        .isPushOutCall = false,
        .calledPocketIndex = -1,
        .firstContactBallId = 1,
        .railHitAfterContact = true,
        .ballsPocketed = {},
        .cueScratch = false
    };
    arbFoul.evaluateShot(p2Safe, unsunk9);

    // Foul 2 (Coach Puddin tactical warning!)
    ShotOutcome f2 = arbFoul.evaluateShot(foulShot, unsunk9);
    assert(f2.p1ConsecutiveFouls == 2);
    assert(f2.coachPuddinWarning);
    std::cout << "         -> 2nd foul triggered Coach Puddin alert: \"" << f2.coachPuddinSpeech << "\"! PASS." << std::endl;

    // Opponent safe
    arbFoul.evaluateShot(p2Safe, unsunk9);

    // Foul 3 -> FORFEITURE
    ShotOutcome f3 = arbFoul.evaluateShot(foulShot, unsunk9);
    assert(f3.foulReason == FoulReason::ThreeConsecutiveFouls);
    assert(f3.matchOutcome == MatchOutcome::Player2Win);
    std::cout << "         -> 3rd consecutive foul forfeited rack; Player 2 declared WINNER! PASS." << std::endl;

    std::cout << "\n======================================================" << std::endl;
    std::cout << "  ALL OFFICIAL RULES TESTS PASSED (6/6)!" << std::endl;
    std::cout << "======================================================" << std::endl;
    return 0;
}
