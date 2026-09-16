#include "puddin/core/PhysicsEngine.h"
#include <algorithm>
#include <cmath>

namespace puddin::core {

RigidBodyPhysicsEngine::RigidBodyPhysicsEngine() = default;

Vector3D RigidBodyPhysicsEngine::computeContactPointVelocity(const RigidBall& ball) {
    // Contact point relative surface velocity:
    // v_cp = v - (R * (omega_y * x_hat - omega_x * y_hat))
    return {
        ball.velocity.x - ball.omega.y * BALL_RADIUS,
        ball.velocity.y + ball.omega.x * BALL_RADIUS,
        0.0
    };
}

void RigidBodyPhysicsEngine::strike(RigidBall& ball, const StrikeParameters& params) {
    if (ball.isSunk) return;

    // 1. Squirt / Cue Deflection: Sideways push opposite to applied English
    const double squirtAngleRad = -params.englishX * 0.035; // ~2.0 degrees max squirt
    const double actualAzimuth = params.azimuthRad + squirtAngleRad;

    // 2. Linear Velocity imparted: v0 = (J / m) * [cos(aim_angle), sin(aim_angle)]
    const double v0 = (params.forwardImpulse / BALL_MASS) * std::cos(params.elevationRad);
    ball.velocity.x = v0 * std::cos(actualAzimuth);
    ball.velocity.y = v0 * std::sin(actualAzimuth);
    ball.velocity.z = 0.0;

    // 3. Angular spin imparted at release:
    // omega_x0 = -(5.0 / (2.0 * R)) * (offset_y) * (J / m) * sin(aim_angle);
    // omega_y0 =  (5.0 / (2.0 * R)) * (offset_y) * (J / m) * cos(aim_angle);
    // omega_z0 = -(5.0 / (2.0 * R)) * (offset_x) * (J / m);
    const double impulseNorm = params.forwardImpulse / BALL_MASS;
    const double spinCoeff = 2.5 / BALL_RADIUS; // (5.0 / (2.0 * R))

    ball.omega.x = -spinCoeff * params.englishY * impulseNorm * std::sin(actualAzimuth);
    ball.omega.y =  spinCoeff * params.englishY * impulseNorm * std::cos(actualAzimuth);
    ball.omega.z = -spinCoeff * params.englishX * impulseNorm;

    ball.state = MotionState::Sliding;
}

void RigidBodyPhysicsEngine::stepBall(RigidBall& ball, double dt) {
    if (ball.isSunk || ball.state == MotionState::Stationary) return;

    // 1. Position integration
    ball.position.x += ball.velocity.x * dt;
    ball.position.y += ball.velocity.y * dt;

    // 2. Relative contact point velocity v_cp
    Vector3D v_cp = computeContactPointVelocity(ball);
    const double slip_speed = std::hypot(v_cp.x, v_cp.y);
    const double speed = ball.velocity.length2D();

    const double maxSpinDecay = (2.5 / BALL_RADIUS) * SPIN_FRICTION * GRAVITY * dt;

    if (slip_speed > SLIP_EPSILON) {
        // --- CASE A: SLIDING / SKIDDING STATE ---
        const double dv_cp_max = 3.5 * SLIDING_FRICTION * GRAVITY * dt;
        if (slip_speed <= dv_cp_max) {
            // Analytical felt grip transition into pure rolling
            ball.velocity.x -= (1.0 / 3.5) * v_cp.x;
            ball.velocity.y -= (1.0 / 3.5) * v_cp.y;
            ball.omega.x = -ball.velocity.y / BALL_RADIUS;
            ball.omega.y =  ball.velocity.x / BALL_RADIUS;
            ball.state = MotionState::Rolling;
        } else {
            ball.state = MotionState::Sliding;

            // Kinetic friction opposes relative contact point velocity
            // F_fric = -mu_s * m * g * (v_contact / slip_speed)
            const double fNorm = -SLIDING_FRICTION * GRAVITY;
            const double ax = fNorm * (v_cp.x / slip_speed);
            const double ay = fNorm * (v_cp.y / slip_speed);

            // Angular acceleration:
            // alpha_x = (F_fric_y * R) / I = (2.5 / R) * ay
            // alpha_y = -(F_fric_x * R) / I = -(2.5 / R) * ax
            const double alphaCoeff = 2.5 / BALL_RADIUS;
            const double alpha_x =  alphaCoeff * ay;
            const double alpha_y = -alphaCoeff * ax;

            // Integrate linear and angular velocities
            ball.velocity.x += ax * dt;
            ball.velocity.y += ay * dt;

            ball.omega.x += alpha_x * dt;
            ball.omega.y += alpha_y * dt;
        }

        // Gyroscopic vertical spin decay
        if (ball.omega.z > 0.0) {
            ball.omega.z = std::max(0.0, ball.omega.z - maxSpinDecay);
        } else if (ball.omega.z < 0.0) {
            ball.omega.z = std::min(0.0, ball.omega.z + maxSpinDecay);
        }

        // Swerve/Masse curvature when vertical spin omega_z is present during slide
        if (std::abs(ball.omega.z) > 1.0 && speed > 0.05) {
            const double swerveForce = 0.04 * (ball.omega.z / 100.0) * SLIDING_FRICTION * GRAVITY;
            ball.velocity.x += -ball.velocity.y / speed * swerveForce * dt;
            ball.velocity.y +=  ball.velocity.x / speed * swerveForce * dt;
        }
    } else {
        // --- CASE B: PURE ROLLING STATE ---
        ball.state = MotionState::Rolling;

        // Lock rotation directly to travel direction to eliminate micro-slippage
        ball.omega.x = -ball.velocity.y / BALL_RADIUS;
        ball.omega.y =  ball.velocity.x / BALL_RADIUS;

        if (speed > VELOCITY_REST) {
            // Apply rolling resistance with cloth nap progressive decay at low velocity
            const double napFactor = (speed < 0.20) ? (1.0 + 0.05 / (speed + 0.015)) : 1.0;
            const double decel = ROLLING_FRICTION * GRAVITY * napFactor;
            const double newSpeed = std::max(0.0, speed - decel * dt);
            ball.velocity.x = (ball.velocity.x / speed) * newSpeed;
            ball.velocity.y = (ball.velocity.y / speed) * newSpeed;

            ball.omega.x = -ball.velocity.y / BALL_RADIUS;
            ball.omega.y =  ball.velocity.x / BALL_RADIUS;
        } else {
            // Dead stop: snap to zero to prevent endless creeping
            ball.velocity = {0.0, 0.0, 0.0};
            ball.omega = {0.0, 0.0, 0.0};
            ball.state = MotionState::Stationary;
        }

        // Apply spin decay to vertical side English (omega.z)
        if (ball.omega.z > 0.0) {
            ball.omega.z = std::max(0.0, ball.omega.z - maxSpinDecay);
        } else if (ball.omega.z < 0.0) {
            ball.omega.z = std::min(0.0, ball.omega.z + maxSpinDecay);
        }
    }
}

void RigidBodyPhysicsEngine::resolveBallCollision(RigidBall& b1, RigidBall& b2) {
    if (b1.isSunk || b2.isSunk) return;

    const double dx = b2.position.x - b1.position.x;
    const double dy = b2.position.y - b1.position.y;
    const double dist = std::hypot(dx, dy);
    const double minDist = BALL_RADIUS * 2.0;

    if (dist <= minDist && dist > 1e-6) {
        // Calculate Normal Vector: n = (pos2 - pos1) / |pos2 - pos1|
        const double nx = dx / dist;
        const double ny = dy / dist;

        // Calculate Tangent Vector: t = [-n.y, n.x]
        const double tx = -ny;
        const double ty = nx;

        // Position separation
        const double overlap = minDist - dist;
        b1.position.x -= nx * overlap * 0.5;
        b1.position.y -= ny * overlap * 0.5;
        b2.position.x += nx * overlap * 0.5;
        b2.position.y += ny * overlap * 0.5;

        // Relative velocity along normal: v_rel_n = dot(v1 - v2, n)
        const double rvx = b1.velocity.x - b2.velocity.x;
        const double rvy = b1.velocity.y - b2.velocity.y;
        const double v_rel_n = rvx * nx + rvy * ny;

        if (v_rel_n <= 0.0) return; // Balls moving apart

        // Compute Normal Collision Impulse:
        // J_n = -(1.0 + e_ball) * v_rel_n / (1.0/m + 1.0/m) = -0.5 * (1.0 + e_ball) * m * v_rel_n
        const double J_n = -0.5 * (1.0 + RESTITUTION) * BALL_MASS * v_rel_n;

        // Cut-Induced Throw (Tangential Impulse):
        // Relative tangential surface velocity including side spin:
        // v_rel_t = dot(v1 - v2, t) - R * (omega1.z + omega2.z)
        const double v_rel_t = (rvx * tx + rvy * ty) - BALL_RADIUS * (b1.omega.z + b2.omega.z);
        const double max_throw = BALL_FRICTION * std::abs(J_n);
        const double j_t_uncapped = -0.5 * BALL_MASS * v_rel_t;
        const double J_t = std::clamp(j_t_uncapped, -max_throw, max_throw);

        // Apply velocities immediately:
        // For authentic billiards 90-degree tangent line separation on stun shots:
        // The cue ball's normal component of velocity along the line of centers is completely transferred.
        const double v1_n_orig = b1.velocity.x * nx + b1.velocity.y * ny;
        const double v1_t_orig = b1.velocity.x * tx + b1.velocity.y * ty;
        const double v2_n_orig = b2.velocity.x * nx + b2.velocity.y * ny;
        const double v2_t_orig = b2.velocity.x * tx + b2.velocity.y * ty;

        // b1 normal component stops (transfers to b2), b1 moves along tangent line t plus throw
        const double v1_n_new = v2_n_orig; // cue ball normal motion fully arrested relative to b2
        const double v1_t_new = v1_t_orig + (J_t / BALL_MASS);
        b1.velocity.x = v1_n_new * nx + v1_t_new * tx;
        b1.velocity.y = v1_n_new * ny + v1_t_new * ty;

        // b2 departs along normal n with restitution, plus throw
        const double v2_n_new = v2_n_orig - (J_n / BALL_MASS);
        const double v2_t_new = v2_t_orig - (J_t / BALL_MASS);
        b2.velocity.x = v2_n_new * nx + v2_t_new * tx;
        b2.velocity.y = v2_n_new * ny + v2_t_new * ty;

        // Tangential torque spin impulse (gearing side spin exchange: action-reaction opposite)
        const double spinImpulse = (2.5 / BALL_RADIUS) * (J_t / BALL_MASS);
        b1.omega.z += spinImpulse;
        b2.omega.z -= spinImpulse;

        // Vertical rolling spin / counter-spin transfer along normal of impact
        const double vNormCueSpin = -b1.omega.x * ny + b1.omega.y * nx;
        const double counterSpinTransfer = vNormCueSpin * 0.12;
        b2.omega.x += counterSpinTransfer * ny;
        b2.omega.y -= counterSpinTransfer * nx;

        b1.state = MotionState::Sliding;
        b2.state = MotionState::Sliding;
    }
}

} // namespace puddin::core
