#include "puddin/core/ValleyTableGeometry.h"
#include <algorithm>
#include <cmath>

namespace puddin::core {

ValleyTableGeometry::ValleyTableGeometry() {
    initPockets();
}

void ValleyTableGeometry::initPockets() {
    pockets_.clear();

    // Corner pockets: 4.5 in throat, 5.125 in mouth, 45 degree cut
    const double cornerThroat = 0.1143;
    const double cornerMouth  = 0.130175;
    const double cornerDropR  = 0.065;
    const double cornerAngle  = 0.785398; // 45 degrees

    // Side pockets: 5.0 in throat, 104 degree flare
    const double sideThroat   = 0.1270;
    const double sideMouth    = 0.1380;
    const double sideDropR    = 0.060;
    const double sideAngle    = 1.815142; // 104 degrees

    // 4 Corner Pockets
    pockets_.push_back({PocketType::Corner, 0.035, 0.035, cornerThroat, cornerMouth, cornerDropR, cornerAngle});
    pockets_.push_back({PocketType::Corner, PLAY_LENGTH - 0.035, 0.035, cornerThroat, cornerMouth, cornerDropR, cornerAngle});
    pockets_.push_back({PocketType::Corner, 0.035, PLAY_WIDTH - 0.035, cornerThroat, cornerMouth, cornerDropR, cornerAngle});
    pockets_.push_back({PocketType::Corner, PLAY_LENGTH - 0.035, PLAY_WIDTH - 0.035, cornerThroat, cornerMouth, cornerDropR, cornerAngle});

    // 2 Side Pockets
    pockets_.push_back({PocketType::Side, PLAY_LENGTH * 0.5, 0.015, sideThroat, sideMouth, sideDropR, sideAngle});
    pockets_.push_back({PocketType::Side, PLAY_LENGTH * 0.5, PLAY_WIDTH - 0.015, sideThroat, sideMouth, sideDropR, sideAngle});
}

bool ValleyTableGeometry::checkPocketDrop(RigidBall& ball) const {
    if (ball.isSunk) return false;

    const double spd = std::hypot(ball.velocity.x, ball.velocity.y);

    for (const auto& p : pockets_) {
        const double dist = std::hypot(ball.position.x - p.x, ball.position.y - p.y);
        if (dist < p.dropRadius) {
            // High-speed rattle check on pocket shelf facing
            // Slow & medium speed balls drop smoothly into leather net
            if (spd > 5.8 && dist > p.dropRadius * 0.65) {
                // High-speed undercut: rattles against pocket facing and rejects
                const double nx = (ball.position.x - p.x) / dist;
                const double ny = (ball.position.y - p.y) / dist;
                ball.velocity.x = std::abs(ball.velocity.x) * nx * 0.55;
                ball.velocity.y = std::abs(ball.velocity.y) * ny * 0.55;
                return false;
            }

            // Sunk
            ball.isSunk = true;
            ball.velocity = {0, 0, 0};
            ball.omega = {0, 0, 0};
            ball.state = MotionState::Stationary;
            return true;
        }
    }
    return false;
}

bool ValleyTableGeometry::resolveCushionBounce(RigidBall& ball) const {
    if (ball.isSunk) return false;

    const double r = RigidBodyPhysicsEngine::BALL_RADIUS;
    bool bounced = false;

    // Helper lambda to compute K-55 non-linear velocity restitution
    auto computeRestitution = [](double vn) {
        return std::max(0.55, K55_BASE_RESTITUTION * (1.0 - K55_VELOCITY_COEFF * std::abs(vn)));
    };

    // 1. Left Cushion (x = 0)
    if (ball.position.x - r < 0.0 && ball.velocity.x < 0.0) {
        ball.position.x = r;
        const double vn = std::abs(ball.velocity.x);
        const double e = computeRestitution(vn);
        ball.velocity.x = vn * e; // Rebound to the right (+X)

        // Tangential friction and spin exchange on Y axis
        // Left cushion normal points +X. Right English (omega_z < 0) pushes ball in -Y direction
        const double spinShift = -ball.omega.z * K55_SPIN_TRANSFER;
        ball.velocity.y += spinShift;
        ball.velocity.y *= (1.0 - K55_RAIL_FRICTION * 0.5);
        ball.omega.z *= 0.65; // Cushion grabs and bleeds spin
        bounced = true;
    }
    // 2. Right Cushion (x = PLAY_LENGTH)
    else if (ball.position.x + r > PLAY_LENGTH && ball.velocity.x > 0.0) {
        ball.position.x = PLAY_LENGTH - r;
        const double vn = std::abs(ball.velocity.x);
        const double e = computeRestitution(vn);
        ball.velocity.x = -vn * e; // Rebound to the left (-X)

        // Right cushion normal points -X. Right English pushes ball in +Y direction
        const double spinShift = ball.omega.z * K55_SPIN_TRANSFER;
        ball.velocity.y += spinShift;
        ball.velocity.y *= (1.0 - K55_RAIL_FRICTION * 0.5);
        ball.omega.z *= 0.65;
        bounced = true;
    }

    // 3. Top Cushion (y = 0)
    if (ball.position.y - r < 0.0 && ball.velocity.y < 0.0) {
        ball.position.y = r;
        const double vn = std::abs(ball.velocity.y);
        const double e = computeRestitution(vn);
        ball.velocity.y = vn * e; // Rebound down (+Y)

        // Top cushion normal points +Y. Right English pushes ball in +X direction
        const double spinShift = ball.omega.z * K55_SPIN_TRANSFER;
        ball.velocity.x += spinShift;
        ball.velocity.x *= (1.0 - K55_RAIL_FRICTION * 0.5);
        ball.omega.z *= 0.65;
        bounced = true;
    }
    // 4. Bottom Cushion (y = PLAY_WIDTH)
    else if (ball.position.y + r > PLAY_WIDTH && ball.velocity.y > 0.0) {
        ball.position.y = PLAY_WIDTH - r;
        const double vn = std::abs(ball.velocity.y);
        const double e = computeRestitution(vn);
        ball.velocity.y = -vn * e; // Rebound up (-Y)

        // Bottom cushion normal points -Y. Right English pushes ball in -X direction
        const double spinShift = -ball.omega.z * K55_SPIN_TRANSFER;
        ball.velocity.x += spinShift;
        ball.velocity.x *= (1.0 - K55_RAIL_FRICTION * 0.5);
        ball.omega.z *= 0.65;
        bounced = true;
    }

    if (bounced) {
        ball.state = MotionState::Sliding; // Reset sliding phase post-bounce
    }
    return bounced;
}

} // namespace puddin::core
