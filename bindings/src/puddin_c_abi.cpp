#include "puddin/bindings/puddin_c_abi.h"
#include "puddin/core/engine.hpp"
#include "puddin/core/trajectory.hpp"

#include <cstring>
#include <algorithm>

struct PuddinEngineHandle {
    puddin::core::Engine engine;
    explicit PuddinEngineHandle(puddin::core::Table table) : engine(std::move(table)) {}
};

extern "C" {

PuddinTableConfig puddin_table_config_default(void) {
    puddin::core::Table t = puddin::core::Table::create_standard_table();
    return PuddinTableConfig{
        .width = t.width,
        .height = t.height,
        .cushion_restitution = t.cushion_restitution,
        .cushion_restitution_alpha = t.cushion_restitution_alpha,
        .cushion_friction = t.cushion_friction,
        .ball_restitution = t.ball_restitution,
        .sliding_friction = t.sliding_friction,
        .rolling_friction = t.rolling_friction
    };
}

PuddinEngineHandle* puddin_engine_create(const PuddinTableConfig* config) {
    puddin::core::Table table = puddin::core::Table::create_standard_table();
    if (config) {
        table.width = config->width;
        table.height = config->height;
        table.cushion_restitution = config->cushion_restitution;
        table.cushion_restitution_alpha = config->cushion_restitution_alpha;
        table.cushion_friction = config->cushion_friction;
        table.ball_restitution = config->ball_restitution;
        table.sliding_friction = config->sliding_friction;
        table.rolling_friction = config->rolling_friction;
    }
    return new PuddinEngineHandle(std::move(table));
}

void puddin_engine_destroy(PuddinEngineHandle* handle) {
    delete handle;
}

void puddin_engine_reset(PuddinEngineHandle* handle) {
    if (handle) {
        handle->engine.reset();
    }
}

void puddin_engine_setup_standard_rack(PuddinEngineHandle* handle) {
    if (handle) {
        handle->engine.setup_standard_rack();
    }
}

uint32_t puddin_engine_add_ball(
    PuddinEngineHandle* handle,
    double x,
    double y,
    double vx,
    double vy,
    double radius,
    double mass
) {
    if (!handle) return 0;
    return handle->engine.add_ball(
        puddin::core::Vec2{x, y},
        puddin::core::Vec2{vx, vy},
        radius,
        mass
    );
}

bool puddin_engine_apply_impulse(
    PuddinEngineHandle* handle,
    uint32_t ball_id,
    double impulse_x,
    double impulse_y
) {
    if (!handle) return false;
    return handle->engine.apply_impulse(ball_id, puddin::core::Vec2{impulse_x, impulse_y});
}

void puddin_engine_step(PuddinEngineHandle* handle, double dt) {
    if (handle) {
        handle->engine.step(dt);
    }
}

void puddin_engine_step_micros(PuddinEngineHandle* handle, int64_t dt_micros) {
    if (handle) {
        handle->engine.step_micros(dt_micros);
    }
}

int64_t puddin_engine_get_clock_micros(const PuddinEngineHandle* handle) {
    if (!handle) return 0;
    return handle->engine.get_clock_micros();
}

uint32_t puddin_engine_get_ball_count(const PuddinEngineHandle* handle) {
    if (!handle) return 0;
    return static_cast<uint32_t>(handle->engine.get_balls().size());
}

uint32_t puddin_engine_get_ball_states(
    const PuddinEngineHandle* handle,
    PuddinBallState* out_balls,
    uint32_t max_count
) {
    if (!handle || !out_balls || max_count == 0) return 0;

    const auto& balls = handle->engine.get_balls();
    uint32_t count = std::min(static_cast<uint32_t>(balls.size()), max_count);

    for (uint32_t i = 0; i < count; ++i) {
        const auto& b = balls[i];
        out_balls[i] = PuddinBallState{
            .id = b.id,
            .position = {b.position.x, b.position.y},
            .velocity = {b.velocity.x, b.velocity.y},
            .radius = b.radius,
            .mass = b.mass,
            .motion_state = static_cast<PuddinMotionState>(b.motion_state),
            .transition_speed = b.transition_speed,
            .is_sunk = b.is_sunk
        };
    }

    return count;
}

bool puddin_engine_is_quiescent(const PuddinEngineHandle* handle, double threshold) {
    if (!handle) return true;
    return handle->engine.is_quiescent(threshold);
}

PuddinTrajectoryPreview puddin_engine_predict_trajectory(
    const PuddinEngineHandle* handle,
    double aim_angle_rad,
    double power,
    double spin_x,
    double spin_y,
    uint32_t max_bounces
) {
    PuddinTrajectoryPreview preview{};
    if (!handle) return preview;

    const auto& state = handle->engine.get_state();
    if (state.balls.empty()) return preview;

    // Default cue ball is ball ID 1
    puddin::core::Vec2 cue_pos = state.balls[0].position;
    for (const auto& b : state.balls) {
        if (b.id == 1 && !b.is_sunk) {
            cue_pos = b.position;
            break;
        }
    }

    puddin::core::AimRay ray{
        .origin = cue_pos,
        .angle_rad = aim_angle_rad,
        .power = power,
        .spin_x = spin_x,
        .spin_y = spin_y
    };

    auto result = puddin::core::predict_trajectory(state, ray, max_bounces);

    preview.has_target_hit = result.has_target_hit;
    preview.target_ball_id = result.target_ball_id;
    preview.cut_angle_deg = result.cut_angle_deg;
    preview.ghost_ball_position = {result.ghost_ball_position.x, result.ghost_ball_position.y};
    preview.contact_normal = {result.contact_normal.x, result.contact_normal.y};
    preview.contact_tangent = {result.contact_tangent.x, result.contact_tangent.y};

    preview.cue_point_count = std::min(
        static_cast<uint32_t>(result.cue_path.size()),
        static_cast<uint32_t>(PUDDIN_MAX_PREVIEW_POINTS)
    );
    for (uint32_t i = 0; i < preview.cue_point_count; ++i) {
        preview.cue_points[i] = {result.cue_path[i].x, result.cue_path[i].y};
    }

    preview.target_point_count = std::min(
        static_cast<uint32_t>(result.target_path.size()),
        static_cast<uint32_t>(PUDDIN_MAX_PREVIEW_POINTS)
    );
    for (uint32_t i = 0; i < preview.target_point_count; ++i) {
        preview.target_points[i] = {result.target_path[i].x, result.target_path[i].y};
    }

    return preview;
}

} // extern "C"
