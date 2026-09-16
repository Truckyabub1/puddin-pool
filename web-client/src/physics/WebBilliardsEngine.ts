export interface BallState {
  id: number;
  x: number;
  y: number;
  vx: number;
  vy: number;
  // 3D Angular Velocity Vector (rad/s)
  wx: number;
  wy: number;
  wz: number;
  isSunk: boolean;
  state: "stationary" | "sliding" | "rolling";
  rotQuaternion: [number, number, number, number]; // [x, y, z, w]
}

export interface TrajectoryPreview {
  hasHit: boolean;
  ghostX: number;
  ghostY: number;
  targetBall: BallState | null;
  normX: number;
  normY: number;
  deflectX: number;
  deflectY: number;
  cuePoints: { x: number; y: number }[];
  targetPoints: { x: number; y: number }[];
}

export interface LegendRecommendation {
  legendName: string;
  advice: string;
  recommendedAimAngle: number;
  recommendedPower: number;
  recommendedSpin: [number, number];
  previewPoints: { x: number; y: number }[];
}

export class WebBilliardsEngine {
  // Valley 7-Foot Commercial Specifications: 38" x 76" (0.9652m x 1.9304m)
  // Standard Tournament: 2.24m x 1.12m
  public TABLE_WIDTH = 1.9304;
  public TABLE_HEIGHT = 0.9652;

  public readonly BALL_RADIUS = 0.028575; // 2.25 in diameter (0.05715m)
  public readonly BALL_MASS = 0.170097;   // 6.0 oz (0.170097 kg)
  public readonly BALL_RESTITUTION = 0.96;
  public readonly BALL_FRICTION = 0.04;   // mu_ball (Cut surface throw coefficient)
  public readonly K55_BASE_RESTITUTION = 0.82;
  public readonly K55_VELOCITY_COEFF = 0.035;
  public readonly SLIDING_FRICTION = 0.270;
  public readonly ROLLING_FRICTION = 0.145;
  public readonly VELOCITY_REST = 0.015; // 1.5 cm/s cloth entrapment dead-stop
  public readonly SLIP_EPSILON = 0.005;
  public readonly SPIN_FRICTION = 0.025;  // mu_spin (Gyroscopic vertical spin decay)
  public readonly GRAVITY = 9.80665;

  public balls: BallState[] = [];
  public pockets: { x: number; y: number; r: number; throat: number }[] = [];
  public isQuiescent = true;
  public currentMode: string = '9ball';

  // Shot event tracking for WPA arbitration
  public firstContactBallId: number = -1;
  public railHitAfterContact: boolean = false;
  public ballsPocketedThisShot: number[] = [];
  public cueScratchThisShot: boolean = false;
  public isBreakShot: boolean = true;

  constructor() {
    this.initPockets();
    this.resetRack('9ball');
  }

  public setTableDimensions(lengthMeters: number, widthMeters: number): void {
    this.TABLE_WIDTH = lengthMeters;
    this.TABLE_HEIGHT = widthMeters;
    this.initPockets();
    this.resetRack(this.currentMode);
  }

  private initPockets(): void {
    const cornerOffset = 0.042;
    const cornerDropR = 0.058;
    const sideDropR = 0.054;
    this.pockets = [
      { x: cornerOffset, y: cornerOffset, r: cornerDropR, throat: 0.1143 },
      { x: this.TABLE_WIDTH * 0.5, y: 0.018, r: sideDropR, throat: 0.1270 },
      { x: this.TABLE_WIDTH - cornerOffset, y: cornerOffset, r: cornerDropR, throat: 0.1143 },
      { x: cornerOffset, y: this.TABLE_HEIGHT - cornerOffset, r: cornerDropR, throat: 0.1143 },
      { x: this.TABLE_WIDTH * 0.5, y: this.TABLE_HEIGHT - 0.018, r: sideDropR, throat: 0.1270 },
      { x: this.TABLE_WIDTH - cornerOffset, y: this.TABLE_HEIGHT - cornerOffset, r: cornerDropR, throat: 0.1143 },
    ];
  }

