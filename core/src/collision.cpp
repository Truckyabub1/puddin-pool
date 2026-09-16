#include "puddin/core/collision.hpp"
#include <algorithm>
#include <cmath>

namespace puddin::core {

Table Table::create_standard_table() {
    Table t;
    t.width = 2.24;
    t.height = 1.12;
    t.cushion_restitution = 0.85;
    t.cushion_restitution_alpha = 0.08;
    t.cushion_friction = 0.20;
    t.ball_restitution = 0.96;
    t.sliding_friction = 0.20;
    t.rolling_friction = 0.015;

    constexpr double corner_cut = 0.12;
    constexpr double side_half_cut = 0.065;
    const double mid_x = t.width * 0.5;

    // 1. Bottom Left Rail (facing +Y)
    t.rails.push_back(RailSegment::from_points(
        1,
        Vec2{corner_cut, 0.0},
        Vec2{mid_x - side_half_cut, 0.0},
        Vec2{0.0, 1.0}
    ));

    // 2. Bottom Right Rail (facing +Y)
    t.rails.push_back(RailSegment::from_points(
        2,
        Vec2{mid_x + side_half_cut, 0.0},
        Vec2{t.width - corner_cut, 0.0},
        Vec2{0.0, 1.0}
    ));

    // 3. Top Left Rail (facing -Y)
    t.rails.push_back(RailSegment::from_points(
        3,
        Vec2{corner_cut, t.height},
        Vec2{mid_x - side_half_cut, t.height},
        Vec2{0.0, -1.0}
    ));

    // 4. Top Right Rail (facing -Y)
    t.rails.push_back(RailSegment::from_points(
        4,
        Vec2{mid_x + side_half_cut, t.height},
        Vec2{t.width - corner_cut, t.height},
        Vec2{0.0, -1.0}
    ));

    // 5. Left Rail (facing +X)
    t.rails.push_back(RailSegment::from_points(
        5,
        Vec2{0.0, corner_cut},
        Vec2{0.0, t.height - corner_cut},
        Vec2{1.0, 0.0}
    ));

    // 6. Right Rail (facing -X)
    t.rails.push_back(RailSegment::from_points(
        6,
        Vec2{t.width, corner_cut},
        Vec2{t.width, t.height - corner_cut},
        Vec2{-1.0, 0.0}
    ));

    return t;
}

std::optional<double> time_to_ball_ball_impact(const Ball& b1, const Ball& b2) {
    if (b1.is_sunk || b2.is_sunk) return std::nullopt;

    Vec2 dr = b2.position - b1.position;
    Vec2 dv = b2.velocity - b1.velocity;

    double A = dv.length_sq();
    if (A < 1e-12) return std::nullopt;

    double B = 2.0 * dr.dot(dv);
    if (B >= 0.0) return std::nullopt; // Moving away from each other

    double R = b1.radius + b2.radius;
    double C = dr.length_sq() - R * R;

    double discr = B * B - 4.0 * A * C;
    if (discr < 0.0) return std::nullopt; // Miss

    double sqrt_discr = std::sqrt(std::max(0.0, discr));
    double t = (-B - sqrt_discr) / (2.0 * A);

    if (t < 1e-9) return std::nullopt;
    return t;
}

std::optional<double> time_to_ball_segment_impact(const Ball& b, const RailSegment& seg) {
    if (b.is_sunk) return std::nullopt;

    double vn = b.velocity.dot(seg.normal);
    if (vn >= -1e-9) return std::nullopt; // Not moving towards inward normal

    double d0 = (b.position - seg.p1).dot(seg.normal);
    if (d0 < b.radius * 0.5) return std::nullopt; // Penetrated behind rail

    double t = (d0 - b.radius) / (-vn);
    if (t < 1e-9) return std::nullopt;

    // Contact point at time t
    Vec2 pos_at_t = b.position + b.velocity * t;
    Vec2 contact = pos_at_t - seg.normal * b.radius;
    double s = (contact - seg.p1).dot(seg.tangent);

    if (s >= -1e-6 && s <= seg.length + 1e-6) {
        return t;
    }
    return std::nullopt;
}

std::optional<double> time_to_ball_point_impact(const Ball& b, const Vec2& point) {
    if (b.is_sunk) return std::nullopt;

    Vec2 dr = b.position - point;
    Vec2 dv = b.velocity;

    double A = dv.length_sq();
    if (A < 1e-12) return std::nullopt;

    double B = 2.0 * dr.dot(dv);
    if (B >= 0.0) return std::nullopt;

    double C = dr.length_sq() - b.radius * b.radius;
    double discr = B * B - 4.0 * A * C;
    if (discr < 0.0) return std::nullopt;

    double sqrt_discr = std::sqrt(std::max(0.0, discr));
    double t = (-B - sqrt_discr) / (2.0 * A);
    if (t < 1e-9) return std::nullopt;

    return t;
}

Vec2 compute_friction_acceleration(const Ball& b, const Table& table) {
    if (b.is_sunk || b.motion_state == MotionState::Stationary) {
        return {0.0, 0.0};
    }

    double spd = b.velocity.length();
    if (spd <= 1e-5) return {0.0, 0.0};

    Vec2 dir = b.velocity / spd;
    constexpr double g = 9.80665;

    if (b.motion_state == MotionState::Sliding) {
        return dir * -(table.sliding_friction * g);
    } else { // Rolling
        return dir * -(table.rolling_friction * g);
    }
}

std::optional<double> time_to_motion_transition(const Ball& b, const Table& table) {
    if (b.is_sunk || b.motion_state == MotionState::Stationary) {
        return std::nullopt;
    }

    double spd = b.velocity.length();
    constexpr double g = 9.80665;

    if (b.motion_state == MotionState::Sliding) {
        double a_s = table.sliding_friction * g;
        if (a_s <= 1e-9) return std::nullopt;
        if (spd > b.transition_speed) {
            double dt = (spd - b.transition_speed) / a_s;
            if (dt > 1e-9) return dt;
        }
        return 0.0;
    } else if (b.motion_state == MotionState::Rolling) {
        double a_r = table.rolling_friction * g;
        if (a_r <= 1e-9) return std::nullopt;
        if (spd > 1e-4) {
            double dt = spd / a_r;
            if (dt > 1e-9) return dt;
        }
        return 0.0;
    }

    return std::nullopt;
}

void resolve_ball_ball_collision(Ball& b1, Ball& b2, double restitution) {
    Vec2 dr = b2.position - b1.position;
    double dist = dr.length();
    if (dist < 1e-12) return;

    Vec2 normal = dr / dist;
    Vec2 rel_vel = b2.velocity - b1.velocity;
    double vn = rel_vel.dot(normal);

    if (vn >= 0.0) return; // Moving apart

    double inv_m1 = 1.0 / b1.mass;
    double inv_m2 = 1.0 / b2.mass;
    double impulse_mag = -(1.0 + restitution) * vn / (inv_m1 + inv_m2);
    Vec2 impulse = normal * impulse_mag;

    b1.velocity -= impulse * inv_m1;
    b2.velocity += impulse * inv_m2;

    // Both balls transition to sliding with analytical 5/7 transition velocity
    b1.motion_state = MotionState::Sliding;
    b1.transition_speed = b1.velocity.length() * (5.0 / 7.0);

    b2.motion_state = MotionState::Sliding;
    b2.transition_speed = b2.velocity.length() * (5.0 / 7.0);
}

void resolve_ball_rail_collision(
    Ball& b,
    const Vec2& normal,
    double base_restitution,
    double alpha,
    double friction
) {
    double vn = b.velocity.dot(normal);
    Vec2 tang{-normal.y, normal.x};
    double vt = b.velocity.dot(tang);

    // Non-linear restitution damping at higher normal velocities
    double effective_e = base_restitution * std::max(0.20, 1.0 - alpha * std::abs(vn));
    double vn_prime = -effective_e * vn;

    // Tangential friction damping
    double delta_vn = (1.0 + effective_e) * std::abs(vn);
    double max_friction_impulse = friction * delta_vn;

    double vt_prime = 0.0;
    if (std::abs(vt) <= max_friction_impulse) {
        vt_prime = 0.0; // Ball grips cushion tangentially
    } else {
        double sign = (vt > 0.0) ? 1.0 : -1.0;
        vt_prime = vt - sign * max_friction_impulse;
    }

    b.velocity = normal * vn_prime + tang * vt_prime;
    b.motion_state = MotionState::Sliding;
    b.transition_speed = b.velocity.length() * (5.0 / 7.0);
}

} // namespace puddin::core
