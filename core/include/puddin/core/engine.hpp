#pragma once

#include "puddin/core/types.hpp"
#include "puddin/core/collision.hpp"
#include <vector>

namespace puddin::core {

class Engine {
public:
    explicit Engine(Table table = Table::create_standard_table());

    // Pure deterministic advancement function
    static SimulationState advance_simulation(const SimulationState& state, int64_t dt_micros);

    void reset();
    void setup_standard_rack();
    uint32_t add_ball(
        Vec2 position,
        Vec2 velocity = {0.0, 0.0},
        double radius = 0.028575,
        double mass = 0.17
    );
    bool apply_impulse(uint32_t ball_id, Vec2 impulse);

    void step_micros(int64_t dt_micros);
    void step(double dt);

    [[nodiscard]] const Table& get_table() const { return state_.table; }
    [[nodiscard]] const std::vector<Ball>& get_balls() const { return state_.balls; }
    [[nodiscard]] int64_t get_clock_micros() const { return state_.clock_micros; }
    [[nodiscard]] const SimulationState& get_state() const { return state_; }
    void set_state(SimulationState state) { state_ = std::move(state); }

    [[nodiscard]] bool is_quiescent(double velocity_threshold = 1e-3) const;

private:
    SimulationState state_;
    uint32_t next_ball_id_{1};
};

} // namespace puddin::core
