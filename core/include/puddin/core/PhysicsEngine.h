#pragma once

#include <cmath>
#include <vector>

namespace puddin::core {

struct Vector3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    Vector3D operator+(const Vector3D& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3D operator-(const Vector3D& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3D operator*(double s) const { return {x * s, y * s, z * s}; }
    double length() const { return std::sqrt(x * x + y * y + z * z); }
    double length2D() const { return std::sqrt(x * x + y * y); }
};

enum class MotionState {
    Stationary,
    Sliding,
    Rolling
};

struct RigidBall {
    int id{0};
    Vector3D position; // x, y (table surface), z = R
    Vector3D velocity; // vx, vy, vz
    Vector3D omega;    // omega_x, omega_y, omega_z (rad/s)
    MotionState state{MotionState::Stationary};
    bool isSunk{false};
};

struct StrikeParameters {
    double forwardImpulse{0.0}; // J in N*s (or equivalent velocity impulse)
    double azimuthRad{0.0};      // Aim angle theta (radians)
    double elevationRad{0.0436}; // Elevation angle psi (radians, default ~2.5 deg)
    double englishX{0.0};        // a: -1.0 (left) to +1.0 (right)
    double englishY{0.0};        // b: -1.0 (draw/bottom) to +1.0 (follow/top)
};

class RigidBodyPhysicsEngine {
public:
    // Physical Constants
    static constexpr double GRAVITY = 9.80665;       // m/s^2
    static constexpr double BALL_MASS = 0.170097;    // kg (6.0 oz)
    static constexpr double BALL_RADIUS = 0.028575;  // m (diameter 2.25 in = 0.05715 m)
    static constexpr double MOMENT_INERTIA = (2.0 / 5.0) * BALL_MASS * BALL_RADIUS * BALL_RADIUS; // (2/5) m R^2
    static constexpr double SLIDING_FRICTION = 0.270;// mu_s (grippy worsted cloth, transitions in 0.45m-0.65m)
    static constexpr double ROLLING_FRICTION = 0.145;// mu_r (slate roll decay, stops 2.5m/s in 2.0m-2.4m)
    static constexpr double VELOCITY_REST = 0.003;   // v_rest = 0.003 m/s (snap to dead stop)
    static constexpr double SLIP_EPSILON = 0.005;    // v_slip_epsilon = 0.005 m/s
    static constexpr double SPIN_FRICTION = 0.025;   // mu_spin
    static constexpr double RESTITUTION = 0.96;      // Ball-ball restitution e_ball (0.95 - 0.98)
    static constexpr double BALL_FRICTION = 0.04;    // mu_ball (Cut surface throw friction)

    RigidBodyPhysicsEngine();

    // Execute comprehensive cue strike with 3D spin, squirt deflection, and elevation
    void strike(RigidBall& ball, const StrikeParameters& params);

    // Step rigid body dynamics over time dt
    void stepBall(RigidBall& ball, double dt);

    // Compute surface contact point relative velocity v_cp
    static Vector3D computeContactPointVelocity(const RigidBall& ball);

    // Resolve 3D elastic collision between two spherical balls
    static void resolveBallCollision(RigidBall& b1, RigidBall& b2);
};

} // namespace puddin::core