  public resetRack(mode: string = '9ball'): void {
    this.currentMode = mode;
    this.balls = [];
    this.isBreakShot = true;
    this.resetShotTracking();

    // Cue ball at head string
    this.balls.push({
      id: 0,
      x: this.TABLE_WIDTH * 0.25,
      y: this.TABLE_HEIGHT * 0.5,
      vx: 0,
      vy: 0,
      wx: 0,
      wy: 0,
      wz: 0,
      isSunk: false,
      state: "stationary",
      rotQuaternion: [0, 0, 0, 1],
    });

    const apexX = this.TABLE_WIDTH * 0.70;
    const apexY = this.TABLE_HEIGHT * 0.50;
    const d = this.BALL_RADIUS * 2.0;
    const rowSpacing = d * Math.sqrt(3.0) * 0.5;

    if (mode === '9ball') {
      // Official 9-Ball Diamond Rack: 1 at apex, 9 in center
      const diamondIds = [
        [1],
        [2, 3],
        [4, 9, 5],
        [6, 7],
        [8]
      ];

      for (let row = 0; row < diamondIds.length; ++row) {
        const rowBalls = diamondIds[row];
        const colX = apexX + row * rowSpacing;
        const startY = apexY - (rowBalls.length - 1) * d * 0.5;
        for (let i = 0; i < rowBalls.length; ++i) {
          this.balls.push({
            id: rowBalls[i],
            x: colX,
            y: startY + i * d,
            vx: 0,
            vy: 0,
            wx: 0,
            wy: 0,
            wz: 0,
            isSunk: false,
            state: "stationary",
            rotQuaternion: [0, 0, 0, 1],
          });
        }
      }
    } else {
      // Official WPA 15-Ball Triangle Rack (8-ball):
      // 8-ball in center (row 2, center), bottom corner balls of opposite groups (Solid 6, Stripe 15).
      const triangleIds = [
        [1],
        [10, 2],
        [3, 8, 14],
        [9, 4, 11, 5],
        [6, 12, 7, 13, 15]
      ];

      for (let row = 0; row < triangleIds.length; ++row) {
        const rowBalls = triangleIds[row];
        const colX = apexX + row * rowSpacing;
        const startY = apexY - (rowBalls.length - 1) * d * 0.5;
        for (let i = 0; i < rowBalls.length; ++i) {
          this.balls.push({
            id: rowBalls[i],
            x: colX,
            y: startY + i * d,
            vx: 0,
            vy: 0,
            wx: 0,
            wy: 0,
            wz: 0,
            isSunk: false,
            state: "stationary",
            rotQuaternion: [0, 0, 0, 1],
          });
        }
      }
    }

    this.balls.sort((a, b) => a.id - b.id);
    this.isQuiescent = true;
  }

  public resetShotTracking(): void {
    this.firstContactBallId = -1;
    this.railHitAfterContact = false;
    this.ballsPocketedThisShot = [];
    this.cueScratchThisShot = false;
  }

  public strikeCueBall(
    angleRad: number,
    impulseMps: number,
    englishX: number = 0.0,
    englishY: number = 0.0,
    elevationDeg: number = 2.5
  ): void {
    const cue = this.balls[0];
    if (!cue || cue.isSunk) return;

    this.resetShotTracking();

    // 1. Cue Deflection (Squirt Angle): sideways push opposite to applied English
    const squirtAngleRad = -englishX * 0.035; // ~2.0 degrees
    const actualAzimuth = angleRad + squirtAngleRad;

    // 2. Elevation component
    const elevRad = (elevationDeg * Math.PI) / 180.0;
    const v0 = impulseMps * Math.cos(elevRad);
    cue.vx = v0 * Math.cos(actualAzimuth);
    cue.vy = v0 * Math.sin(actualAzimuth);

    // 3. 3D Spin Distribution (Topspin/Draw and English)
    // omega_perp = (2.5 / R) * englishY * (J / m)
    const omegaPerp = (2.5 / this.BALL_RADIUS) * englishY * impulseMps;
    cue.wx = -omegaPerp * Math.sin(actualAzimuth);
    cue.wy = omegaPerp * Math.cos(actualAzimuth);

    // Left/Right English around Z axis
    cue.wz = -(2.5 / this.BALL_RADIUS) * englishX * impulseMps;

    cue.state = "sliding";
    this.isQuiescent = false;
  }

  public isPlacementValid(x: number, y: number): boolean {
    const minClearance = this.BALL_RADIUS * 2.05;
    if (x < this.BALL_RADIUS * 1.5 || x > this.TABLE_WIDTH - this.BALL_RADIUS * 1.5 ||
        y < this.BALL_RADIUS * 1.5 || y > this.TABLE_HEIGHT - this.BALL_RADIUS * 1.5) {
      return false;
    }
    for (let i = 1; i < this.balls.length; ++i) {
      const b = this.balls[i];
      if (b.isSunk) continue;
      if (Math.hypot(x - b.x, y - b.y) < minClearance) {
        return false;
      }
    }
    return true;
  }

