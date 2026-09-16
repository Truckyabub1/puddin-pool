#include "puddin/core/PuddinWisdomAI.h"

namespace puddin::core {

LegendPersona PuddinWisdomAI::select_appropriate_legend(const TableSituation& situation) {
    if (situation.is_snookered) {
        return LegendPersona::EfrenReyes;
    }
    if (situation.has_clusters || situation.remaining_balls >= 10) {
        return LegendPersona::WillieMosconi;
    }
    if (situation.cue_distance > 1.2 || situation.cut_angle_deg > 45.0) {
        return LegendPersona::AllisonFisher;
    }
    return LegendPersona::RonnieOSullivan;
}

const char* PuddinWisdomAI::get_legend_name(LegendPersona persona) {
    switch (persona) {
        case LegendPersona::EfrenReyes:
            return "Efren 'The Magician' Reyes";
        case LegendPersona::WillieMosconi:
            return "Willie Mosconi";
        case LegendPersona::AllisonFisher:
            return "Allison 'The Duchess' Fisher";
        case LegendPersona::RonnieOSullivan:
            return "Ronnie 'The Rocket' O'Sullivan";
    }
    return "Coach Puddin";
}

std::string PuddinWisdomAI::generate_coaching_advice(
    const TableSituation& situation,
    int difficulty_level
) {
    LegendPersona legend = select_appropriate_legend(situation);

    switch (legend) {
        case LegendPersona::EfrenReyes:
            if (difficulty_level >= 2) {
                return "Efren Reyes: 'Don't fight the geometry, son. Hit diamond 30 on the head rail with running English, carom two cushions, and the cue ball will tuck behind the 9-ball for a complete snooker.'";
            }
            return "Efren Reyes: 'Every wall is an open door if you hit it right. Kick off the long rail and let the diamond system do the work.'";

        case LegendPersona::WillieMosconi:
            if (difficulty_level >= 2) {
                return "Willie Mosconi: 'Look 3 shots ahead! That 4-ball near the foot rail is your Key Ball. Pocket the 2, draw back to the center of the table, and use the key ball to break the cluster wide open.'";
            }
            return "Willie Mosconi: 'Don't shoot without a plan. Find your Key Ball first, then the runout takes care of itself.'";

        case LegendPersona::AllisonFisher:
            if (difficulty_level >= 2) {
                return "Allison Fisher: 'Lock that bridge hand down firm on the felt. Smooth backstroke, pause at the transition, and deliver the tip into the center-of-pocket margin.'";
            }
            return "Allison Fisher: 'Precision beats power every day of the week. Soft hands, level cue, and steady follow-through.'";

        case LegendPersona::RonnieOSullivan:
            if (difficulty_level >= 2) {
                return "Ronnie O'Sullivan: 'Trust the natural angle! Play this with top-right spin at pace, break through the pack, and let the cue ball glide naturally into prime position.'";
            }
            return "Ronnie O'Sullivan: 'Keep your rhythm fluid, son. See the line, step into the shot, and let your natural stroke take over.'";
    }

    return "Coach Puddin: 'Smooth stroke, quiet mind. The balls always know where you told them to go.'";
}

} // namespace puddin::core
