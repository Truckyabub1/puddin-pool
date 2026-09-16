#include "puddin/core/PhysicsEngine.h"
#include "puddin/core/ValleyTableGeometry.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

using namespace puddin::core;

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "   AUTHENTIC BILLIARDS IMPACT & FRICTION TEST SUITE       " << std::endl;
    std::cout << "==========================================================" << std::endl;

    RigidBodyPhysicsEngine engine;
    ValleyTableGeometry table;
    const double dt = 0.001; // 1 millisecond physics integration step
    const double R = RigidBodyPhysicsEngine::BALL_RADIUS;

    // ----------------------------------------------------
    // TEST 1: STUN SHOT TEST (90.0° ± 0.25° SEPARATION)
    // ----------------------------------------------------
    std::cout << "\n[TEST 1] Testing Stun Shot 90-Degree Tangent Departure..." << std::endl;
    {
        // Direct half-ball cut: cut angle is 30 degrees (pi/6)
        const double cutAngleRad = M_PI / 6.0; // 30 degrees
        const double nx = std::cos(cutAngleRad);
        const double ny = std::sin(cutAngleRad);

        RigidBall cueBall;
        cueBall.id = 0;
        cueBall.position = {0.50, 0.50, R};

        RigidBall objectBall;
        objectBall.id = 1;
        // Position object ball at distance ~2R along line of centers n for instant contact
        objectBall.position = {cueBall.position.x + 1.999 * R * nx, cueBall.position.y + 1.999 * R * ny, R};

        // Strike with zero vertical tip offset (stun shot) along azimuth 0.0 (+X)
        StrikeParameters stunParams{
            .forwardImpulse = 0.50,
            .azimuthRad = 0.0,
            .elevationRad = 0.0,
            .englishX = 0.0,
            .englishY = 0.0 // Zero vertical offset (pure stun)
        };
        engine.strike(cueBall, stunParams);

        assert(cueBall.velocity.x > 0.0);
        assert(std::abs(cueBall.omega.y) < 1e-6);

        // Resolve ball-to-ball impact
        RigidBodyPhysicsEngine::resolveBallCollision(cueBall, objectBall);

        // Object ball must depart along the line of centers (path of object ball)
        const double objSpeed = std::hypot(objectBall.velocity.x, objectBall.velocity.y);
        assert(objSpeed > 0.0);

        // Calculate cue ball departure angle relative to object ball nominal path n
        const double cueSpeed = std::hypot(cueBall.velocity.x, cueBall.velocity.y);
        assert(cueSpeed > 0.0);

        const double dotPath = (cueBall.velocity.x * nx + cueBall.velocity.y * ny) / cueSpeed;
        const double angleDeg = std::acos(std::clamp(dotPath, -1.0, 1.0)) * (180.0 / M_PI);

        std::cout << "         -> Object ball speed: " << objSpeed << " m/s" << std::endl;
        std::cout << "         -> Cue ball departure speed: " << cueSpeed << " m/s" << std::endl;
        std::cout << "         -> Measured departure angle to object path: " << angleDeg << " deg" << std::endl;

        assert(std::abs(angleDeg - 90.0) <= 0.25);
        std::cout << "         -> Cue ball departed at exactly 90.0 deg (within +-0.25 deg)! PASS." << std::endl;
    }

    // ----------------------------------------------------
    // TEST 2: DRAW SHOT TEST (-0.6 VERTICAL OFFSET REVERSAL)
    // ----------------------------------------------------
    std::cout << "\n[TEST 2] Testing Draw Shot (-0.6 English) Post-Impact Curve Reversal..." << std::endl;
    {
        RigidBall cueBall;
        cueBall.id = 0;
        cueBall.position = {0.35, 0.50, R};

        RigidBall objectBall;
        objectBall.id = 1;
        objectBall.position = {0.55, 0.50, R}; // Distance 0.20 m ahead on X axis

        // Strike cue ball with -0.6 vertical tip offset (backspin/draw) along azimuth 0.0
        StrikeParameters drawParams{
            .forwardImpulse = 0.55,
            .azimuthRad = 0.0,
            .elevationRad = 0.02,
            .englishX = 0.0,
            .englishY = -0.6 // -0.6 vertical offset
        };
        engine.strike(cueBall, drawParams);

        assert(cueBall.velocity.x > 0.0);
        assert(cueBall.omega.y < 0.0); // Backspin

        const double initialX = cueBall.position.x;
        bool collided = false;
        double impactPointX = 0.0;
        bool reversedVelocity = false;
        double minVxPostImpact = 0.0;

        // Step simulation through pre-collision slide, impact, and post-collision draw
        for (int step = 0; step < 1500; ++step) {
            engine.stepBall(cueBall, dt);
            engine.stepBall(objectBall, dt);

            if (!collided) {
                const double dist = std::hypot(objectBall.position.x - cueBall.position.x,
                                               objectBall.position.y - cueBall.position.y);
                if (dist <= 2.0 * R) {
                    RigidBodyPhysicsEngine::resolveBallCollision(cueBall, objectBall);
                    collided = true;
                    impactPointX = cueBall.position.x;
                    // Cue ball linear velocity arrested, retaining backspin
                    assert(cueBall.omega.y < 0.0);
                }
            } else {
                // Post-collision: felt sliding friction acts on backspin, pulling cue ball backwards
                if (cueBall.velocity.x < 0.0) {
                    reversedVelocity = true;
                    if (cueBall.velocity.x < minVxPostImpact) {
                        minVxPostImpact = cueBall.velocity.x;
                    }
                }
            }
        }

        assert(collided);
        assert(reversedVelocity);
        assert(minVxPostImpact < -0.05); // Substantial backward draw velocity
        // Verify cue ball curved backward toward break point
        assert(cueBall.position.x < impactPointX);

        std::cout << "         -> Collided at x = " << impactPointX << " m" << std::endl;
        std::cout << "         -> Post-impact peak draw reversal speed: " << minVxPostImpact << " m/s" << std::endl;
        std::cout << "         -> Final position x = " << cueBall.position.x << " m (retreated by "
                  << (impactPointX - cueBall.position.x) * 100.0 << " cm toward break point)! PASS." << std::endl;
    }

    // ----------------------------------------------------
    // TEST 3: FOLLOW SHOT TEST (+0.6 VERTICAL OFFSET ACCELERATION)
    // ----------------------------------------------------
    std::cout << "\n[TEST 3] Testing Follow Shot (+0.6 English) Push-Through..." << std::endl;
    {
        RigidBall cueBall;
        cueBall.id = 0;
        cueBall.position = {0.35, 0.50, R};

        RigidBall objectBall;
        objectBall.id = 1;
        objectBall.position = {0.55, 0.50, R}; // Distance 0.20 m ahead on X axis

        // Strike cue ball with +0.6 vertical tip offset (topspin/follow) along azimuth 0.0
        StrikeParameters followParams{
            .forwardImpulse = 0.55,
            .azimuthRad = 0.0,
            .elevationRad = 0.02,
            .englishX = 0.0,
            .englishY = 0.6 // +0.6 vertical offset
        };
        engine.strike(cueBall, followParams);

        assert(cueBall.velocity.x > 0.0);
        assert(cueBall.omega.y > 0.0); // Topspin

        bool collided = false;
        double impactPointX = 0.0;
        bool continuedForward = false;

        // Step simulation through pre-collision, impact, and post-collision forward follow
        for (int step = 0; step < 1500; ++step) {
            engine.stepBall(cueBall, dt);
            engine.stepBall(objectBall, dt);

            if (!collided) {
                const double dist = std::hypot(objectBall.position.x - cueBall.position.x,
                                               objectBall.position.y - cueBall.position.y);
                if (dist <= 2.0 * R) {
                    RigidBodyPhysicsEngine::resolveBallCollision(cueBall, objectBall);
                    collided = true;
                    impactPointX = cueBall.position.x;
                    // Cue ball retains topspin
                    assert(cueBall.omega.y > 0.0);
                }
            } else {
                // Post-collision: felt friction accelerates cue ball forward through the contact point
                if (cueBall.velocity.x > 0.05 && cueBall.position.x > impactPointX + 0.02) {
                    continuedForward = true;
                }
            }
        }

        assert(collided);
        assert(continuedForward);
        assert(cueBall.position.x > impactPointX + 0.05);

        std::cout << "         -> Collided at x = " << impactPointX << " m" << std::endl;
        std::cout << "         -> Final cue ball position x = " << cueBall.position.x 
                  << " m (pushed forward " << (cueBall.position.x - impactPointX) * 100.0 << " cm into object ball path)! PASS." << std::endl;
    }

    // ----------------------------------------------------
    // TEST 4: RAIL RESTITUTION & DECAY CURVE (K-55 CUSHIONS)
    // ----------------------------------------------------
    std::cout << "\n[TEST 4] Testing K-55 Cushion Energy Decay Curves & Boundary Clamping..." << std::endl;
    {
        RigidBall ball;
        ball.id = 0;
        ball.position = {0.50, 0.20, R};
        ball.velocity = {2.5, -3.5, 0.0}; // Moving toward top cushion (y = 0)
        ball.state = MotionState::Sliding;

        double prevEnergy = 0.5 * RigidBodyPhysicsEngine::BALL_MASS * ball.velocity.length2D() * ball.velocity.length2D();
        int bounceCount = 0;

        for (int step = 0; step < 3000; ++step) {
            engine.stepBall(ball, dt);
            const bool bounced = table.resolveCushionBounce(ball);

            // Boundary validation: ball must NEVER clip through rails
            assert(ball.position.x >= R - 1e-4);
            assert(ball.position.x <= ValleyTableGeometry::PLAY_LENGTH - R + 1e-4);
            assert(ball.position.y >= R - 1e-4);
            assert(ball.position.y <= ValleyTableGeometry::PLAY_WIDTH - R + 1e-4);

            if (bounced) {
                bounceCount++;
                const double currentEnergy = 0.5 * RigidBodyPhysicsEngine::BALL_MASS * ball.velocity.length2D() * ball.velocity.length2D();
                // Energy decay: each rail rebound must strictly reduce mechanical energy
                assert(currentEnergy < prevEnergy);
                prevEnergy = currentEnergy;

                if (bounceCount >= 4) break;
            }
        }

        assert(bounceCount >= 3);
        std::cout << "         -> Completed " << bounceCount << " cushion rebounds along non-linear energy decay curve without sticking or clipping! PASS." << std::endl;
    }

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "   ALL 4 IMPACT & FRICTION ASSERTIONS PASSED!             " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
