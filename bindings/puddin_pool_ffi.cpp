#include "puddin_pool_ffi.h"
#include "puddin/core/engine.hpp"
#include "puddin/core/trajectory.hpp"
#include "puddin/core/wisdom.hpp"
#include "puddin/core/GameRules.h"
#include "puddin/core/DiamondMath.h"
#include "puddin/core/PuddinWisdomAI.h"

#include <memory>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <string>

namespace {

struct Quat {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    float w{1.0f};

    static Quat identity() {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }

    Quat operator*(const Quat& q) const {
        return Quat{
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
        };
    }

    [[nodiscard]] Quat normalized() const {
        float len = std::sqrt(x * x + y * y + z * z + w * w);
        if (len <= 1e-6f) return identity();
        return {x / len, y / len, z / len, w / len};
    }
};

std::unique_ptr<puddin::core::Engine> g_engine;
std::unique_ptr<puddin::core::GameRulesEngine> g_rules;
std::unordered_map<int, Quat> g_ball_orientations;
std::unordered_map<int, puddin::core::Vec2> g_prev_positions;
std::string g_last_legend_advice;

void ensure_engine_initialized() {
    if (!g_engine) {
        g_engine = std::make_unique<puddin::core::Engine>(puddin::core::Table::create_standard_table());
        g_engine->setup_standard_rack();
        for (const auto& b : g_engine->get_balls()) {
            g_ball_orientations[b.id] = Quat::identity();
            g_prev_positions[b.id] = b.position;
        }
    }
    if (!g_rules) {
        g_rules = std::make_unique<puddin::core::GameRulesEngine>(puddin::core::GameVariant::EightBall);
    }
}

} // namespace

