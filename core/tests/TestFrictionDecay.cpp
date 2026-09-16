#include "puddin/core/PhysicsEngine.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace puddin::core;

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "     CLOTH FRICTION & ROLL DECAY TEST SUITE               " << std::endl;
    std::cout << "==========================================================" << std::endl;

    RigidBodyPhysicsEngine engine;
    const double dt = 0.001; // 1 millisecond sub-step
    const double R = RigidBodyPhysicsEngine::BALL_RADIUS;

    // ----------------------------------------------------
    // TEST 1: CENTER STRIKE SLIDE-TO-ROLL TRANSITION
    // ----------------------------------------------------
    std::cout << "\n[TEST 1] Testing Center Strike (2.5 m/s) Slide-to-Roll Distance..." << std::endl;
    {
        RigidBall ball;
        ball.id = 0;
        ball.position = {0.0, 0.0, R};
        ball.velocity = {2.5, 0.0, 0.0}; // 2.5 m/s linear velocity
        ball.omega = {0.0, 0.0, 0.0};    // 0 spin (pure skid/slide at contact point)
        ball.state = MotionState::Sliding;

        double slideTransitionX = 0.0;
        bool transitioned = false;

        for (int step = 0; step < 5000; ++step) {
            engine.stepBall(ball, dt);

            if (ball.state == MotionState::Rolling && !transitioned) {
                transitioned = true;
                slideTransitionX = ball.position.x;
                break;
            }
        }

        assert(transitioned);
        std::cout << "         -> Transitioned from slide to pure roll at x = " 
                  << slideTransitionX << " m" << std::endl;

        // Must transition from slide to pure roll within 0.45m to 0.65m
        assert(slideTransitionX >= 0.45 && slideTransitionX <= 0.65);
        std::cout << "         -> Slide-to-roll transition verified within [0.45m, 0.65m]! PASS." << std::endl;
    }

    // ----------------------------------------------------
    // TEST 2: TOTAL TRAVEL DISTANCE (ROLLING DECAY)
    // ----------------------------------------------------
    std::cout << "\n[TEST 2] Testing 2.5 m/s Rolling Ball Total Travel Distance..." << std::endl;
    {
        RigidBall ball;
        ball.id = 0;
        ball.position = {0.0, 0.0, R};
        ball.velocity = {2.5, 0.0, 0.0};
        ball.omega = {0.0, 2.5 / R, 0.0}; // Synchronized rolling spin
        ball.state = MotionState::Rolling;

        bool stopped = false;
        double stopDistance = 0.0;

        for (int step = 0; step < 10000; ++step) {
            engine.stepBall(ball, dt);

            if (ball.state == MotionState::Stationary) {
                stopped = true;
                stopDistance = ball.position.x;
                break;
            }
        }

        assert(stopped);
        std::cout << "         -> Rolling ball came to rest at x = " << stopDistance << " m" << std::endl;

        // Must come to an absolute rest within approximately 2.0m to 2.4m, eliminating indefinite glide
        assert(stopDistance >= 2.0 && stopDistance <= 2.4);
        std::cout << "         -> Total travel distance verified within [2.0m, 2.4m]! PASS." << std::endl;
    }

    // ----------------------------------------------------
    // TEST 3: ZERO-CREEP DEAD STOP
    // ----------------------------------------------------
    std::cout << "\n[TEST 3] Testing Zero-Creep Halt & Dead Stop Snap..." << std::endl;
    {
        // Test ball traveling along angled diagonal at low speed
        const double angleRad = M_PI / 4.0; // 45 degrees
        const double lowSpeed = 0.05;       // 0.05 m/s (approaching rest threshold)

        RigidBall ball;
        ball.id = 0;
        ball.position = {1.0, 1.0, R};
        ball.velocity = {lowSpeed * std::cos(angleRad), lowSpeed * std::sin(angleRad), 0.0};
        ball.omega = {-ball.velocity.y / R, ball.velocity.x / R, 0.0};
        ball.state = MotionState::Rolling;

        int stepCountToHalt = 0;
        for (int step = 0; step < 2000; ++step) {
            engine.stepBall(ball, dt);
            if (ball.state == MotionState::Stationary) {
                stepCountToHalt = step;
                break;
            }
        }

        assert(ball.state == MotionState::Stationary);
        // Verify exact zero velocities (snap to dead stop, no creeping)
        assert(ball.velocity.x == 0.0);
        assert(ball.velocity.y == 0.0);
        assert(ball.velocity.z == 0.0);
        assert(ball.omega.x == 0.0);
        assert(ball.omega.y == 0.0);
        assert(ball.omega.z == 0.0);

        // Verify that subsequent stepBall calls perform zero drift
        const double finalX = ball.position.x;
        const double finalY = ball.position.y;
        for (int step = 0; step < 100; ++step) {
            engine.stepBall(ball, dt);
        }
        assert(ball.position.x == finalX);
        assert(ball.position.y == finalY);

        std::cout << "         -> Ball halted in " << stepCountToHalt 
                  << " ms and snapped cleanly to absolute zero without asymptotic creeping! PASS." << std::endl;
    }

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "   ALL FRICTION & ROLL DECAY ASSERTIONS PASSED (3/3)!     " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
