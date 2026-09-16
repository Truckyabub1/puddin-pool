#pragma once

#include "puddin/core/types.hpp"
#include <vector>
#include <optional>

namespace puddin::core {

struct AimRay {
    Vec2 origin{0.0, 0.0};
    double angle_rad{0.0}; // Aim direction in radians (0 = +X, pi/2 = +Y)
    double power{1.0};     // Strike impulse magnitude
    double spin_x{0.0};    // Horizontal spin / English [-1.0, 1.0]
    double spin_y{0.0};    // Vertical spin: Draw (-1.0) to Follow (+1.0)

    [[nodiscard]] Vec2 direction() const {
        return {std::cos(angle_rad), std::sin(angle_rad)};
    }
};

struct TrajectoryResult {
    std::vector<Vec2> cue_path;       // Polyline points for cue ball
    std::vector<Vec2> target_path;    // Polyline points for struck target ball
    Vec2 ghost_ball_position{0.0, 0.0};
    Vec2 contact_normal{0.0, 0.0};    // Line of centers (target ball departure vector)
    Vec2 contact_tangent{0.0, 0.0};   // 90-degree tangent (cue ball deflection vector)
    uint32_t target_ball_id{0};
    double cut_angle_deg{0.0};
    bool has_target_hit{false};
};

// Pure, analytical trajectory predictor running in < 0.1ms
TrajectoryResult predict_trajectory(
    const SimulationState& state,
    const AimRay& ray,
    uint32_t max_bounces = 3
);

} // namespace puddin::core
