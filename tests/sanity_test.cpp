#include "puddin/core/engine.hpp"
#include "puddin/core/collision.hpp"
#include "puddin/core/trajectory.hpp"
#include "puddin/bindings/puddin_c_abi.h"
#include "puddin/view/cli_view.hpp"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <chrono>

void test_ball_ball_analytical_collision() {
    std::cout << "[TEST] 1. Ball-to-ball analytical collision...\n";
    puddin::core::Ball b1{
        .id = 1,
        .position = {0.5, 0.5},
        .velocity = {1.0, 0.0},
        .radius = 0.028575,
        .mass = 0.17,
        .motion_state = puddin::core::MotionState::Sliding
    };
    puddin::core::Ball b2{
        .id = 2,
        .position = {0.8, 0.5},
        .velocity = {0.0, 0.0},
        .radius = 0.028575,
        .mass = 0.17,
        .motion_state = puddin::core::MotionState::Stationary
    };

    auto t_opt = puddin::core::time_to_ball_ball_impact(b1, b2);
    assert(t_opt.has_value() && "Expected analytical collision time");
    double expected_t = (0.3 - (b1.radius + b2.radius)) / 1.0;
    assert(std::abs(*t_opt - expected_t) < 1e-5 && "Collision time mismatch");

    b1.position += b1.velocity * (*t_opt);
    b2.position += b2.velocity * (*t_opt);

    puddin::core::Vec2 initial_momentum = b1.velocity * b1.mass + b2.velocity * b2.mass;
    puddin::core::resolve_ball_ball_collision(b1, b2, 1.0);
    puddin::core::Vec2 final_momentum = b1.velocity * b1.mass + b2.velocity * b2.mass;

    assert(std::abs(initial_momentum.x - final_momentum.x) < 1e-9 && "Momentum X not conserved");
    assert(std::abs(initial_momentum.y - final_momentum.y) < 1e-9 && "Momentum Y not conserved");
    assert(std::abs(b1.velocity.x) < 1e-6 && "Cue ball did not transfer full head-on momentum");
    assert(std::abs(b2.velocity.x - 1.0) < 1e-6 && "Target ball did not receive head-on momentum");
    std::cout << "       -> PASS: Head-on collision t_coll=" << *t_opt << "s, momentum conserved.\n";
}

void test_segmented_cushion_reflection() {
    std::cout << "[TEST] 2. Segmented cushion reflection with friction & non-linear restitution...\n";
    auto table = puddin::core::Table::create_standard_table();
    assert(!table.rails.empty() && "Standard table should have segmented rails");

    puddin::core::Ball b{
        .id = 1,
        .position = {0.5, 1.0},
        .velocity = {1.0, 1.0},
        .radius = 0.028575,
        .mass = 0.17,
        .motion_state = puddin::core::MotionState::Sliding
    };

    double earliest_t = 1e9;
    const puddin::core::RailSegment* hit_rail = nullptr;
    for (const auto& rail : table.rails) {
        auto t = puddin::core::time_to_ball_segment_impact(b, rail);
        if (t && *t < earliest_t) {
            earliest_t = *t;
            hit_rail = &rail;
        }
    }
    assert(hit_rail != nullptr && "Expected cushion impact");
    assert(hit_rail->normal.y < 0.0 && "Top cushion inward normal must point -Y");

    b.position += b.velocity * earliest_t;
    puddin::core::Vec2 pre_v = b.velocity;
    puddin::core::resolve_ball_rail_collision(
        b,
        hit_rail->normal,
        table.cushion_restitution,
        table.cushion_restitution_alpha,
        table.cushion_friction
    );

    assert(b.velocity.y < 0.0 && "Cushion did not reflect Y velocity component inward");
    assert(std::abs(b.velocity.y) < pre_v.y * table.cushion_restitution && "Non-linear restitution damping failed");
    assert(b.velocity.x < pre_v.x && "Tangential velocity not damped by cushion friction");
    std::cout << "       -> PASS: Reflected v=(" << b.velocity.x << ", " << b.velocity.y << ") from normal ("
              << hit_rail->normal.x << ", " << hit_rail->normal.y << ")\n";
}

