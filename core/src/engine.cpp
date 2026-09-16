#include "puddin/core/engine.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace puddin::core {

Engine::Engine(Table table) {
    state_.table = std::move(table);
    state_.clock_micros = 0;
}

void Engine::reset() {
    state_.balls.clear();
    state_.clock_micros = 0;
    next_ball_id_ = 1;
}

void Engine::setup_standard_rack() {
    reset();

    // Cue ball on left (head string)
    add_ball(Vec2{state_.table.width * 0.25, state_.table.height * 0.5}, Vec2{0.0, 0.0});

    // Triangle rack on right (foot spot)
    const double apex_x = state_.table.width * 0.70;
    const double apex_y = state_.table.height * 0.50;
    const double r = 0.028575;
    const double d = 2.0 * r;
    const double row_spacing = d * std::sqrt(3.0) * 0.5;

    for (int row = 0; row < 5; ++row) {
        double col_x = apex_x + row * row_spacing;
        double start_y = apex_y - (row * d * 0.5);
        for (int i = 0; i <= row; ++i) {
            double ball_y = start_y + i * d;
            add_ball(Vec2{col_x, ball_y}, Vec2{0.0, 0.0});
        }
    }
}

uint32_t Engine::add_ball(Vec2 position, Vec2 velocity, double radius, double mass) {
    uint32_t id = next_ball_id_++;
    double spd = velocity.length();
    MotionState mstate = (spd > 1e-4) ? MotionState::Sliding : MotionState::Stationary;
    double trans_spd = (mstate == MotionState::Sliding) ? spd * (5.0 / 7.0) : 0.0;

    state_.balls.push_back(Ball{
        .id = id,
        .position = position,
        .velocity = velocity,
        .radius = radius,
        .mass = mass,
        .motion_state = mstate,
        .transition_speed = trans_spd,
        .is_sunk = false
    });
    return id;
}

bool Engine::apply_impulse(uint32_t ball_id, Vec2 impulse) {
    for (auto& b : state_.balls) {
        if (b.id == ball_id && !b.is_sunk) {
            b.velocity += impulse / b.mass;
            double spd = b.velocity.length();
            if (spd > 1e-4) {
                b.motion_state = MotionState::Sliding;
                b.transition_speed = spd * (5.0 / 7.0);
            }
            return true;
        }
    }
    return false;
}

bool Engine::is_quiescent(double velocity_threshold) const {
    const double thresh_sq = velocity_threshold * velocity_threshold;
    for (const auto& b : state_.balls) {
        if (!b.is_sunk && b.velocity.length_sq() > thresh_sq) {
            return false;
        }
    }
    return true;
}

