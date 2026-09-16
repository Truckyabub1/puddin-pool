#pragma once

namespace puddin::core {

struct DiamondBankResult {
    double cue_origin_diamond{50.0};  // e.g. Corner = 50
    double third_rail_target{20.0};   // e.g. Diamond 20
    double nominal_first_rail_aim{30.0}; // Aim = Origin - 3rd Rail Target
    double speed_adjustment{0.0};
    double spin_adjustment{0.0};
    double final_aim_diamond{30.0};
};

class DiamondMath {
public:
    // Classical Ceulemans Corner-50 System: Aim = Origin - 3rd Rail Target
    static DiamondBankResult calculate_bank(
        double cue_origin_diamond,
        double third_rail_target,
        double speed_mps = 2.0,
        double spin_english = 0.0 // [-1.0, 1.0] Running/Reverse English
    );

    // Convert meter coordinates on 2.24 x 1.12m table to diamond indices (0 to 8 long, 0 to 4 short)
    static double x_to_diamond_long(double x_meters, double table_width = 2.24);
    static double y_to_diamond_short(double y_meters, double table_height = 1.12);

    // Rail friction matrix adjustment factor
    static double calculate_rail_friction_factor(double incident_speed, double spin_english);
};

} // namespace puddin::core
