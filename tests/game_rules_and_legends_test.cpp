#include "puddin/core/GameRules.h"
#include "puddin/core/DiamondMath.h"
#include "puddin/core/PuddinWisdomAI.h"
#include "puddin_pool_ffi.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

void test_eight_ball_rules() {
    std::cout << "[RULES TEST] 1. Testing 8-Ball WPA rules...\n";
    puddin::core::GameRulesEngine rules(puddin::core::GameVariant::EightBall);

    assert(rules.is_table_open() && "Table must start open in 8-Ball");

    // Open table shot: called 3-ball pocketed legally
    puddin::core::ShotOutcome shot1{
        .first_ball_hit = 3,
        .pocketed_balls = {3},
        .cushions_after_contact = 1,
        .was_scratch = false,
        .called_ball = 3,
        .called_pocket = 1
    };
    std::vector<bool> active(16, true);
    auto res1 = rules.process_shot(shot1, active);
    assert(res1 == puddin::core::TurnResult::ContinueTurn);
    assert(!rules.is_table_open() && "Table must be closed after called ball pocketed");
    assert(rules.get_player_group(1) == puddin::core::PlayerGroup::Solids);
    assert(rules.get_player_group(2) == puddin::core::PlayerGroup::Stripes);
    std::cout << "             -> Group assignment: Player 1 = Solids, Player 2 = Stripes\n";

    // Player 1 legal 8-ball called shot for the win
    puddin::core::ShotOutcome shot_win{
        .first_ball_hit = 8,
        .pocketed_balls = {8},
        .cushions_after_contact = 1,
        .was_scratch = false,
        .called_ball = 8,
        .called_pocket = 2
    };
    auto res_win = rules.process_shot(shot_win, active);
    assert(res_win == puddin::core::TurnResult::Win && "Legal 8-ball call must result in Win");
    std::cout << "             -> Legal 8-Ball called pot: WIN confirmed!\n";

    // Early or scratch 8-ball results in Loss
    puddin::core::GameRulesEngine rules_scratch(puddin::core::GameVariant::EightBall);
    puddin::core::ShotOutcome shot_loss{
        .first_ball_hit = 8,
        .pocketed_balls = {8},
        .was_scratch = true
    };
    auto res_loss = rules_scratch.process_shot(shot_loss, active);
    assert(res_loss == puddin::core::TurnResult::Loss && "Scratch on 8-ball must result in Loss");
    std::cout << "             -> Scratch on 8-Ball: LOSS confirmed!\n";
}

void test_nine_ball_rules() {
    std::cout << "[RULES TEST] 2. Testing 9-Ball rotational rules...\n";
    puddin::core::GameRulesEngine rules(puddin::core::GameVariant::NineBall);

    std::vector<bool> active(10, true);
    active[0] = true;

    // Lowest ball is 1: legal hit on 1, caroming into 9 for break win
    puddin::core::ShotOutcome break_shot{
        .first_ball_hit = 1,
        .pocketed_balls = {9},
        .cushions_after_contact = 2,
        .was_scratch = false
    };
    auto res_break = rules.process_shot(break_shot, active);
    assert(res_break == puddin::core::TurnResult::Win && "9-ball pocketed on legal break must Win");
    std::cout << "             -> 9-Ball on legal break: WIN confirmed!\n";

    // Wrong ball hit first (hit 3 when 1 is lowest on table) -> foul / switch turn
    puddin::core::ShotOutcome foul_shot{
        .first_ball_hit = 3,
        .pocketed_balls = {3},
        .was_scratch = false
    };
    auto res_foul = rules.process_shot(foul_shot, active);
    assert(res_foul == puddin::core::TurnResult::SwitchTurn && "Wrong ball first must switch turn");
    std::cout << "             -> Wrong ball hit first: FOUL confirmed!\n";
}

