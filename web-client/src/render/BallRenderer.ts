/**
 * BallRenderer: 3D Procedural Spherical Billiards Ball Mapping
 * Phenolic Aramith colors, rotating stripe patterns, 3D quaternion rotation,
 * and high-gloss specular lighting.
 */

import { BallState } from '../physics/WebBilliardsEngine';

export interface BallColorDef {
  color: string;
  isStripe: boolean;
}

export const ARAMITH_BALL_COLORS: { [id: number]: BallColorDef } = {
  0: { color: '#ffffff', isStripe: false }, // Cue Ivory
  1: { color: '#fbbf24', isStripe: false }, // 1: Yellow Solid
  2: { color: '#2563eb', isStripe: false }, // 2: Blue Solid
  3: { color: '#dc2626', isStripe: false }, // 3: Red Solid
  4: { color: '#7c3aed', isStripe: false }, // 4: Purple Solid
  5: { color: '#f97316', isStripe: false }, // 5: Orange Solid
  6: { color: '#16a34a', isStripe: false }, // 6: Green Solid
  7: { color: '#881337', isStripe: false }, // 7: Maroon Solid
  8: { color: '#09090b', isStripe: false }, // 8: Black 8-Ball
  9: { color: '#fbbf24', isStripe: true },  // 9: Yellow Stripe
  10: { color: '#2563eb', isStripe: true }, // 10: Blue Stripe
  11: { color: '#dc2626', isStripe: true }, // 11: Red Stripe
  12: { color: '#7c3aed', isStripe: true }, // 12: Purple Stripe
  13: { color: '#f97316', isStripe: true }, // 13: Orange Stripe
  14: { color: '#16a34a', isStripe: true }, // 14: Green Stripe
  15: { color: '#881337', isStripe: true }, // 15: Maroon Stripe
};

export class BallRenderer {
  private ctx: CanvasRenderingContext2D;

  constructor(ctx: CanvasRenderingContext2D) {
    this.ctx = ctx;
  }

  public drawBalls(balls: BallState[], ballRadiusMeters: number, scale: number, isBallInHand: boolean): void {
    const ctx = this.ctx;
    const r = ballRadiusMeters * scale;

    for (const b of balls) {
      if (b.isSunk) continue;
      const bx = b.x * scale;
      const by = b.y * scale;

      // Ball-in-Hand target ring around cue ball
      if (b.id === 0 && isBallInHand) {
        ctx.save();
        ctx.strokeStyle = '#38bdf8';
        ctx.lineWidth = 3;
        ctx.setLineDash([4, 4]);
        ctx.beginPath();
        ctx.arc(bx, by, r * 1.5, 0, Math.PI * 2);
        ctx.stroke();
        ctx.restore();
      }

      ctx.save();
      ctx.translate(bx, by);

      // Compute rotation angle around Z-axis from quaternion
      // q = [x, y, z, w]
      const qx = b.rotQuaternion[0];
      const qy = b.rotQuaternion[1];
      const qz = b.rotQuaternion[2];
      const qw = b.rotQuaternion[3];
      const rollAngle = Math.atan2(2 * (qw * qz + qx * qy), 1 - 2 * (qy * qy + qz * qz));
      const pitchShift = Math.sin(rollAngle) * (r * 0.25);

      // Clip ball circle
      ctx.beginPath();
      ctx.arc(0, 0, r, 0, Math.PI * 2);
      ctx.clip();

      // Base Aramith Color
      const ballDef = ARAMITH_BALL_COLORS[b.id] || { color: '#ffffff', isStripe: false };

      if (ballDef.isStripe) {
        // Stripe ball: White phenolic sphere with centered colored equatorial belt
        ctx.fillStyle = '#f8fafc';
        ctx.fillRect(-r, -r, r * 2, r * 2);

        ctx.save();
        ctx.rotate(rollAngle);
        ctx.fillStyle = ballDef.color;
        ctx.fillRect(-r, -r * 0.48 + pitchShift, r * 2, r * 0.96);
        ctx.restore();
      } else {
        // Solid ball
        ctx.fillStyle = ballDef.color;
        ctx.fillRect(-r, -r, r * 2, r * 2);
      }

      // Number badge
      if (b.id > 0) {
        ctx.save();
        ctx.rotate(rollAngle);
        ctx.fillStyle = '#ffffff';
        ctx.beginPath();
        ctx.arc(0, pitchShift, r * 0.42, 0, Math.PI * 2);
        ctx.fill();

        ctx.fillStyle = '#0f172a';
        ctx.font = `bold ${Math.round(r * 0.46)}px sans-serif`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(b.id.toString(), 0, pitchShift);
        ctx.restore();
      }

      // 3D Spherical Specular Gloss Highlight
      const glossGrad = ctx.createRadialGradient(-r * 0.35, -r * 0.35, r * 0.05, 0, 0, r);
      glossGrad.addColorStop(0, 'rgba(255, 255, 255, 0.75)');
      glossGrad.addColorStop(0.3, 'rgba(255, 255, 255, 0.15)');
      glossGrad.addColorStop(0.8, 'rgba(0, 0, 0, 0.2)');
      glossGrad.addColorStop(1, 'rgba(0, 0, 0, 0.7)');

      ctx.fillStyle = glossGrad;
      ctx.beginPath();
      ctx.arc(0, 0, r, 0, Math.PI * 2);
      ctx.fill();

      ctx.restore();
    }
  }

  public drawCueStick(cue: BallState | undefined, aimAngle: number, power: number, scale: number, ballRadiusMeters: number): void {
    if (!cue || cue.isSunk) return;
    const ctx = this.ctx;
    const cx = cue.x * scale;
    const cy = cue.y * scale;
    const r = ballRadiusMeters * scale;

    const pullBack = 12 + power * 28;
    const stickDist = r + pullBack;
    const tipX = cx - Math.cos(aimAngle) * stickDist;
    const tipY = cy - Math.sin(aimAngle) * stickDist;
    const buttX = cx - Math.cos(aimAngle) * (stickDist + 160);
    const buttY = cy - Math.sin(aimAngle) * (stickDist + 160);

    ctx.save();
    // Tapered Maple Cue Shaft
    ctx.strokeStyle = '#d97706';
    ctx.lineWidth = 5;
    ctx.beginPath();
    ctx.moveTo(tipX, tipY);
    ctx.lineTo(buttX, buttY);
    ctx.stroke();

    // Cue Chalk Tip
    ctx.strokeStyle = '#38bdf8';
    ctx.lineWidth = 4;
    ctx.beginPath();
    ctx.moveTo(tipX, tipY);
    ctx.lineTo(tipX - Math.cos(aimAngle) * 4, tipY - Math.sin(aimAngle) * 4);
    ctx.stroke();
    ctx.restore();
  }
}
