#include "puddin/core/DiamondMath.h"
#include <algorithm>
#include <cmath>

namespace puddin::core {

DiamondBankResult DiamondMath::calculate_bank(
    double cue_origin_diamond,
    double third_rail_target,
    double speed_mps,
    double spin_english
) {
    DiamondBankResult result;
    result.cue_origin_diamond = cue_origin_diamond;
    result.third_rail_target = third_rail_target;

    // Classical Ceulemans Formula: Aim = Origin - 3rd Rail Target
    result.nominal_first_rail_aim = cue_origin_diamond - third_rail_target;

    // Speed adjustment: High speed compresses rail rubber, shortening the bounce angle
    // Standard speed is ~2.0 m/s
    double speed_delta = speed_mps - 2.0;
    result.speed_adjustment = -1.2 * speed_delta;

    // Spin / English adjustment: 1 tip of running English widens angle by ~5 diamond units
    result.spin_adjustment = spin_english * 5.0;

    result.final_aim_diamond = result.nominal_first_rail_aim + result.speed_adjustment + result.spin_adjustment;

    return result;
}

double DiamondMath::x_to_diamond_long(double x_meters, double table_width) {
    if (table_width <= 0.0) return 0.0;
    // 8 diamond segments along long rail
    double norm = std::clamp(x_meters / table_width, 0.0, 1.0);
    return norm * 8.0;
}

double DiamondMath::y_to_diamond_short(double y_meters, double table_height) {
    if (table_height <= 0.0) return 0.0;
    // 4 diamond segments along short rail
    double norm = std::clamp(y_meters / table_height, 0.0, 1.0);
    return norm * 4.0;
}

double DiamondMath::calculate_rail_friction_factor(double incident_speed, double spin_english) {
    // Friction matrix table lookup approximation
    // Low speeds have higher friction grab; high speeds skid with lower effective friction
    double base_friction = 0.20;
    double speed_attenuation = std::exp(-0.25 * std::max(0.0, incident_speed - 1.0));
    double spin_influence = 1.0 + 0.15 * std::abs(spin_english);

    return base_friction * speed_attenuation * spin_influence;
}

} // namespace puddin::core