void test_straight_pool_and_one_pocket() {
    std::cout << "[RULES TEST] 3. Testing 14.1 Continuous & One-Pocket rules...\n";

    // 14.1 Continuous
    puddin::core::GameRulesEngine straight_pool(puddin::core::GameVariant::StraightPool14_1);
    std::vector<bool> active(16, true);

    for (int i = 1; i <= 14; ++i) {
        puddin::core::ShotOutcome shot{
            .first_ball_hit = i,
            .pocketed_balls = {i},
            .called_ball = i,
            .called_pocket = 1
        };
        straight_pool.process_shot(shot, active);
    }
    assert(straight_pool.get_player_score(1) == 14);
    assert(straight_pool.needs_rerack_14_1() && "14.1 must trigger re-rack at 14 balls");
    std::cout << "             -> 14.1 Continuous: 14 points scored, Re-rack triggered!\n";

    // One-Pocket
    puddin::core::GameRulesEngine one_pocket(puddin::core::GameVariant::OnePocket);
    puddin::core::ShotOutcome p1_own_pocket{
        .pocketed_balls = {2},
        .called_pocket = 1 // P1 assigned pocket
    };
    one_pocket.process_shot(p1_own_pocket, active);
    assert(one_pocket.get_player_score(1) == 1);

    puddin::core::ShotOutcome p1_opp_pocket{
        .pocketed_balls = {3},
        .called_pocket = 3 // P2 assigned pocket
    };
    one_pocket.process_shot(p1_opp_pocket, active);
    assert(one_pocket.get_player_score(2) == 1 && "Pocketing in opponent pocket must award point to opponent");
    std::cout << "             -> One-Pocket: P1 scored in P1 pocket, P2 awarded point from P2 pocket!\n";
}

void test_three_cushion_carom() {
    std::cout << "[RULES TEST] 4. Testing 3-Cushion Carom pocketless rules...\n";
    puddin::core::GameRulesEngine carom(puddin::core::GameVariant::ThreeCushionCarom);
    std::vector<bool> active(4, true);

    // Hit with only 2 cushions before 2nd carom -> no point
    puddin::core::ShotOutcome fail_carom{
        .cushions_before_second_carom = 2,
        .hit_second_carom = true
    };
    auto res_fail = carom.process_shot(fail_carom, active);
    assert(res_fail == puddin::core::TurnResult::SwitchTurn);
    assert(carom.get_player_score(1) == 0);

    // Hit with 3 cushions before 2nd carom -> 1 point scored!
    puddin::core::ShotOutcome valid_carom{
        .cushions_before_second_carom = 3,
        .hit_second_carom = true
    };
    auto res_valid = carom.process_shot(valid_carom, active);
    assert(res_valid == puddin::core::TurnResult::ContinueTurn);
    assert(carom.get_player_score(2) == 1); // Turn switched to P2, so P2 scored
    std::cout << "             -> 3-Cushion Carom: 3 rail contacts before 2nd carom awarded point!\n";
}

void test_ceulemans_diamond_math() {
    std::cout << "[DIAMOND TEST] 5. Testing Ceulemans Corner-50 Diamond System...\n";

    // Corner 50 origin, 20 target on third rail: Aim = 50 - 20 = 30
    auto result = puddin::core::DiamondMath::calculate_bank(50.0, 20.0, 2.0, 0.0);
    assert(std::abs(result.nominal_first_rail_aim - 30.0) < 1e-6);
    assert(std::abs(result.final_aim_diamond - 30.0) < 1e-6);
    std::cout << "               -> Ceulemans Corner-50: Origin 50 - Target 20 = Aim Diamond "
              << result.nominal_first_rail_aim << "\n";

    // With running English (+0.5 tip): Aim adjusts by +2.5 diamonds
    auto result_spin = puddin::core::DiamondMath::calculate_bank(50.0, 20.0, 2.0, 0.5);
    assert(result_spin.final_aim_diamond > 30.0);
    std::cout << "               -> With Running English (+0.5): Aim Diamond adjusted to "
              << result_spin.final_aim_diamond << "\n";

    // C-ABI call
    float aim_c = compute_diamond_bank_aim(50.0f, 20.0f, 2.0f, 0.0f);
    assert(std::abs(aim_c - 30.0f) < 1e-3f);
    std::cout << "               -> C-ABI compute_diamond_bank_aim verified: " << aim_c << "\n";
}

