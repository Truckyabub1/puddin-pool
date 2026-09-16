#include "puddin/core/PhysicsEngine.h"
#include "puddin/core/ValleyTableGeometry.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace puddin::core;

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  3D RIGID-BODY DYNAMICS & VALLEY K-55 CUSHION TEST SUITE" << std::endl;
    std::cout << "==========================================================" << std::endl;

    RigidBodyPhysicsEngine engine;
    ValleyTableGeometry table;

    // ----------------------------------------------------
    // TEST 1: DRAW SHOT (BOTTOM ENGLISH) REVERSAL
    // ----------------------------------------------------
    std::cout << "\n[TEST 1] Testing Draw Shot (Bottom English) Dynamics..." << std::endl;
    RigidBall cueBall;
    cueBall.position = {0.5, 0.5, RigidBodyPhysicsEngine::BALL_RADIUS};
    
    // Strike cue ball forward (+X azimuth 0.0) with maximum bottom spin (englishY = -1.0)
    StrikeParameters drawParams{
        .forwardImpulse = 0.55,
        .azimuthRad = 0.0,
        .elevationRad = 0.05,
        .englishX = 0.0,
        .englishY = -1.0 // Maximum draw
    };
    engine.strike(cueBall, drawParams);
    assert(cueBall.velocity.x > 0.0); // Initially traveling forward
    assert(cueBall.omega.y < 0.0);     // Backspin (omega_y is negative for forward roll)

    // Simulate draw ball in free slide on felt
    double minVx = cueBall.velocity.x;
    bool arrestedAndReversed = false;
    const double dt = 0.001;

    for (int step = 0; step < 2000; ++step) {
        engine.stepBall(cueBall, dt);
        if (cueBall.velocity.x < 0.0) {
            arrestedAndReversed = true;
            minVx = cueBall.velocity.x;
            break;
        }
    }
    assert(arrestedAndReversed);
    std::cout << "         -> Draw shot slid forward, arrested, and reversed velocity to " 
              << minVx << " m/s! PASS." << std::endl;

    // ----------------------------------------------------
    // TEST 2: TOPSPIN (FOLLOW) ACCELERATION
    // ----------------------------------------------------
    std::cout << "\n[TEST 2] Testing Topspin (Follow) Acceleration..." << std::endl;
    RigidBall topBall;
    topBall.position = {0.5, 0.5, RigidBodyPhysicsEngine::BALL_RADIUS};

    // Strike with high topspin (englishY = +1.0)
    StrikeParameters topParams{
        .forwardImpulse = 0.55,
        .azimuthRad = 0.0,
        .elevationRad = 0.05,
        .englishX = 0.0,
        .englishY = 1.0 // Maximum follow
    };
    engine.strike(topBall, topParams);
    const double initialVx = topBall.velocity.x;
    assert(topBall.omega.y > 0.0); // Forward roll overspeed

    // Relative contact velocity is backward, so sliding friction accelerates linear speed forward!
    bool acceleratedForward = false;
    for (int step = 0; step < 500; ++step) {
        engine.stepBall(topBall, dt);
        if (topBall.velocity.x > initialVx) {
            acceleratedForward = true;
            break;
        }
    }
    assert(acceleratedForward);
    std::cout << "         -> Topspin exerted forward acceleration through felt grip! PASS." << std::endl;

    // ----------------------------------------------------
    // TEST 3: CUE SQUIRT / DEFLECTION ANGLE
    // ----------------------------------------------------
    std::cout << "\n[TEST 3] Testing Cue Deflection (Squirt) Opposite to English..." << std::endl;
    RigidBall squirtBall;
    squirtBall.position = {0.5, 0.5, RigidBodyPhysicsEngine::BALL_RADIUS};

    // Right English (englishX = +1.0) along azimuth 0.0
    // Right English must push ball sideways to the LEFT (-Y)
    StrikeParameters squirtParams{
        .forwardImpulse = 0.50,
        .azimuthRad = 0.0,
        .elevationRad = 0.05,
        .englishX = 1.0, // Right English
        .englishY = 0.0
    };
    engine.strike(squirtBall, squirtParams);
    assert(squirtBall.velocity.y < 0.0); // Deflected left!
    const double squirtAngleDeg = std::atan2(squirtBall.velocity.y, squirtBall.velocity.x) * (180.0 / M_PI);
    assert(squirtAngleDeg < -1.0 && squirtAngleDeg > -3.0);
    std::cout << "         -> Right English produced squirt deflection of " 
              << squirtAngleDeg << " deg (opposite to tip offset)! PASS." << std::endl;

    // ----------------------------------------------------
    // TEST 4: K-55 CUSHION RUNNING VS REVERSE ENGLISH (500 ANGLES)
    // ----------------------------------------------------
    std::cout << "\n[TEST 4] Testing K-55 Cushion Dynamics across 500 incident angles..." << std::endl;
    int verifiedAngles = 0;

    for (int i = 0; i < 500; ++i) {
        // Incident angle theta from 15 deg to 75 deg against bottom rail (y = PLAY_WIDTH)
        double thetaIn = 0.2618 + (i / 500.0) * (1.3090 - 0.2618); // 15 to 75 deg
        double speed = 2.5;

        // Ball A: Running English (right English heading toward bottom rail)
        RigidBall ballRunning;
        ballRunning.position = {0.8, ValleyTableGeometry::PLAY_WIDTH - RigidBodyPhysicsEngine::BALL_RADIUS * 0.99, RigidBodyPhysicsEngine::BALL_RADIUS};
        ballRunning.velocity = {speed * std::cos(thetaIn), speed * std::sin(thetaIn), 0};
        ballRunning.omega = {0, 0, -120.0}; // Running English

        // Ball B: Reverse English (checks the ball)
        RigidBall ballReverse;
        ballReverse.position = ballRunning.position;
        ballReverse.velocity = ballRunning.velocity;
        ballReverse.omega = {0, 0, 120.0}; // Reverse English

        table.resolveCushionBounce(ballRunning);
        table.resolveCushionBounce(ballReverse);

        // Rebound angle: theta = atan2(|vy|, vx)
        double thetaOutRunning = std::atan2(std::abs(ballRunning.velocity.y), std::abs(ballRunning.velocity.x));
        double thetaOutReverse = std::atan2(std::abs(ballReverse.velocity.y), std::abs(ballReverse.velocity.x));

        // Running English accelerates tangential speed -> thetaOutRunning is flatter/wider
        // Reverse English checks tangential speed -> thetaOutReverse is steeper/tighter
        assert(ballRunning.velocity.x > ballReverse.velocity.x);
        verifiedAngles++;
    }
    assert(verifiedAngles == 500);
    std::cout << "         -> 500/500 Cushion incident angles verified: running English accelerates rebound, reverse English checks speed! PASS." << std::endl;

    // ----------------------------------------------------
    // TEST 5: VALLEY POCKET DROP SHELF & HIGH-SPEED RATTLE
    // ----------------------------------------------------
    std::cout << "\n[TEST 5] Testing Valley Pocket 3D Shelf & High-Speed Rattle..." << std::endl;
    // Slow roll into corner pocket
    RigidBall slowRoll;
    slowRoll.position = {0.04, 0.04, RigidBodyPhysicsEngine::BALL_RADIUS};
    slowRoll.velocity = {-0.6, -0.6, 0};
    bool dropped = table.checkPocketDrop(slowRoll);
    assert(dropped && slowRoll.isSunk);

    // Fast undercut into corner pocket shelf
    RigidBall fastUndercut;
    fastUndercut.position = {0.08, 0.04, RigidBodyPhysicsEngine::BALL_RADIUS};
    fastUndercut.velocity = {-6.8, -1.2, 0}; // 6.9 m/s high speed
    bool rejected = !table.checkPocketDrop(fastUndercut);
    assert(rejected && !fastUndercut.isSunk);
    std::cout << "         -> Slow ball dropped into shelf; fast undercut rattled off facing! PASS." << std::endl;

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "  ALL 3D PHYSICS & VALLEY K-55 TESTS PASSED (5/5)!" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
