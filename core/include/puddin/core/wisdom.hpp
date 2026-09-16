#pragma once

#include <cstdint>

namespace puddin::core {

enum class ShotContextType : uint8_t {
    BreakShot = 0,
    LongDistanceCut = 1,
    SnookeredTrapped = 2,
    PostScratch = 3,
    HardBankOpportunity = 4,
    GreatShotCelebration = 5,
    GeneralWisdom = 6
};

enum class CoachEmoteState : uint8_t {
    IdleWise = 0,
    AimAdvice = 1,
    GreatShot = 2,
    ScratchConsoling = 3,
    HardBankCheer = 4
};

struct ShotContext {
    ShotContextType type{ShotContextType::GeneralWisdom};
    double cue_distance{0.0};
    double cut_angle_deg{0.0};
    int cushions_hit{0};
    bool was_scratch{false};
    bool ball_pocketed{false};
};

// Wisdom aphorism dispatcher
const char* get_shot_advice(const ShotContext& context);

// Determine emote reaction based on physical shot outcome
CoachEmoteState evaluate_coach_emote(const ShotContext& context);

} // namespace puddin::core