void test_dual_friction_states() {
    std::cout << "[TEST] 3. Dual friction states (Sliding -> Rolling -> Stop)...\n";
    puddin::core::Engine engine;
    engine.reset();

    uint32_t id = engine.add_ball({0.5, 0.56}, {0.25, 0.0});
    const auto& balls = engine.get_balls();
    assert(balls[0].motion_state == puddin::core::MotionState::Sliding);
    double expected_trans_speed = 0.25 * (5.0 / 7.0);
    assert(std::abs(balls[0].transition_speed - expected_trans_speed) < 1e-9);

    int steps = 0;
    while (engine.get_balls()[0].motion_state == puddin::core::MotionState::Sliding && steps < 1000) {
        engine.step_micros(10000);
        steps++;
    }
    assert(engine.get_balls()[0].motion_state == puddin::core::MotionState::Rolling && "Ball failed to transition to rolling");
    std::cout << "       -> PASS: Transitioned from Sliding to Rolling at step " << steps
              << ", speed=" << engine.get_balls()[0].speed() << " m/s (expected ~" << expected_trans_speed << ")\n";

    while (!engine.is_quiescent() && steps < 5000) {
        engine.step_micros(20000);
        steps++;
    }
    assert(engine.get_balls()[0].motion_state == puddin::core::MotionState::Stationary && "Ball failed to reach stationary state");
    assert(engine.get_balls()[0].speed() == 0.0 && "Stationary ball has non-zero speed");
    std::cout << "       -> PASS: Natural rolling deceleration completed to stationary at step " << steps << "\n";
}

void test_pure_advance_simulation_determinism() {
    std::cout << "[TEST] 4. Pure advance_simulation determinism & zero float drift...\n";

    puddin::core::Table table = puddin::core::Table::create_standard_table();
    puddin::core::SimulationState s1{
        .clock_micros = 0,
        .balls = {
            puddin::core::Ball{
                .id = 1,
                .position = {0.5, 0.56},
                .velocity = {1.5, 0.1},
                .radius = 0.028575,
                .mass = 0.17,
                .motion_state = puddin::core::MotionState::Sliding,
                .transition_speed = 1.5 * (5.0 / 7.0)
            },
            puddin::core::Ball{
                .id = 2,
                .position = {1.0, 0.59},
                .velocity = {0.0, 0.0},
                .radius = 0.028575,
                .mass = 0.17,
                .motion_state = puddin::core::MotionState::Stationary,
                .transition_speed = 0.0
            }
        },
        .table = table
    };

    puddin::core::SimulationState s2 = s1;

    constexpr int64_t step_dt = 16666;
    for (int i = 0; i < 50; ++i) {
        s1 = puddin::core::Engine::advance_simulation(s1, step_dt);
        s2 = puddin::core::Engine::advance_simulation(s2, step_dt);
    }

    assert(s1.clock_micros == s2.clock_micros && "Collision clock mismatch");
    assert(s1.clock_micros == 50 * step_dt && "Collision clock didn't advance accurately");
    for (size_t i = 0; i < s1.balls.size(); ++i) {
        assert(s1.balls[i].position.x == s2.balls[i].position.x && "Position X drift");
        assert(s1.balls[i].position.y == s2.balls[i].position.y && "Position Y drift");
        assert(s1.balls[i].velocity.x == s2.balls[i].velocity.x && "Velocity X drift");
        assert(s1.balls[i].velocity.y == s2.balls[i].velocity.y && "Velocity Y drift");
    }
    std::cout << "       -> PASS: Exact bit-for-bit determinism verified over 50 steps ("
              << s1.clock_micros << " microseconds).\n";
}

