#include "puddin/core/wisdom.hpp"

namespace puddin::core {

const char* get_shot_advice(const ShotContext& context) {
    if (context.was_scratch) {
        return "Table’s forgiving, long as you learn where that cue ball wanted to go. Dust it off, son!";
    }

    switch (context.type) {
        case ShotContextType::BreakShot:
            return "Don't just hit it hard, aim right through the center of that head ball.";

        case ShotContextType::LongDistanceCut:
            return "Take your time, son! Plan your angles carefully, the shot reveals itself.";

        case ShotContextType::SnookeredTrapped:
            return "Think three rails ahead. Every wall is an open door if you hit it right.";

        case ShotContextType::HardBankOpportunity:
            return "Bank shots aren't luck, they're geometry with a little respect for the cushion.";

        case ShotContextType::GreatShotCelebration:
            return "That's how you let the cue stick do the talking, son! Beautiful stroke.";

        case ShotContextType::PostScratch:
            return "Table’s forgiving, long as you learn where that cue ball wanted to go.";

        case ShotContextType::GeneralWisdom:
        default:
            return "Smooth stroke, quiet mind. The balls always know what you truly told them.";
    }
}

CoachEmoteState evaluate_coach_emote(const ShotContext& context) {
    if (context.was_scratch) {
        return CoachEmoteState::ScratchConsoling;
    }

    if (context.ball_pocketed) {
        if (context.cushions_hit >= 2 || context.cut_angle_deg > 65.0) {
            return CoachEmoteState::HardBankCheer;
        }
        return CoachEmoteState::GreatShot;
    }

    if (context.type == ShotContextType::BreakShot ||
        context.type == ShotContextType::LongDistanceCut ||
        context.type == ShotContextType::SnookeredTrapped) {
        return CoachEmoteState::AimAdvice;
    }

    return CoachEmoteState::IdleWise;
}

} // namespace puddin::core
