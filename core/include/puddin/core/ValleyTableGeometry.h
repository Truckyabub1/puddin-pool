#pragma once

#include "puddin/core/PhysicsEngine.h"
#include <vector>

namespace puddin::core {

enum class PocketType {
    Corner,
    Side
};

struct ValleyPocket {
    PocketType type;
    double x;
    double y;
    double throatWidth; // Meters
    double mouthWidth;  // Meters
    double dropRadius;  // Meters
    double shelfAngle;  // Radians
};

class ValleyTableGeometry {
public:
    // Valley 7-Foot Slate Dimensions (Meters)
    static constexpr double PLAY_WIDTH  = 0.9652; // 38.0 inches (narrow axis)
    static constexpr double PLAY_LENGTH = 1.9304; // 76.0 inches (long axis)
    static constexpr double FRAME_WIDTH = 1.3462; // 53.0 inches
    static constexpr double FRAME_LENGTH= 2.3622; // 93.0 inches

    // K-55 Cushion Specifications
    static constexpr double K55_BASE_RESTITUTION = 0.82; // e0
    static constexpr double K55_VELOCITY_COEFF   = 0.035;// kv
    static constexpr double K55_RAIL_FRICTION    = 0.18; // mu_rail
    static constexpr double K55_SPIN_TRANSFER    = 0.015;// Transfer of omega_z to vt

    ValleyTableGeometry();

    // Check and resolve collision of a ball with K-55 cushions
    bool resolveCushionBounce(RigidBall& ball) const;

    // Check if ball falls into a pocket shelf (modeling 3D drop and high-speed rattle)
    bool checkPocketDrop(RigidBall& ball) const;

    const std::vector<ValleyPocket>& getPockets() const { return pockets_; }

private:
    std::vector<ValleyPocket> pockets_;
    void initPockets();
};

} // namespace puddin::core