  public findValidPlacement(startX: number, startY: number): { x: number; y: number } | null {
    if (this.isPlacementValid(startX, startY)) return { x: startX, y: startY };
    const step = this.BALL_RADIUS * 0.5;
    for (let ring = 1; ring <= 25; ring++) {
      const dist = ring * step;
      for (let angle = 0; angle < Math.PI * 2; angle += Math.PI / 8) {
        const testX = startX + Math.cos(angle) * dist;
        const testY = startY + Math.sin(angle) * dist;
        if (this.isPlacementValid(testX, testY)) {
          return { x: testX, y: testY };
        }
      }
    }
    return null;
  }

  public placeCueBall(x: number, y: number): boolean {
    const cue = this.balls[0];
    if (!cue) return false;
    let targetX = x;
    let targetY = y;
    if (!this.isPlacementValid(targetX, targetY)) {
      const valid = this.findValidPlacement(targetX, targetY);
      if (!valid) return false;
      targetX = valid.x;
      targetY = valid.y;
    }
    cue.isSunk = false;
    cue.x = Math.max(this.BALL_RADIUS, Math.min(this.TABLE_WIDTH - this.BALL_RADIUS, targetX));
    cue.y = Math.max(this.BALL_RADIUS, Math.min(this.TABLE_HEIGHT - this.BALL_RADIUS, targetY));
    cue.vx = 0;
    cue.vy = 0;
    cue.wx = 0;
    cue.wy = 0;
    cue.wz = 0;
    cue.state = "stationary";
    return true;
  }

  public respotNineBall(): void {
    const b9 = this.balls.find(b => b.id === 9);
    if (!b9) return;

    b9.isSunk = false;
    b9.vx = 0;
    b9.vy = 0;
    b9.wx = 0;
    b9.wy = 0;
    b9.wz = 0;
    b9.state = "stationary";

    let spotX = this.TABLE_WIDTH * 0.70;
    const spotY = this.TABLE_HEIGHT * 0.50;
    const d = this.BALL_RADIUS * 2.0;

    let occupied = true;
    while (occupied) {
      occupied = false;
      for (const b of this.balls) {
        if (b.id !== 9 && !b.isSunk) {
          if (Math.hypot(b.x - spotX, b.y - spotY) < d) {
            occupied = true;
            spotX += d;
            break;
          }
        }
      }
    }

    b9.x = spotX;
    b9.y = spotY;
  }

  public getLowestBall(): number {
    for (let id = 1; id <= 9; ++id) {
      const b = this.balls.find(ball => ball.id === id && !ball.isSunk);
      if (b) return id;
    }
    return -1;
  }