extern "C" {

void init_table(float width, float height) {
    puddin::core::Table table = puddin::core::Table::create_standard_table();
    if (width > 0.1f && height > 0.1f) {
        table.width = width;
        table.height = height;
    }
    g_engine = std::make_unique<puddin::core::Engine>(std::move(table));
    g_engine->setup_standard_rack();
    g_ball_orientations.clear();
    g_prev_positions.clear();
    for (const auto& b : g_engine->get_balls()) {
        g_ball_orientations[b.id] = Quat::identity();
        g_prev_positions[b.id] = b.position;
    }
}

void reset_rack(int game_mode) {
    ensure_engine_initialized();
    puddin::core::GameVariant v = static_cast<puddin::core::GameVariant>(game_mode);
    g_rules->set_variant(v);
    g_engine->setup_standard_rack();
    g_ball_orientations.clear();
    g_prev_positions.clear();
    for (const auto& b : g_engine->get_balls()) {
        g_ball_orientations[b.id] = Quat::identity();
        g_prev_positions[b.id] = b.position;
    }
}

void get_aim_trajectory(
    float cue_x,
    float cue_y,
    float angle_rad,
    float power,
    float spin_x,
    float spin_y,
    TrajectoryBuffer* out_buffer
) {
    if (!out_buffer) return;
    ensure_engine_initialized();

    puddin::core::AimRay ray{
        .origin = {cue_x, cue_y},
        .angle_rad = angle_rad,
        .power = power,
        .spin_x = spin_x,
        .spin_y = spin_y
    };

    auto result = puddin::core::predict_trajectory(g_engine->get_state(), ray, 3);

    out_buffer->target_ball_id = result.has_target_hit ? static_cast<int>(result.target_ball_id) : 0;

    out_buffer->cue_count = std::min(static_cast<int>(result.cue_path.size()), 16);
    for (int i = 0; i < out_buffer->cue_count; ++i) {
        out_buffer->cue_points[i].x = static_cast<float>(result.cue_path[i].x);
        out_buffer->cue_points[i].y = static_cast<float>(result.cue_path[i].y);
    }

    out_buffer->target_count = std::min(static_cast<int>(result.target_path.size()), 16);
    for (int i = 0; i < out_buffer->target_count; ++i) {
        out_buffer->target_points[i].x = static_cast<float>(result.target_path[i].x);
        out_buffer->target_points[i].y = static_cast<float>(result.target_path[i].y);
    }
}

void execute_strike(float angle_rad, float power, float spin_x, float spin_y) {
    ensure_engine_initialized();

    float dir_x = std::cos(angle_rad);
    float dir_y = std::sin(angle_rad);

    puddin::core::Vec2 impulse{dir_x * power, dir_y * power};
    g_engine->apply_impulse(1, impulse);
}

bool step_simulation(
    float dt,
    BallTransform* out_transforms,
    int max_balls,
    int* out_active_count
) {
    ensure_engine_initialized();
    g_engine->step(static_cast<double>(dt));

    const auto& balls = g_engine->get_balls();
    int count = std::min(static_cast<int>(balls.size()), max_balls);
    if (out_active_count) {
        *out_active_count = count;
    }

    constexpr double R = 0.028575;

    for (int i = 0; i < count; ++i) {
        const auto& b = balls[i];
        puddin::core::Vec2 prev = g_prev_positions[b.id];
        puddin::core::Vec2 delta = b.position - prev;
        g_prev_positions[b.id] = b.position;

        // Rolling quaternion integration: omega = (-dy/R, dx/R, 0)
        double dist = delta.length();
        if (dist > 1e-7) {
            float angle = static_cast<float>(dist / R);
            float ax = static_cast<float>(-delta.y / dist);
            float ay = static_cast<float>(delta.x / dist);

            float half_sin = std::sin(angle * 0.5f);
            float half_cos = std::cos(angle * 0.5f);

            Quat delta_q{ax * half_sin, ay * half_sin, 0.0f, half_cos};
            g_ball_orientations[b.id] = (delta_q * g_ball_orientations[b.id]).normalized();
        }

        if (out_transforms) {
            Quat q = g_ball_orientations[b.id];
            out_transforms[i] = BallTransform{
                .id = static_cast<int>(b.id),
                .x = static_cast<float>(b.position.x),
                .y = static_cast<float>(b.position.y),
                .rot_quaternion = {q.x, q.y, q.z, q.w},
                .is_pocketed = b.is_sunk
            };
        }
    }

    return !g_engine->is_quiescent();
}

bool is_simulation_at_rest(void) {
    if (!g_engine) return true;
    return g_engine->is_quiescent();
}

const char* get_coach_puddin_advice(
    int context_type,
    float cut_angle_deg,
    float distance,
    bool was_scratch
) {
    puddin::core::ShotContext ctx{
        .type = static_cast<puddin::core::ShotContextType>(context_type),
        .cue_distance = distance,
        .cut_angle_deg = cut_angle_deg,
        .cushions_hit = 0,
        .was_scratch = was_scratch,
        .ball_pocketed = false
    };
    return puddin::core::get_shot_advice(ctx);
}

int evaluate_coach_puddin_emote(
    int context_type,
    int cushions_hit,
    float cut_angle_deg,
    bool was_scratch,
    bool ball_pocketed
) {
    puddin::core::ShotContext ctx{
        .type = static_cast<puddin::core::ShotContextType>(context_type),
        .cue_distance = 0.0,
        .cut_angle_deg = cut_angle_deg,
        .cushions_hit = cushions_hit,
        .was_scratch = was_scratch,
        .ball_pocketed = ball_pocketed
    };
    return static_cast<int>(puddin::core::evaluate_coach_emote(ctx));
}

const char* get_puddin_coaching_advice(
    int difficulty_level,
    bool is_snookered,
    bool has_clusters,
    float cue_distance,
    float cut_angle
) {
    puddin::core::TableSituation situation{
        .is_snookered = is_snookered,
        .has_clusters = has_clusters,
        .cue_distance = cue_distance,
        .cut_angle_deg = cut_angle,
        .is_break_shot = false,
        .remaining_balls = 15
    };
    g_last_legend_advice = puddin::core::PuddinWisdomAI::generate_coaching_advice(situation, difficulty_level);
    return g_last_legend_advice.c_str();
}

float compute_diamond_bank_aim(
    float cue_diamond,
    float target_diamond,
    float speed,
    float spin_english
) {
    auto res = puddin::core::DiamondMath::calculate_bank(
        cue_diamond,
        target_diamond,
        speed,
        spin_english
    );
    return static_cast<float>(res.final_aim_diamond);
}

int evaluate_game_rules(
    int variant,
    int first_ball_hit,
    const int* pocketed_balls,
    int pocketed_count,
    int cushions_after_contact,
    bool scratch,
    int called_ball,
    int called_pocket
) {
    ensure_engine_initialized();
    puddin::core::GameVariant v = static_cast<puddin::core::GameVariant>(variant);
    g_rules->set_variant(v);

    puddin::core::ShotOutcome outcome;
    outcome.first_ball_hit = first_ball_hit;
    outcome.cushions_after_contact = cushions_after_contact;
    outcome.was_scratch = scratch;
    outcome.called_ball = called_ball;
    outcome.called_pocket = called_pocket;
    if (pocketed_balls && pocketed_count > 0) {
        outcome.pocketed_balls.assign(pocketed_balls, pocketed_balls + pocketed_count);
    }

    std::vector<bool> active_balls(16, true);
    for (int b : outcome.pocketed_balls) {
        if (b >= 0 && b < 16) active_balls[b] = false;
    }

    auto res = g_rules->process_shot(outcome, active_balls);
    return static_cast<int>(res);
}

} // extern "C"