void test_full_rack_and_view() {
    std::cout << "[TEST] 5. Full 16-ball rack simulation & decoupled CLI view...\n";
    PuddinTableConfig config = puddin_table_config_default();
    PuddinEngineHandle* handle = puddin_engine_create(&config);
    puddin_engine_setup_standard_rack(handle);

    assert(puddin_engine_get_ball_count(handle) == 16);
    puddin_engine_apply_impulse(handle, 1, 0.60, 0.015);

    for (int i = 0; i < 60; ++i) {
        puddin_engine_step(handle, 0.016);
    }

    std::vector<PuddinBallState> states(16);
    puddin_engine_get_ball_states(handle, states.data(), 16);

    puddin::core::Table core_table = puddin::core::Table::create_standard_table();
    std::vector<puddin::core::Ball> core_balls;
    for (const auto& s : states) {
        core_balls.push_back(puddin::core::Ball{
            .id = s.id,
            .position = {s.position.x, s.position.y},
            .velocity = {s.velocity.x, s.velocity.y},
            .radius = s.radius,
            .mass = s.mass,
            .motion_state = static_cast<puddin::core::MotionState>(s.motion_state),
            .transition_speed = s.transition_speed,
            .is_sunk = s.is_sunk
        });
    }

    puddin::view::CliView cli_view(65, 16);
    std::cout << "\n" << cli_view.render_to_string(core_table, core_balls) << "\n";

    puddin_engine_destroy(handle);
    std::cout << "       -> PASS: 16-ball break simulation executed cleanly.\n";
}