  public step(dt: number, onImpact?: (speed: number) => void, onCushion?: () => void, onPocket?: (ballId: number) => void): void {
    let anyMoving = false;
    const EPSILON = this.SLIP_EPSILON;

    // 1. Movement, 3D Dual-Phase Friction, and Spin Dynamics
    for (const b of this.balls) {
      if (b.isSunk || b.state === "stationary") continue;

      // Position update
      const prevX = b.x;
      const prevY = b.y;
      b.x += b.vx * dt;
      b.y += b.vy * dt;

      // Relative surface contact point velocity: v_cp = (vx - wy * R, vy + wx * R)
      const cpVx = b.vx - b.wy * this.BALL_RADIUS;
      const cpVy = b.vy + b.wx * this.BALL_RADIUS;
      const cpSpeed = Math.hypot(cpVx, cpVy);
      const ballSpeed = Math.hypot(b.vx, b.vy);

      const spinDecayRate = (2.5 / this.BALL_RADIUS) * this.SPIN_FRICTION * this.GRAVITY * 2.0;

      if (cpSpeed > EPSILON) {
        // --- DUAL-PHASE: SLIDING / SKIDDING STATE ---
        if (ballSpeed <= this.VELOCITY_REST && cpSpeed <= 0.02) {
          // Cleanly arrest micro-skids at low velocity
          b.vx = 0;
          b.vy = 0;
          b.wx = 0;
          b.wy = 0;
          b.wz = 0;
          b.state = "stationary";
        } else {
          const dvCpMax = 3.5 * this.SLIDING_FRICTION * this.GRAVITY * dt;
          if (cpSpeed <= dvCpMax) {
            // Analytical felt grip: exactly arrest contact point velocity in this substep
            b.vx -= (1.0 / 3.5) * cpVx;
            b.vy -= (1.0 / 3.5) * cpVy;
            b.wx = -b.vy / this.BALL_RADIUS;
            b.wy =  b.vx / this.BALL_RADIUS;
            b.state = "rolling";
            anyMoving = true;
          } else {
            b.state = "sliding";
            anyMoving = true;

            const fNorm = -this.SLIDING_FRICTION * this.GRAVITY;
            const ax = fNorm * (cpVx / cpSpeed);
            const ay = fNorm * (cpVy / cpSpeed);

            const alphaCoeff = 2.5 / this.BALL_RADIUS;
            const alphaX =  alphaCoeff * ay;
            const alphaY = -alphaCoeff * ax;

            b.vx += ax * dt;
            b.vy += ay * dt;

            b.wx += alphaX * dt;
            b.wy += alphaY * dt;
          }
          
          if (b.wz > 0) b.wz = Math.max(0, b.wz - spinDecayRate * dt);
          else if (b.wz < 0) b.wz = Math.min(0, b.wz + spinDecayRate * dt);

          // Swerve/Masse curvature when vertical spin wz is present during slide
          if (Math.abs(b.wz) > 1.0 && ballSpeed > 0.05) {
            const swerveForce = 0.04 * (b.wz / 100.0) * this.SLIDING_FRICTION * this.GRAVITY;
            b.vx += -b.vy / ballSpeed * swerveForce * dt;
            b.vy +=  b.vx / ballSpeed * swerveForce * dt;
          }
        }
      } else {
        // --- DUAL-PHASE: PURE ROLLING STATE ---
        b.state = "rolling";

        if (ballSpeed > this.VELOCITY_REST) {
          anyMoving = true;
          // Worsted cloth nap resistance: slight progressive deceleration at very low velocities
          const napFactor = (ballSpeed < 0.20) ? (1.0 + 0.05 / (ballSpeed + 0.015)) : 1.0;
          const aRoll = -this.ROLLING_FRICTION * this.GRAVITY * napFactor;
          const newSpeed = Math.max(0, ballSpeed + aRoll * dt);
          b.vx = (b.vx / ballSpeed) * newSpeed;
          b.vy = (b.vy / ballSpeed) * newSpeed;

          // Pure rolling constraint
          b.wx = -b.vy / this.BALL_RADIUS;
          b.wy =  b.vx / this.BALL_RADIUS;
          
          if (b.wz > 0) b.wz = Math.max(0, b.wz - spinDecayRate * dt);
          else if (b.wz < 0) b.wz = Math.min(0, b.wz + spinDecayRate * dt);
        } else {
          // Dead stop snap: eliminate endless creeping
          b.vx = 0;
          b.vy = 0;
          b.wx = 0;
          b.wy = 0;
          b.wz = 0;
          b.state = "stationary";
        }
      }

      // Quaternion rotation integration for visual rolling
      const dx = b.x - prevX;
      const dy = b.y - prevY;
      const dist = Math.hypot(dx, dy);
      if (dist > 1e-6) {
        const angle = dist / this.BALL_RADIUS;
        const ax = -dy / dist;
        const ay = dx / dist;
        const halfSin = Math.sin(angle * 0.5);
        const halfCos = Math.cos(angle * 0.5);
        const qx = ax * halfSin;
        const qy = ay * halfSin;
        const qw = halfCos;
        const curW = b.rotQuaternion[3];
        b.rotQuaternion = [
          curW * qx + b.rotQuaternion[0] * qw,
          curW * qy + b.rotQuaternion[1] * qw,
          0,
          curW * qw - b.rotQuaternion[0] * qx - b.rotQuaternion[1] * qy,
        ];
      }
    }

    // 2. Pocket captures with 3D drop shelf
    for (const b of this.balls) {
      if (b.isSunk) continue;
      const spd = Math.hypot(b.vx, b.vy);

      for (const p of this.pockets) {
        const dist = Math.hypot(b.x - p.x, b.y - p.y);
        if (dist < p.r) {
          // Fast undercut rattle rejection
          if (spd > 5.8 && dist > p.r * 0.65) {
            b.vx = -b.vx * 0.45;
            b.vy = -b.vy * 0.45;
            continue;
          }

          b.isSunk = true;
          b.vx = 0;
          b.vy = 0;
          b.wx = 0;
          b.wy = 0;
          b.wz = 0;
          b.state = "stationary";

          if (b.id === 0) {
            this.cueScratchThisShot = true;
          } else {
            this.ballsPocketedThisShot.push(b.id);
          }
          onPocket?.(b.id);
          break;
        }
      }
    }

    // 3. K-55 Cushion Collisions with Non-Linear Restitution & Spin Exchange
    const r = this.BALL_RADIUS;
    const computeK55Restitution = (vn: number) => {
      return Math.max(0.55, this.K55_BASE_RESTITUTION * (1.0 - this.K55_VELOCITY_COEFF * Math.abs(vn)));
    };

    for (const b of this.balls) {
      if (b.isSunk) continue;
      let hitCushion = false;

      // Left rail (x = 0)
      if (b.x - r < 0.0 && b.vx < 0.0) {
        b.x = r;
        const vn = Math.abs(b.vx);
        b.vx = vn * computeK55Restitution(vn);
        b.vy += -b.wz * 0.015;
        b.vy *= 0.91;
        b.wz = b.wz * 0.65 - Math.sign(b.vy) * Math.min(25.0 * vn, Math.abs(b.vy) / r * 0.5);
        hitCushion = true;
      }
      // Right rail (x = TABLE_WIDTH)
      else if (b.x + r > this.TABLE_WIDTH && b.vx > 0.0) {
        b.x = this.TABLE_WIDTH - r;
        const vn = Math.abs(b.vx);
        b.vx = -vn * computeK55Restitution(vn);
        b.vy += b.wz * 0.015;
        b.vy *= 0.91;
        b.wz = b.wz * 0.65 + Math.sign(b.vy) * Math.min(25.0 * vn, Math.abs(b.vy) / r * 0.5);
        hitCushion = true;
      }

      // Top rail (y = 0)
      if (b.y - r < 0.0 && b.vy < 0.0) {
        b.y = r;
        const vn = Math.abs(b.vy);
        b.vy = vn * computeK55Restitution(vn);
        b.vx += b.wz * 0.015;
        b.vx *= 0.91;
        b.wz = b.wz * 0.65 + Math.sign(b.vx) * Math.min(25.0 * vn, Math.abs(b.vx) / r * 0.5);
        hitCushion = true;
      }
      // Bottom rail (y = TABLE_HEIGHT)
      else if (b.y + r > this.TABLE_HEIGHT && b.vy > 0.0) {
        b.y = this.TABLE_HEIGHT - r;
        const vn = Math.abs(b.vy);
        b.vy = -vn * computeK55Restitution(vn);
        b.vx += -b.wz * 0.015;
        b.vx *= 0.91;
        b.wz = b.wz * 0.65 - Math.sign(b.vx) * Math.min(25.0 * vn, Math.abs(b.vx) / r * 0.5);
        hitCushion = true;
      }

      if (hitCushion) {
        b.state = "sliding";
        onCushion?.();
        if (this.firstContactBallId !== -1) {
          this.railHitAfterContact = true;
        }
      }
    }

    // 4. Ball-to-ball collisions & 3-pass cluster relaxation
    for (let pass = 0; pass < 3; ++pass) {
      for (let i = 0; i < this.balls.length; ++i) {
        const b1 = this.balls[i];
        if (b1.isSunk) continue;
        for (let j = i + 1; j < this.balls.length; ++j) {
          const b2 = this.balls[j];
          if (b2.isSunk) continue;

          const dx = b2.x - b1.x;
          const dy = b2.y - b1.y;
          const dist = Math.hypot(dx, dy);
          const minDist = this.BALL_RADIUS * 2.0;

          if (dist <= minDist && dist > 1e-6) {
            const nx = dx / dist;
            const ny = dy / dist;
            const overlap = minDist - dist;

            // Position separation on every pass to untangle break clusters
            b1.x -= nx * overlap * 0.5;
            b1.y -= ny * overlap * 0.5;
            b2.x += nx * overlap * 0.5;
            b2.y += ny * overlap * 0.5;

            // Resolve impulse and spin transfer on primary pass
            if (pass === 0) {
              const tx = -ny;
              const ty = nx;
              const rvx = b1.vx - b2.vx;
              const rvy = b1.vy - b2.vy;
              const vn = rvx * nx + rvy * ny;

              if (vn > 0) {
                const Jn = -0.5 * (1.0 + this.BALL_RESTITUTION) * this.BALL_MASS * vn;

                // Cut-Induced Throw (Tangential Impulse)
                const vRelT = (rvx * tx + rvy * ty) + this.BALL_RADIUS * (b1.wz + b2.wz);
                const maxThrow = this.BALL_FRICTION * Math.abs(Jn);
                const jtUncapped = -0.5 * this.BALL_MASS * vRelT;
                const Jt = Math.max(-maxThrow, Math.min(maxThrow, jtUncapped));

                const v1tOrig = b1.vx * tx + b1.vy * ty;
                const v2nOrig = b2.vx * nx + b2.vy * ny;
                const v2tOrig = b2.vx * tx + b2.vy * ty;

                const v1nNew = v2nOrig;
                const v1tNew = v1tOrig + Jt / this.BALL_MASS;
                b1.vx = v1nNew * nx + v1tNew * tx;
                b1.vy = v1nNew * ny + v1tNew * ty;

                const v2nNew = v2nOrig - Jn / this.BALL_MASS;
                const v2tNew = v2tOrig - Jt / this.BALL_MASS;
                b2.vx = v2nNew * nx + v2tNew * tx;
                b2.vy = v2nNew * ny + v2tNew * ty;

                // Tangential torque spin transfer (gearing side spin: action-reaction opposite)
                const spinImpulse = (2.5 / this.BALL_RADIUS) * (Jt / this.BALL_MASS);
                b1.wz += spinImpulse;
                b2.wz -= spinImpulse;

                // Vertical rolling spin / counter-spin transfer along normal of impact
                // Topspin/backspin from cue ball drags object ball surface in opposite vertical direction
                const vNormCueSpin = -b1.wx * ny + b1.wy * nx;
                const counterSpinTransfer = vNormCueSpin * 0.12;
                b2.wx += counterSpinTransfer * ny;
                b2.wy -= counterSpinTransfer * nx;

                b1.state = "sliding";
                b2.state = "sliding";

                if (this.firstContactBallId === -1) {
                  if (b1.id === 0) this.firstContactBallId = b2.id;
                  else if (b2.id === 0) this.firstContactBallId = b1.id;
                }

                onImpact?.(Math.abs(vn));
              }
            }
          }
        }
      }
    }

    this.isQuiescent = !anyMoving;
  }

