#pragma once

#include <cstdint>
#include <cmath>
#include <vector>
#include <string>

namespace puddin::core {

struct Vec2 {
    double x{0.0};
    double y{0.0};

    constexpr Vec2() = default;
    constexpr Vec2(double x_, double y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(double s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(double s) const { return {x / s, y / s}; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(double s) { x *= s; y *= s; return *this; }

    [[nodiscard]] constexpr double dot(const Vec2& o) const { return x * o.x + y * o.y; }
    [[nodiscard]] constexpr double cross(const Vec2& o) const { return x * o.y - y * o.x; }
    [[nodiscard]] double length_sq() const { return x * x + y * y; }
    [[nodiscard]] double length() const { return std::sqrt(length_sq()); }

    [[nodiscard]] Vec2 normalized() const {
        double len = length();
        if (len <= 1e-12) return {0.0, 0.0};
        return {x / len, y / len};
    }
};

enum class MotionState : uint8_t {
    Stationary = 0,
    Sliding = 1,
    Rolling = 2
};

struct Ball {
    uint32_t id{0};
    Vec2 position{0.0, 0.0};
    Vec2 velocity{0.0, 0.0};
    double radius{0.028575};
    double mass{0.17};
    MotionState motion_state{MotionState::Stationary};
    double transition_speed{0.0}; // Velocity below which sliding converts to natural roll
    bool is_sunk{false};

    [[nodiscard]] double speed() const { return velocity.length(); }
};

struct RailSegment {
    uint32_t id{0};
    Vec2 p1{0.0, 0.0};
    Vec2 p2{0.0, 0.0};
    Vec2 normal{0.0, 1.0}; // Inward-facing unit normal
    Vec2 tangent{1.0, 0.0};
    double length{0.0};

    static RailSegment from_points(uint32_t id, Vec2 p1, Vec2 p2, Vec2 inward_hint) {
        Vec2 diff = p2 - p1;
        double len = diff.length();
        Vec2 tang = (len > 1e-9) ? diff / len : Vec2{1.0, 0.0};
        // Perpendicular vector (-tang.y, tang.x)
        Vec2 norm{-tang.y, tang.x};
        if (norm.dot(inward_hint) < 0.0) {
            norm = norm * -1.0;
        }
        return RailSegment{
            .id = id,
            .p1 = p1,
            .p2 = p2,
            .normal = norm,
            .tangent = tang,
            .length = len
        };
    }
};

struct Table {
    double width{2.24};
    double height{1.12};
    double cushion_restitution{0.85};
    double cushion_restitution_alpha{0.08}; // Non-linear restitution damping coefficient
    double cushion_friction{0.20};          // Tangential friction coefficient
    double ball_restitution{0.96};
    double sliding_friction{0.20};          // mu_s
    double rolling_friction{0.015};         // mu_r
    std::vector<RailSegment> rails;

    static Table create_standard_table();
};

struct SimulationState {
    int64_t clock_micros{0};
    std::vector<Ball> balls;
    Table table;
};

} // namespace puddin::core