void test_legend_wisdom_ai() {
    std::cout << "[LEGEND TEST] 6. Testing Coach Puddin's Legend Wisdom AI...\n";

    // 1. Snookered situation -> Efren Reyes
    puddin::core::TableSituation snookered_sit{
        .is_snookered = true,
        .has_clusters = false,
        .cue_distance = 0.8,
        .cut_angle_deg = 20.0,
        .is_break_shot = false,
        .remaining_balls = 8
    };
    auto legend1 = puddin::core::PuddinWisdomAI::select_appropriate_legend(snookered_sit);
    assert(legend1 == puddin::core::LegendPersona::EfrenReyes);
    std::string adv1 = puddin::core::PuddinWisdomAI::generate_coaching_advice(snookered_sit, 2);
    assert(adv1.find("Efren Reyes") != std::string::npos);
    std::cout << "              -> Snookered Persona: " << puddin::core::PuddinWisdomAI::get_legend_name(legend1) << "\n";
    std::cout << "                 Quote: \"" << adv1 << "\"\n";

    // 2. Clusters on table -> Willie Mosconi (Key Ball)
    puddin::core::TableSituation cluster_sit{
        .is_snookered = false,
        .has_clusters = true,
        .cue_distance = 0.5,
        .cut_angle_deg = 15.0,
        .is_break_shot = false,
        .remaining_balls = 12
    };
    auto legend2 = puddin::core::PuddinWisdomAI::select_appropriate_legend(cluster_sit);
    assert(legend2 == puddin::core::LegendPersona::WillieMosconi);
    std::string adv2 = puddin::core::PuddinWisdomAI::generate_coaching_advice(cluster_sit, 2);
    assert(adv2.find("Key Ball") != std::string::npos);
    std::cout << "              -> Cluster Persona: " << puddin::core::PuddinWisdomAI::get_legend_name(legend2) << "\n";
    std::cout << "                 Quote: \"" << adv2 << "\"\n";

    // 3. Steep cut angle & distance -> Allison Fisher
    puddin::core::TableSituation cut_sit{
        .is_snookered = false,
        .has_clusters = false,
        .cue_distance = 1.5,
        .cut_angle_deg = 55.0,
        .is_break_shot = false,
        .remaining_balls = 5
    };
    auto legend3 = puddin::core::PuddinWisdomAI::select_appropriate_legend(cut_sit);
    assert(legend3 == puddin::core::LegendPersona::AllisonFisher);
    std::string adv3 = puddin::core::PuddinWisdomAI::generate_coaching_advice(cut_sit, 2);
    assert(adv3.find("Allison Fisher") != std::string::npos);
    std::cout << "              -> Long Cut Persona: " << puddin::core::PuddinWisdomAI::get_legend_name(legend3) << "\n";
    std::cout << "                 Quote: \"" << adv3 << "\"\n";

    // 4. Natural breakout -> Ronnie O'Sullivan
    puddin::core::TableSituation runout_sit{
        .is_snookered = false,
        .has_clusters = false,
        .cue_distance = 0.6,
        .cut_angle_deg = 20.0,
        .is_break_shot = false,
        .remaining_balls = 3
    };
    auto legend4 = puddin::core::PuddinWisdomAI::select_appropriate_legend(runout_sit);
    assert(legend4 == puddin::core::LegendPersona::RonnieOSullivan);
    std::string adv4 = puddin::core::PuddinWisdomAI::generate_coaching_advice(runout_sit, 2);
    assert(adv4.find("Ronnie O'Sullivan") != std::string::npos);
    std::cout << "              -> Runout Persona: " << puddin::core::PuddinWisdomAI::get_legend_name(legend4) << "\n";
    std::cout << "                 Quote: \"" << adv4 << "\"\n";

    // C-ABI export test
    const char* c_abi_advice = get_puddin_coaching_advice(2, true, false, 0.8f, 20.0f);
    assert(c_abi_advice != nullptr);
    assert(std::string(c_abi_advice).find("Efren Reyes") != std::string::npos);
    std::cout << "              -> C-ABI get_puddin_coaching_advice verified!\n";
}

int main() {
    std::cout << "=================================================================\n";
    std::cout << " Puddin Pool: Step 11 Game Modes & Legends Verification\n";
    std::cout << "=================================================================\n";

    test_eight_ball_rules();
    test_nine_ball_rules();
    test_straight_pool_and_one_pocket();
    test_three_cushion_carom();
    test_ceulemans_diamond_math();
    test_legend_wisdom_ai();

    std::cout << "=================================================================\n";
    std::cout << " ALL GAME MODES & LEGEND SYSTEM TESTS PASSED!\n";
    std::cout << "=================================================================\n";
    return 0;
}
