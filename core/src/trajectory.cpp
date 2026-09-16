#include "puddin/core/trajectory.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace puddin::core {

namespace {

constexpr double PI = 3.14159265358979323846;

struct RailHit {
    double distance{std::numeric_limits<double>::infinity()};
    Vec2 normal{0.0, 0.0};
    const RailSegment* segment{nullptr};
};

struct BallHit {
    double distance{std::numeric_limits<double>::infinity()};
    const Ball* ball{nullptr};
};

// Raycast against stationary target sphere
std::optional<double> raycast_sphere(
    const Vec2& origin,
    const Vec2& dir,
    const Vec2& target_pos,
    double sum_radius
) {
    Vec2 dr = target_pos - origin;
    double d_par = dr.dot(dir);
    if (d_par <= 1e-9) return std::nullopt; // Behind origin

    double dr_sq = dr.length_sq();
    double d_perp_sq = dr_sq - d_par * d_par;
    double r_sum_sq = sum_radius * sum_radius;

    if (d_perp_sq >= r_sum_sq) return std::nullopt; // Miss

    double d_in = std::sqrt(std::max(0.0, r_sum_sq - d_perp_sq));
    double s = d_par - d_in;
    if (s <= 1e-9) return std::nullopt;

    return s;
}

// Raycast against segmented rail cushion
std::optional<double> raycast_rail_segment(
    const Vec2& origin,
    const Vec2& dir,
    double radius,
    const RailSegment& seg
) {
    double vn = dir.dot(seg.normal);
    if (vn >= -1e-9) return std::nullopt; // Moving away from cushion

    double d0 = (origin - seg.p1).dot(seg.normal);
    if (d0 < radius * 0.5) return std::nullopt;

    double s = (d0 - radius) / (-vn);
    if (s <= 1e-9) return std::nullopt;

    Vec2 contact = (origin + dir * s) - seg.normal * radius;
    double u = (contact - seg.p1).dot(seg.tangent);

    if (u >= -1e-5 && u <= seg.length + 1e-5) {
        return s;
    }
    return std::nullopt;
}

// Trace multiple cushion bounces analytically
void trace_cushion_bounces(
    Vec2 start_pos,
    Vec2 direction,
    double radius,
    const std::vector<RailSegment>& rails,
    uint32_t max_bounces,
    std::vector<Vec2>& out_path,
    double spin_x
) {
    Vec2 current_pos = start_pos;
    Vec2 current_dir = direction.normalized();

    for (uint32_t bounce = 0; bounce < max_bounces; ++bounce) {
        RailHit closest_rail;

        for (const auto& rail : rails) {
            auto s = raycast_rail_segment(current_pos, current_dir, radius, rail);
            if (s && *s < closest_rail.distance) {
                closest_rail.distance = *s;
                closest_rail.normal = rail.normal;
                closest_rail.segment = &rail;
            }
        }

        if (!closest_rail.segment || std::isinf(closest_rail.distance)) {
            // No more cushion intersections; project forward to reasonable table bounds
            out_path.push_back(current_pos + current_dir * 0.8);
            break;
        }

        Vec2 hit_pos = current_pos + current_dir * closest_rail.distance;
        out_path.push_back(hit_pos);

        // Reflection vector: d' = d - 2*(d.n)*n
        Vec2 refl = current_dir - closest_rail.normal * (2.0 * current_dir.dot(closest_rail.normal));

        // Horizontal spin (English) alters cushion rebound angle
        if (std::abs(spin_x) > 1e-3) {
            // Tangent along cushion
            Vec2 tang{-closest_rail.normal.y, closest_rail.normal.x};
            double english_deflect = spin_x * 0.15; // Angular shift in radians
            double cs = std::cos(english_deflect);
            double sn = std::sin(english_deflect);
            refl = Vec2{refl.x * cs - refl.y * sn, refl.x * sn + refl.y * cs}.normalized();
        }

        current_pos = hit_pos;
        current_dir = refl.normalized();
    }
}

} // namespace

TrajectoryResult predict_trajectory(
    const SimulationState& state,
    const AimRay& ray,
    uint32_t max_bounces
) {
    TrajectoryResult result;
    Vec2 dir = ray.direction().normalized();
    result.cue_path.push_back(ray.origin);

    constexpr double cue_radius = 0.028575;

    // 1. Find earliest ball hit
    BallHit closest_ball;
    for (const auto& ball : state.balls) {
        if (ball.is_sunk) continue;
        // Don't collide with self if origin is inside ball
        if ((ball.position - ray.origin).length_sq() < 1e-6) continue;

        double sum_r = cue_radius + ball.radius;
        auto s = raycast_sphere(ray.origin, dir, ball.position, sum_r);
        if (s && *s < closest_ball.distance) {
            closest_ball.distance = *s;
            closest_ball.ball = &ball;
        }
    }

    // 2. Find earliest rail hit
    RailHit closest_rail;
    for (const auto& rail : state.table.rails) {
        auto s = raycast_rail_segment(ray.origin, dir, cue_radius, rail);
        if (s && *s < closest_rail.distance) {
            closest_rail.distance = *s;
            closest_rail.normal = rail.normal;
            closest_rail.segment = &rail;
        }
    }

    // 3. Evaluate if ball or cushion is hit first
    if (closest_ball.ball && closest_ball.distance < closest_rail.distance) {
        result.has_target_hit = true;
        result.target_ball_id = closest_ball.ball->id;

        // Ghost ball center position
        result.ghost_ball_position = ray.origin + dir * closest_ball.distance;
        result.cue_path.push_back(result.ghost_ball_position);

        // Contact normal: vector from ghost ball to target ball center
        Vec2 normal = (closest_ball.ball->position - result.ghost_ball_position).normalized();
        result.contact_normal = normal;

        // Contact tangent: perpendicular vector along cue ball deflection
        Vec2 tangent{-normal.y, normal.x};
        if (tangent.dot(dir) < 0.0) {
            tangent = tangent * -1.0;
        }
        result.contact_tangent = tangent;

        // Cut angle in degrees
        double cos_theta = std::clamp(dir.dot(normal), -1.0, 1.0);
        result.cut_angle_deg = std::acos(cos_theta) * (180.0 / PI);

        // Target ball trajectory
        result.target_path.push_back(closest_ball.ball->position);
        trace_cushion_bounces(
            closest_ball.ball->position,
            normal,
            closest_ball.ball->radius,
            state.table.rails,
            max_bounces,
            result.target_path,
            0.0
        );

        // Cue ball post-deflection trajectory with spin
        Vec2 cue_deflect_dir = tangent;
        if (ray.spin_y > 0.0) {
            // Topspin / Follow curves forward into initial aim direction
            cue_deflect_dir = (cue_deflect_dir + dir * (ray.spin_y * 0.50)).normalized();
        } else if (ray.spin_y < 0.0) {
            // Backspin / Draw pulls backward against contact normal
            cue_deflect_dir = (cue_deflect_dir - normal * (-ray.spin_y * 0.50)).normalized();
        }

        trace_cushion_bounces(
            result.ghost_ball_position,
            cue_deflect_dir,
            cue_radius,
            state.table.rails,
            max_bounces,
            result.cue_path,
            ray.spin_x
        );

    } else {
        // No ball hit; cue ball reflects off rails directly
        result.has_target_hit = false;
        trace_cushion_bounces(
            ray.origin,
            dir,
            cue_radius,
            state.table.rails,
            max_bounces,
            result.cue_path,
            ray.spin_x
        );
    }

    return result;
}

} // namespace puddin::core
