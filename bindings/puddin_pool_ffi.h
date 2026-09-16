#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32)
    #if defined(PUDDIN_EXPORTS)
        #define PUDDIN_FFI_API __declspec(dllexport)
    #else
        #define PUDDIN_FFI_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define PUDDIN_FFI_API __attribute__((visibility("default")))
#else
    #define PUDDIN_FFI_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x;
    float y;
} NativeVector2;

typedef struct {
    int id;
    float x;
    float y;
    float rot_quaternion[4]; // [x, y, z, w]
    bool is_pocketed;
} BallTransform;

typedef struct {
    NativeVector2 cue_points[16];
    int cue_count;
    NativeVector2 target_points[16];
    int target_count;
    int target_ball_id;
} TrajectoryBuffer;

PUDDIN_FFI_API void init_table(float width, float height);
PUDDIN_FFI_API void reset_rack(int game_mode);
PUDDIN_FFI_API void get_aim_trajectory(
    float cue_x,
    float cue_y,
    float angle_rad,
    float power,
    float spin_x,
    float spin_y,
    TrajectoryBuffer* out_buffer
);
PUDDIN_FFI_API void execute_strike(float angle_rad, float power, float spin_x, float spin_y);
PUDDIN_FFI_API bool step_simulation(
    float dt,
    BallTransform* out_transforms,
    int max_balls,
    int* out_active_count
);
PUDDIN_FFI_API bool is_simulation_at_rest(void);

// Coach Puddin Wisdom & Emote API
PUDDIN_FFI_API const char* get_coach_puddin_advice(
    int context_type,
    float cut_angle_deg,
    float distance,
    bool was_scratch
);

PUDDIN_FFI_API int evaluate_coach_puddin_emote(
    int context_type,
    int cushions_hit,
    float cut_angle_deg,
    bool was_scratch,
    bool ball_pocketed
);

// Legend System & Multi-Variant Game Rules API
PUDDIN_FFI_API const char* get_puddin_coaching_advice(
    int difficulty_level,
    bool is_snookered,
    bool has_clusters,
    float cue_distance,
    float cut_angle
);

PUDDIN_FFI_API float compute_diamond_bank_aim(
    float cue_diamond,
    float target_diamond,
    float speed,
    float spin_english
);

PUDDIN_FFI_API int evaluate_game_rules(
    int variant,
    int first_ball_hit,
    const int* pocketed_balls,
    int pocketed_count,
    int cushions_after_contact,
    bool scratch,
    int called_ball,
    int called_pocket
);

#ifdef __cplusplus
}
#endif