  public predictTrajectory(aimAngle: number, spinX = 0, spinY = 0): TrajectoryPreview {
    const cue = this.balls[0];
    if (!cue || cue.isSunk) {
      return {
        hasHit: false, ghostX: 0, ghostY: 0, targetBall: null,
        normX: 0, normY: 0, deflectX: 0, deflectY: 0,
        cuePoints: [], targetPoints: [],
      };
    }

    const r = this.BALL_RADIUS;
    const squirtAngleRad = -spinX * 0.035;
    const actualAzimuth = aimAngle + squirtAngleRad;
    const dirX = Math.cos(actualAzimuth);
    const dirY = Math.sin(actualAzimuth);

    let closestBall: BallState | null = null;
    let closestDist = Infinity;

    for (let i = 1; i < this.balls.length; ++i) {
      const b = this.balls[i];
      if (b.isSunk) continue;

      const drx = b.x - cue.x;
      const dry = b.y - cue.y;
      const dpar = drx * dirX + dry * dirY;
      if (dpar <= 0) continue;

      const dperpSq = (drx * drx + dry * dry) - dpar * dpar;
      const rSum = r * 2.0;

      if (dperpSq < rSum * rSum) {
        const s = dpar - Math.sqrt(rSum * rSum - dperpSq);
        if (s > 0 && s < closestDist) {
          closestDist = s;
          closestBall = b;
        }
      }
    }

    if (closestBall) {
      const ghostX = cue.x + dirX * closestDist;
      const ghostY = cue.y + dirY * closestDist;

      const normX = (closestBall.x - ghostX) / (r * 2.0);
      const normY = (closestBall.y - ghostY) / (r * 2.0);

      let tangX = -normY;
      let tangY = normX;
      if (tangX * dirX + tangY * dirY < 0) {
        tangX = -tangX;
        tangY = -tangY;
      }

      // Cut-induced throw calculation on object ball
      const cutAngleSin = Math.abs(tangX * dirX + tangY * dirY);
      const throwAngle = (0.032 * cutAngleSin) + (spinX * 0.025);
      const cosT = Math.cos(throwAngle);
      const sinT = Math.sin(throwAngle) * (tangX * dirX + tangY * dirY >= 0 ? 1 : -1);
      const objDirX = normX * cosT - normY * sinT;
      const objDirY = normX * sinT + normY * cosT;

      // Object ball raycast path to pocket or cushion
      let objLen = 0.8;
      let tObjX = Infinity;
      let tObjY = Infinity;
      if (objDirX > 1e-5) tObjX = (this.TABLE_WIDTH - r - closestBall.x) / objDirX;
      else if (objDirX < -1e-5) tObjX = (r - closestBall.x) / objDirX;
      if (objDirY > 1e-5) tObjY = (this.TABLE_HEIGHT - r - closestBall.y) / objDirY;
      else if (objDirY < -1e-5) tObjY = (r - closestBall.y) / objDirY;
      const tObjRail = Math.min(tObjX, tObjY);
      if (tObjRail > 0 && tObjRail < objLen) objLen = tObjRail;

      const targetPoints = [
        { x: closestBall.x, y: closestBall.y },
        { x: closestBall.x + objDirX * objLen, y: closestBall.y + objDirY * objLen },
      ];

      // Multi-step physical forward trace of cue ball post-impact
      // Determine sliding vs natural rolling state at moment of collision
      const v0 = 2.8;
      const slideDistToRoll = 0.38 * (1.0 - Math.min(0.85, spinY));
      let effectiveSpinY = spinY;
      if (closestDist >= slideDistToRoll && spinY >= 0) {
        // Natural forward roll reached (30-degree rule)
        effectiveSpinY = 0.55;
      } else if (spinY < 0) {
        // Draw spin depletion by sliding friction over distance
        const maxDrawDist = 0.65 * Math.abs(spinY);
        if (closestDist >= maxDrawDist) {
          effectiveSpinY = 0.15; // transitioned to forward roll
        } else {
          effectiveSpinY = spinY * (1.0 - closestDist / maxDrawDist);
        }
      }

      // Initial post-impact tangent velocity and spin vector
      const vTang = Math.max(0.2, v0 * (dirX * tangX + dirY * tangY));
      let curX = ghostX;
      let curY = ghostY;
      let curVx = vTang * tangX;
      let curVy = vTang * tangY;
      const omegaMag = (2.5 / r) * effectiveSpinY * v0;
      let curWx = -omegaMag * Math.sin(actualAzimuth);
      let curWy =  omegaMag * Math.cos(actualAzimuth);

      const cuePoints: { x: number; y: number }[] = [
        { x: cue.x, y: cue.y },
        { x: ghostX, y: ghostY },
      ];

      const simDt = 0.022;
      for (let step = 0; step < 16; ++step) {
        curX += curVx * simDt;
        curY += curVy * simDt;

        // Relative contact point velocity and felt friction curve
        const cpVx = curVx - curWy * r;
        const cpVy = curVy + curWx * r;
        const cpSpeed = Math.hypot(cpVx, cpVy);
        if (cpSpeed > 0.05) {
          const fNorm = -this.SLIDING_FRICTION * this.GRAVITY;
          curVx += fNorm * (cpVx / cpSpeed) * simDt;
          curVy += fNorm * (cpVy / cpSpeed) * simDt;
          curWx += (2.5 / r) * (fNorm * (cpVy / cpSpeed)) * simDt;
          curWy -= (2.5 / r) * (fNorm * (cpVx / cpSpeed)) * simDt;
        }

        // Cushion bounce reflection if hitting boundary
        if (curX <= r || curX >= this.TABLE_WIDTH - r) {
          curVx = -curVx * 0.75;
          curX = Math.max(r, Math.min(this.TABLE_WIDTH - r, curX));
        }
        if (curY <= r || curY >= this.TABLE_HEIGHT - r) {
          curVy = -curVy * 0.75;
          curY = Math.max(r, Math.min(this.TABLE_HEIGHT - r, curY));
        }

        cuePoints.push({ x: curX, y: curY });
        if (Math.hypot(curVx, curVy) < 0.12) break;
      }

      const deflectEnd = cuePoints[cuePoints.length - 1];
      const deflectX = (deflectEnd.x - ghostX) || tangX;
      const deflectY = (deflectEnd.y - ghostY) || tangY;
      const dLen = Math.hypot(deflectX, deflectY) || 1;

      return {
        hasHit: true,
        ghostX,
        ghostY,
        targetBall: closestBall,
        normX,
        normY,
        deflectX: deflectX / dLen,
        deflectY: deflectY / dLen,
        cuePoints,
        targetPoints,
      };
    }

    // Cushion Rail Raycasting when no ball is directly in line
    let tX = Infinity;
    let tY = Infinity;

    if (dirX > 1e-6) tX = (this.TABLE_WIDTH - r - cue.x) / dirX;
    else if (dirX < -1e-6) tX = (r - cue.x) / dirX;

    if (dirY > 1e-6) tY = (this.TABLE_HEIGHT - r - cue.y) / dirY;
    else if (dirY < -1e-6) tY = (r - cue.y) / dirY;

    const tRail = Math.min(tX, tY);
    if (tRail > 0 && tRail < 10) {
      const railX = cue.x + dirX * tRail;
      const railY = cue.y + dirY * tRail;

      // Calculate rebound direction with authentic running English
      let reboundDirX = dirX;
      let reboundDirY = dirY;
      if (tX < tY) {
        // Vertical rail (Left or Right)
        reboundDirX = -dirX;
        // Right english (spinX > 0) runs down (+Y) on right rail, runs down (-Y check) on left rail
        const englishSign = (dirX > 0) ? 1 : -1;
        reboundDirY = dirY + spinX * 0.18 * englishSign;
      } else {
        // Horizontal rail (Top or Bottom)
        const englishSign = (dirY < 0) ? 1 : -1;
        reboundDirX = dirX + spinX * 0.18 * englishSign;
        reboundDirY = -dirY;
      }
      const reboundLen = Math.hypot(reboundDirX, reboundDirY) || 1;
      reboundDirX /= reboundLen;
      reboundDirY /= reboundLen;

      // Raycast secondary bank to second cushion
      let t2X = Infinity;
      let t2Y = Infinity;
      if (reboundDirX > 1e-6) t2X = (this.TABLE_WIDTH - r - railX) / reboundDirX;
      else if (reboundDirX < -1e-6) t2X = (r - railX) / reboundDirX;
      if (reboundDirY > 1e-6) t2Y = (this.TABLE_HEIGHT - r - railY) / reboundDirY;
      else if (reboundDirY < -1e-6) t2Y = (r - railY) / reboundDirY;
      const t2Rail = Math.min(t2X, t2Y);
      const bank2Len = (t2Rail > 0 && t2Rail < 1.2) ? t2Rail : 0.8;

      return {
        hasHit: false,
        ghostX: railX,
        ghostY: railY,
        targetBall: null,
        normX: 0,
        normY: 0,
        deflectX: reboundDirX,
        deflectY: reboundDirY,
        cuePoints: [
          { x: cue.x, y: cue.y },
          { x: railX, y: railY },
          { x: railX + reboundDirX * bank2Len, y: railY + reboundDirY * bank2Len },
        ],
        targetPoints: [],
      };
    }

    return {
      hasHit: false,
      ghostX: 0,
      ghostY: 0,
      targetBall: null,
      normX: 0,
      normY: 0,
      deflectX: 0,
      deflectY: 0,
      cuePoints: [
        { x: cue.x, y: cue.y },
        { x: cue.x + dirX * 1.5, y: cue.y + dirY * 1.5 },
      ],
      targetPoints: [],
    };
  }