void test_trajectory_predictor_and_ghost_ball() {
    std::cout << "[TEST] 6. Ghost-ball, multi-bounce raycast & latency benchmark (< 0.1ms)...\n";
    PuddinTableConfig config = puddin_table_config_default();
    PuddinEngineHandle* handle = puddin_engine_create(&config);
    puddin_engine_reset(handle);

    // Cue ball at (0.50, 0.56)
    puddin_engine_add_ball(handle, 0.50, 0.56, 0.0, 0.0, 0.028575, 0.17);
    // Target ball (object ball 2) at (1.20, 0.62)
    puddin_engine_add_ball(handle, 1.20, 0.62, 0.0, 0.0, 0.028575, 0.17);

    // Aim toward target ball with slight cut angle
    double dx = 1.20 - 0.50;
    double dy = 0.62 - 0.56;
    double aim_angle = std::atan2(dy, dx) - 0.03; // Slight cut angle

    // Run single trajectory preview via C-ABI
    PuddinTrajectoryPreview preview = puddin_engine_predict_trajectory(
        handle,
        aim_angle,
        2.5,   // power
        0.2,   // right English spin_x
        0.4,   // follow spin_y
        3      // max 3 bounces
    );

    // 1. Assert raycast found target ball
    assert(preview.has_target_hit && "Predictor missed target ball");
    assert(preview.target_ball_id == 2 && "Wrong target ball hit");
    std::cout << "       -> PASS: Hit target ball " << preview.target_ball_id
              << " with cut angle " << preview.cut_angle_deg << " deg.\n";

    // 2. Assert ghost ball distance is exactly 2 * radius
    constexpr double R = 0.028575;
    double ghost_dist = std::sqrt(
        std::pow(preview.ghost_ball_position.x - 1.20, 2) +
        std::pow(preview.ghost_ball_position.y - 0.62, 2)
    );
    assert(std::abs(ghost_dist - 2.0 * R) < 1e-6 && "Ghost ball not at 2*R distance");
    std::cout << "       -> PASS: Ghost ball position (" << preview.ghost_ball_position.x
              << ", " << preview.ghost_ball_position.y << ") dist=" << ghost_dist << " (2*R=" << 2*R << ").\n";

    // 3. Assert contact normal & tangent orthogonality
    double dot_prod = preview.contact_normal.x * preview.contact_tangent.x +
                      preview.contact_normal.y * preview.contact_tangent.y;
    assert(std::abs(dot_prod) < 1e-9 && "Contact normal and tangent are not orthogonal");
    std::cout << "       -> PASS: Normal (" << preview.contact_normal.x << ", " << preview.contact_normal.y
              << ") and Tangent (" << preview.contact_tangent.x << ", " << preview.contact_tangent.y
              << ") dot=" << dot_prod << " (orthogonal 90 deg).\n";

    // 4. Assert multi-bounce polyline points are populated and within table bounds
    assert(preview.cue_point_count >= 2 && "Cue path points insufficient");
    assert(preview.target_point_count >= 2 && "Target path points insufficient");
    for (uint32_t i = 0; i < preview.cue_point_count; ++i) {
        assert(preview.cue_points[i].x >= 0.0 && preview.cue_points[i].x <= config.width);
        assert(preview.cue_points[i].y >= 0.0 && preview.cue_points[i].y <= config.height);
    }
    for (uint32_t i = 0; i < preview.target_point_count; ++i) {
        assert(preview.target_points[i].x >= 0.0 && preview.target_points[i].x <= config.width);
        assert(preview.target_points[i].y >= 0.0 && preview.target_points[i].y <= config.height);
    }
    std::cout << "       -> PASS: Multi-bounce cue points (" << preview.cue_point_count
              << ") and target points (" << preview.target_point_count << ") bounded in table.\n";

    // 5. Latency benchmark over 10,000 runs (< 0.1ms = 100 microseconds)
    constexpr int BENCH_ITERATIONS = 10000;
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCH_ITERATIONS; ++i) {
        volatile auto p = puddin_engine_predict_trajectory(handle, aim_angle, 2.0, 0.0, 0.0, 3);
        (void)p;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    double total_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();
    double avg_us = total_us / BENCH_ITERATIONS;

    std::cout << "       -> BENCHMARK: 10,000 iterations completed in " << total_us / 1000.0 << " ms.\n";
    std::cout << "       -> AVERAGE TIME: " << avg_us << " microseconds per prediction (budget: 100 us = 0.1 ms).\n";
    assert(avg_us < 100.0 && "Trajectory predictor exceeded 0.1ms performance budget");
    std::cout << "       -> PASS: Execution latency < 0.1ms validated!\n";

    // 6. Visual CLI Preview render with Ghost Ball ('G') and path dots ('.')
    puddin::core::Table core_table = puddin::core::Table::create_standard_table();
    std::vector<puddin::core::Ball> balls{
        {.id = 1, .position = {0.50, 0.56}, .radius = R},
        {.id = 2, .position = {1.20, 0.62}, .radius = R}
    };
    std::vector<puddin::core::Vec2> preview_pts;
    for (uint32_t i = 0; i < preview.cue_point_count; ++i) {
        preview_pts.push_back({preview.cue_points[i].x, preview.cue_points[i].y});
    }
    for (uint32_t i = 0; i < preview.target_point_count; ++i) {
        preview_pts.push_back({preview.target_points[i].x, preview.target_points[i].y});
    }

    puddin::view::CliView cli_view(65, 16);
    std::cout << "\n[Trajectory & Ghost Ball 'G' CLI Preview]:\n";
    cli_view.render(core_table, balls, preview_pts, {preview.ghost_ball_position.x, preview.ghost_ball_position.y});

    puddin_engine_destroy(handle);
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " Puddin Pool: Full Physics & Predictor Test Suite\n";
    std::cout << "========================================================\n";

    test_ball_ball_analytical_collision();
    test_segmented_cushion_reflection();
    test_dual_friction_states();
    test_pure_advance_simulation_determinism();
    test_full_rack_and_view();
    test_trajectory_predictor_and_ghost_ball();

    std::cout << "========================================================\n";
    std::cout << " ALL 6 TESTS PASSED (PREDICTOR VERIFIED < 0.1ms)\n";
    std::cout << "========================================================\n";
    return 0;
}
