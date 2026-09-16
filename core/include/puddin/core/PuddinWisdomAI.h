#pragma once

#include "puddin/core/types.hpp"
#include <string>

namespace puddin::core {

enum class LegendPersona : uint8_t {
    EfrenReyes = 0,     // The Magician: Multi-rail kicks & impossible safeties
    WillieMosconi = 1,  // The Runout King: Key Ball identification & 3-shot planning
    AllisonFisher = 2,  // The Duchess of Doom: Precision bridge, speed control, entry margin
    RonnieOSullivan = 3 // The Rocket: Natural flow, rapid break breakout, cue ball speed
};

struct TableSituation {
    bool is_snookered{false};
    bool has_clusters{false};
    double cue_distance{0.0};
    double cut_angle_deg{0.0};
    bool is_break_shot{false};
    int remaining_balls{15};
};

class PuddinWisdomAI {
public:
    static std::string generate_coaching_advice(
        const TableSituation& situation,
        int difficulty_level = 1 // 0 = Novice, 1 = Master, 2 = Legend
    );

    static LegendPersona select_appropriate_legend(const TableSituation& situation);

    static const char* get_legend_name(LegendPersona persona);
};

} // namespace puddin::core
