#include "puddin/core/wisdom.hpp"
#include "puddin/core/engine.hpp"
#include "puddin_pool_ffi.h"

#include <iostream>
#include <cassert>
#include <cstring>
#include <string>

void test_wisdom_advice_dispatch() {
    std::cout << "[COACH TEST] 1. Testing wisdom advice dispatcher...\n";

    // Break shot advice
    const char* break_adv = get_coach_puddin_advice(0, 0.0f, 0.0f, false);
    assert(break_adv != nullptr);
    assert(std::string(break_adv).find("center of that head ball") != std::string::npos);
    std::cout << "             -> Break Advice: \"" << break_adv << "\"\n";

    // Long distance cut advice
    const char* cut_adv = get_coach_puddin_advice(1, 55.0f, 1.4f, false);
    assert(cut_adv != nullptr);
    assert(std::string(cut_adv).find("Plan your angles carefully") != std::string::npos);
    std::cout << "             -> Long Cut Advice: \"" << cut_adv << "\"\n";

    // Snookered advice
    const char* snook_adv = get_coach_puddin_advice(2, 0.0f, 0.0f, false);
    assert(snook_adv != nullptr);
    assert(std::string(snook_adv).find("Think three rails ahead") != std::string::npos);
    std::cout << "             -> Snookered Advice: \"" << snook_adv << "\"\n";

    // Post-scratch advice
    const char* scratch_adv = get_coach_puddin_advice(3, 0.0f, 0.0f, true);
    assert(scratch_adv != nullptr);
    assert(std::string(scratch_adv).find("Table’s forgiving") != std::string::npos);
    std::cout << "             -> Scratch Advice: \"" << scratch_adv << "\"\n";

    std::cout << "             -> PASS: All contextual advice aphorisms verified!\n";
}

void test_coach_emote_state_transitions() {
    std::cout << "[COACH TEST] 2. Testing emote state transitions on physics outcomes...\n";

    // 1. Scratch -> SCRATCH_CONSOLING (3)
    int emote_scratch = evaluate_coach_puddin_emote(0, 0, 0.0f, true, false);
    assert(emote_scratch == static_cast<int>(puddin::core::CoachEmoteState::ScratchConsoling));
    std::cout << "             -> Scratch Emote: SCRATCH_CONSOLING (3)\n";

    // 2. Multi-cushion bank shot pot -> HARD_BANK_CHEER (4)
    int emote_bank = evaluate_coach_puddin_emote(4, 2, 45.0f, false, true);
    assert(emote_bank == static_cast<int>(puddin::core::CoachEmoteState::HardBankCheer));
    std::cout << "             -> 2-Cushion Bank Pot Emote: HARD_BANK_CHEER (4)\n";

    // 3. Regular ball pot -> GREAT_SHOT (2)
    int emote_pot = evaluate_coach_puddin_emote(0, 0, 20.0f, false, true);
    assert(emote_pot == static_cast<int>(puddin::core::CoachEmoteState::GreatShot));
    std::cout << "             -> Standard Pot Emote: GREAT_SHOT (2)\n";

    // 4. Aiming long cut -> AIM_ADVICE (1)
    int emote_aim = evaluate_coach_puddin_emote(1, 0, 50.0f, false, false);
    assert(emote_aim == static_cast<int>(puddin::core::CoachEmoteState::AimAdvice));
    std::cout << "             -> Long Cut Aiming Emote: AIM_ADVICE (1)\n";

    // 5. Idle / general -> IDLE_WISE (0)
    int emote_idle = evaluate_coach_puddin_emote(6, 0, 0.0f, false, false);
    assert(emote_idle == static_cast<int>(puddin::core::CoachEmoteState::IdleWise));
    std::cout << "             -> General Idle Emote: IDLE_WISE (0)\n";

    std::cout << "             -> PASS: Emote state transitions accurately match physics states!\n";
}

void test_physics_simulation_binding() {
    std::cout << "[COACH TEST] 3. Binding Coach Puddin to live physics simulation...\n";

    init_table(2.24f, 1.12f);
    reset_rack(0);

    // Initial break aim
    int initial_emote = evaluate_coach_puddin_emote(0, 0, 0.0f, false, false);
    assert(initial_emote == 1 && "Break shot aiming must trigger AIM_ADVICE");

    // Strike cue ball into rack
    execute_strike(0.01f, 3.5f, 0.0f, 0.0f);

    BallTransform transforms[16];
    int active_count = 0;
    bool pocketed_any = false;
    for (int frame = 0; frame < 100; ++frame) {
        step_simulation(0.016f, transforms, 16, &active_count);
        for (int i = 0; i < active_count; ++i) {
            if (transforms[i].is_pocketed) {
                pocketed_any = true;
                break;
            }
        }
    }

    int post_break_emote = evaluate_coach_puddin_emote(0, 0, 0.0f, false, pocketed_any);
    if (pocketed_any) {
        assert(post_break_emote == 2 && "Pocketing on break must trigger GREAT_SHOT");
        std::cout << "             -> Ball pocketed on break: GREAT_SHOT triggered!\n";
    } else {
        assert(post_break_emote == 0 && "Dry break must transition to IDLE_WISE");
        std::cout << "             -> Dry break: IDLE_WISE transition confirmed.\n";
    }

    std::cout << "             -> PASS: Live physics event binding verified!\n";
}

int main() {
    std::cout << "=================================================================\n";
    std::cout << " Puddin Pool: Coach Puddin Mentor & Emote Verification\n";
    std::cout << "=================================================================\n";

    test_wisdom_advice_dispatch();
    test_coach_emote_state_transitions();
    test_physics_simulation_binding();

    std::cout << "=================================================================\n";
    std::cout << " ALL COACH PUDDIN VERIFICATION TESTS PASSED!\n";
    std::cout << "=================================================================\n";
    return 0;
}
