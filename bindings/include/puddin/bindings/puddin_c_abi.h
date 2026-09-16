#pragma once

#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32)
    #if defined(PUDDIN_EXPORTS)
        #define PUDDIN_API __declspec(dllexport)
    #else
        #define PUDDIN_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define PUDDIN_API __attribute__((visibility("default")))
#else
    #define PUDDIN_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PuddinEngineHandle PuddinEngineHandle;

typedef struct {
    double x;
    double y;
} PuddinVec2;

typedef PuddinVec2 PuddinPoint2D;

typedef enum {
    PUDDIN_MOTION_STATIONARY = 0,
    PUDDIN_MOTION_SLIDING = 1,
    PUDDIN_MOTION_ROLLING = 2
} PuddinMotionState;

typedef struct {
    uint32_t id;
    PuddinVec2 position;
    PuddinVec2 velocity;
    double radius;
    double mass;
    PuddinMotionState motion_state;
    double transition_speed;
    bool is_sunk;
} PuddinBallState;

typedef struct {
    double width;
    double height;
    double cushion_restitution;
    double cushion_restitution_alpha;
    double cushion_friction;
    double ball_restitution;
    double sliding_friction;
    double rolling_friction;
} PuddinTableConfig;

#define PUDDIN_MAX_PREVIEW_POINTS 32

typedef struct {
    PuddinPoint2D cue_points[PUDDIN_MAX_PREVIEW_POINTS];
    uint32_t cue_point_count;
    PuddinPoint2D target_points[PUDDIN_MAX_PREVIEW_POINTS];
    uint32_t target_point_count;
    PuddinPoint2D ghost_ball_position;
    PuddinPoint2D contact_normal;
    PuddinPoint2D contact_tangent;
    uint32_t target_ball_id;
    double cut_angle_deg;
    bool has_target_hit;
} PuddinTrajectoryPreview;

PUDDIN_API PuddinTableConfig puddin_table_config_default(void);

PUDDIN_API PuddinEngineHandle* puddin_engine_create(const PuddinTableConfig* config);
PUDDIN_API void puddin_engine_destroy(PuddinEngineHandle* handle);

PUDDIN_API void puddin_engine_reset(PuddinEngineHandle* handle);
PUDDIN_API void puddin_engine_setup_standard_rack(PuddinEngineHandle* handle);

PUDDIN_API uint32_t puddin_engine_add_ball(
    PuddinEngineHandle* handle,
    double x,
    double y,
    double vx,
    double vy,
    double radius,
    double mass
);

PUDDIN_API bool puddin_engine_apply_impulse(
    PuddinEngineHandle* handle,
    uint32_t ball_id,
    double impulse_x,
    double impulse_y
);

PUDDIN_API void puddin_engine_step(PuddinEngineHandle* handle, double dt);
PUDDIN_API void puddin_engine_step_micros(PuddinEngineHandle* handle, int64_t dt_micros);
PUDDIN_API int64_t puddin_engine_get_clock_micros(const PuddinEngineHandle* handle);

PUDDIN_API uint32_t puddin_engine_get_ball_count(const PuddinEngineHandle* handle);

PUDDIN_API uint32_t puddin_engine_get_ball_states(
    const PuddinEngineHandle* handle,
    PuddinBallState* out_balls,
    uint32_t max_count
);

PUDDIN_API bool puddin_engine_is_quiescent(const PuddinEngineHandle* handle, double threshold);

// Ghost-ball & multi-bounce trajectory prediction API
PUDDIN_API PuddinTrajectoryPreview puddin_engine_predict_trajectory(
    const PuddinEngineHandle* handle,
    double aim_angle_rad,
    double power,
    double spin_x,
    double spin_y,
    uint32_t max_bounces
);

#ifdef __cplusplus
}
#endif
