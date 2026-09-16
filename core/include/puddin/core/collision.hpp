#pragma once

#include "puddin/core/types.hpp"
#include <optional>

namespace puddin::core {

enum class EventType : uint8_t {
    BallBallCollision,
    BallRailCollision,
    BallSlideToRollTransition,
    BallStopTransition
};

struct CollisionEvent {
    double time{0.0}; // Seconds relative to start of interval
    EventType type{EventType::BallBallCollision};
    uint32_t primary_ball_id{0};
    uint32_t secondary_id{0}; // secondary ball_id or rail_id
    Vec2 contact_normal{0.0, 0.0};
};

// Analytical quadratic collision solver for two moving spheres
std::optional<double> time_to_ball_ball_impact(const Ball& b1, const Ball& b2);

// Analytical collision solver for ball impacting a line segment
std::optional<double> time_to_ball_segment_impact(const Ball& b, const RailSegment& seg);

// Analytical collision solver for ball impacting a stationary vertex point
std::optional<double> time_to_ball_point_impact(const Ball& b, const Vec2& point);

// Friction acceleration for ball given current motion state
Vec2 compute_friction_acceleration(const Ball& b, const Table& table);

// Analytical time until next friction state transition (sliding -> rolling, or rolling -> stationary)
std::optional<double> time_to_motion_transition(const Ball& b, const Table& table);

// Elastic collision response for two balls
void resolve_ball_ball_collision(Ball& b1, Ball& b2, double restitution);

// Cushion rail collision response with non-linear restitution and tangential friction
void resolve_ball_rail_collision(
    Ball& b,
    const Vec2& normal,
    double base_restitution,
    double alpha,
    double friction
);

} // namespace puddin::core