  public getLegendRecommendation(): LegendRecommendation {
    const cue = this.balls[0];
    const unsunk = this.balls.filter((b) => b.id > 0 && !b.isSunk);

    if (unsunk.length === 0) {
      return {
        legendName: "Coach Puddin",
        advice: "Table is clear, son! Pure perfection.",
        recommendedAimAngle: 0,
        recommendedPower: 1,
        recommendedSpin: [0, 0],
        previewPoints: [],
      };
    }

    const lowest = this.getLowestBall();
    const target = this.balls.find(b => b.id === lowest && !b.isSunk) || unsunk[0];

    const dx = target.x - cue.x;
    const dy = target.y - cue.y;
    const aim = Math.atan2(dy, dx);
    const dist = Math.hypot(dx, dy);

    if (dist > 1.2) {
      return {
        legendName: "Efren 'The Magician' Reyes",
        advice: `Efren Reyes: 'Lowest ball is the ${lowest}-ball. Play the 3-cushion bank off the top rail with running English!'`,
        recommendedAimAngle: aim - 0.35,
        recommendedPower: 4.2,
        recommendedSpin: [0.3, 0.1],
        previewPoints: [
          { x: cue.x, y: cue.y },
          { x: cue.x + 0.6, y: 0.03 },
          { x: this.TABLE_WIDTH - 0.05, y: 0.7 },
          { x: target.x, y: target.y },
        ],
      };
    }

    return {
      legendName: "Willie Mosconi",
      advice: `Willie Mosconi: 'Line up right on that ${lowest}-ball. Control your stroke speed and leave yourself an angle for the runout.'`,
      recommendedAimAngle: aim,
      recommendedPower: 2.8,
      recommendedSpin: [0.0, -0.2],
      previewPoints: [
        { x: cue.x, y: cue.y },
        { x: target.x, y: target.y },
      ],
    };
  }
}