SimulationState Engine::advance_simulation(const SimulationState& current_state, int64_t dt_micros) {
    if (dt_micros <= 0) return current_state;

    SimulationState state = current_state;
    int64_t remaining_micros = dt_micros;
    constexpr int max_iterations = 256;
    int iteration = 0;

    while (remaining_micros > 0 && iteration++ < max_iterations) {
        double dt_rem = static_cast<double>(remaining_micros) * 1e-6;
        double earliest_time = dt_rem;
        bool event_found = false;
        CollisionEvent earliest_event;

        const size_t ball_count = state.balls.size();

        // 1. Check ball-ball collisions
        for (size_t i = 0; i < ball_count; ++i) {
            const auto& b1 = state.balls[i];
            if (b1.is_sunk) continue;

            for (size_t j = i + 1; j < ball_count; ++j) {
                const auto& b2 = state.balls[j];
                if (b2.is_sunk) continue;

                auto t = time_to_ball_ball_impact(b1, b2);
                if (t && *t < earliest_time) {
                    earliest_time = *t;
                    earliest_event = CollisionEvent{
                        .time = *t,
                        .type = EventType::BallBallCollision,
                        .primary_ball_id = b1.id,
                        .secondary_id = b2.id,
                        .contact_normal = (b2.position - b1.position).normalized()
                    };
                    event_found = true;
                }
            }
        }

        // 2. Check ball-rail collisions
        for (size_t i = 0; i < ball_count; ++i) {
            const auto& b = state.balls[i];
            if (b.is_sunk || b.motion_state == MotionState::Stationary) continue;

            // Segments
            for (const auto& rail : state.table.rails) {
                auto t = time_to_ball_segment_impact(b, rail);
                if (t && *t < earliest_time) {
                    earliest_time = *t;
                    earliest_event = CollisionEvent{
                        .time = *t,
                        .type = EventType::BallRailCollision,
                        .primary_ball_id = b.id,
                        .secondary_id = rail.id,
                        .contact_normal = rail.normal
                    };
                    event_found = true;
                }

                // Rail corner endpoints
                auto tp1 = time_to_ball_point_impact(b, rail.p1);
                if (tp1 && *tp1 < earliest_time) {
                    earliest_time = *tp1;
                    Vec2 norm = (b.position - rail.p1).normalized();
                    earliest_event = CollisionEvent{
                        .time = *tp1,
                        .type = EventType::BallRailCollision,
                        .primary_ball_id = b.id,
                        .secondary_id = rail.id,
                        .contact_normal = norm
                    };
                    event_found = true;
                }

                auto tp2 = time_to_ball_point_impact(b, rail.p2);
                if (tp2 && *tp2 < earliest_time) {
                    earliest_time = *tp2;
                    Vec2 norm = (b.position - rail.p2).normalized();
                    earliest_event = CollisionEvent{
                        .time = *tp2,
                        .type = EventType::BallRailCollision,
                        .primary_ball_id = b.id,
                        .secondary_id = rail.id,
                        .contact_normal = norm
                    };
                    event_found = true;
                }
            }
        }

        // 3. Check motion transitions (sliding->rolling or rolling->stop)
        for (size_t i = 0; i < ball_count; ++i) {
            const auto& b = state.balls[i];
            if (b.is_sunk || b.motion_state == MotionState::Stationary) continue;

            auto t_trans = time_to_motion_transition(b, state.table);
            if (t_trans && *t_trans < earliest_time) {
                earliest_time = *t_trans;
                EventType trans_type = (b.motion_state == MotionState::Sliding)
                    ? EventType::BallSlideToRollTransition
                    : EventType::BallStopTransition;

                earliest_event = CollisionEvent{
                    .time = *t_trans,
                    .type = trans_type,
                    .primary_ball_id = b.id,
                    .secondary_id = 0,
                    .contact_normal = {0.0, 0.0}
                };
                event_found = true;
            }
        }

        // Advance by step duration
        int64_t step_micros = static_cast<int64_t>(std::round(earliest_time * 1e6));
        step_micros = std::clamp(step_micros, int64_t(0), remaining_micros);
        double step_dt = static_cast<double>(step_micros) * 1e-6;

        if (step_dt > 0.0) {
            // 6 standard pocket positions
            const double mid_x = state.table.width * 0.5;
            const Vec2 pockets[6] = {
                {0.06, 0.06},                          // BL corner
                {mid_x, 0.03},                         // B center
                {state.table.width - 0.06, 0.06},      // BR corner
                {0.06, state.table.height - 0.06},     // TL corner
                {mid_x, state.table.height - 0.03},    // T center
                {state.table.width - 0.06, state.table.height - 0.06} // TR corner
            };
            constexpr double pocket_capture_r_sq = 0.085 * 0.085;

            for (auto& b : state.balls) {
                if (b.is_sunk || b.motion_state == MotionState::Stationary) continue;

                Vec2 acc = compute_friction_acceleration(b, state.table);
                b.position += b.velocity * step_dt + acc * (0.5 * step_dt * step_dt);
                b.velocity += acc * step_dt;

                // Stop ball if velocity crossed zero under deceleration
                if (b.velocity.dot(acc) > 0.0) {
                    b.velocity = {0.0, 0.0};
                    b.motion_state = MotionState::Stationary;
                }

                // Pocket capture detection
                for (const auto& p : pockets) {
                    if ((b.position - p).length_sq() < pocket_capture_r_sq) {
                        b.is_sunk = true;
                        b.velocity = {0.0, 0.0};
                        b.motion_state = MotionState::Stationary;
                        break;
                    }
                }
                if (b.is_sunk) continue;

                // Perimeter safeguard against tunneling
                bool near_pocket = (b.position.x < 0.15 || b.position.x > state.table.width - 0.15) ||
                                   (std::abs(b.position.x - mid_x) < 0.08);

                if (b.position.x < b.radius) {
                    if (near_pocket) { b.is_sunk = true; b.velocity = {0, 0}; b.motion_state = MotionState::Stationary; continue; }
                    b.position.x = b.radius;
                    if (b.velocity.x < 0.0) b.velocity.x = -b.velocity.x * state.table.cushion_restitution;
                } else if (b.position.x > state.table.width - b.radius) {
                    if (near_pocket) { b.is_sunk = true; b.velocity = {0, 0}; b.motion_state = MotionState::Stationary; continue; }
                    b.position.x = state.table.width - b.radius;
                    if (b.velocity.x > 0.0) b.velocity.x = -b.velocity.x * state.table.cushion_restitution;
                }

                if (b.position.y < b.radius) {
                    if (near_pocket) { b.is_sunk = true; b.velocity = {0, 0}; b.motion_state = MotionState::Stationary; continue; }
                    b.position.y = b.radius;
                    if (b.velocity.y < 0.0) b.velocity.y = -b.velocity.y * state.table.cushion_restitution;
                } else if (b.position.y > state.table.height - b.radius) {
                    if (near_pocket) { b.is_sunk = true; b.velocity = {0, 0}; b.motion_state = MotionState::Stationary; continue; }
                    b.position.y = state.table.height - b.radius;
                    if (b.velocity.y > 0.0) b.velocity.y = -b.velocity.y * state.table.cushion_restitution;
                }
            }
            state.clock_micros += step_micros;
            remaining_micros -= step_micros;
        }

        // Resolve event if triggered
        if (event_found && step_micros < remaining_micros + 10) {
            if (earliest_event.type == EventType::BallBallCollision) {
                Ball* b1 = nullptr;
                Ball* b2 = nullptr;
                for (auto& b : state.balls) {
                    if (b.id == earliest_event.primary_ball_id) b1 = &b;
                    if (b.id == earliest_event.secondary_id) b2 = &b;
                }
                if (b1 && b2) {
                    resolve_ball_ball_collision(*b1, *b2, state.table.ball_restitution);
                }
            } else if (earliest_event.type == EventType::BallRailCollision) {
                for (auto& b : state.balls) {
                    if (b.id == earliest_event.primary_ball_id) {
                        resolve_ball_rail_collision(
                            b,
                            earliest_event.contact_normal,
                            state.table.cushion_restitution,
                            state.table.cushion_restitution_alpha,
                            state.table.cushion_friction
                        );
                        break;
                    }
                }
            } else if (earliest_event.type == EventType::BallSlideToRollTransition) {
                for (auto& b : state.balls) {
                    if (b.id == earliest_event.primary_ball_id) {
                        b.motion_state = MotionState::Rolling;
                        break;
                    }
                }
            } else if (earliest_event.type == EventType::BallStopTransition) {
                for (auto& b : state.balls) {
                    if (b.id == earliest_event.primary_ball_id) {
                        b.velocity = {0.0, 0.0};
                        b.motion_state = MotionState::Stationary;
                        break;
                    }
                }
            }
        }

        // Quiescence check
        bool any_moving = false;
        for (const auto& b : state.balls) {
            if (!b.is_sunk && b.motion_state != MotionState::Stationary) {
                any_moving = true;
                break;
            }
        }
        if (!any_moving) {
            state.clock_micros += remaining_micros;
            remaining_micros = 0;
            break;
        }
    }

    return state;
}

void Engine::step_micros(int64_t dt_micros) {
    state_ = advance_simulation(state_, dt_micros);
}

void Engine::step(double dt) {
    if (dt <= 0.0) return;
    int64_t micros = static_cast<int64_t>(std::round(dt * 1e6));
    step_micros(micros);
}

} // namespace puddin::core
