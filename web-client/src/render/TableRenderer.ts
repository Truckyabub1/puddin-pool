/**
 * TableRenderer: High-Fidelity Billiards Table Presentation
 * Micro-fiber worsted cloth texture, contact ambient occlusion circles,
 * Ceulemans diamond inlays, and fixed 16:9 pillarbox/letterbox auto-centering.
 */

import { BallState, TrajectoryPreview, LegendRecommendation } from '../physics/WebBilliardsEngine';

export class TableRenderer {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  private container: HTMLElement;

  public scale: number = 1.0;
  public offsetX: number = 0;
  public offsetY: number = 0;
  public tableDrawW: number = 0;
  public tableDrawH: number = 0;
  public railThickness: number = 0;
  public tableWidthMeters: number = 1.9304;
  public tableHeightMeters: number = 0.9652;

  // Procedural felt micro-weave pattern
  private feltPatternCanvas: HTMLCanvasElement | null = null;
  private feltPattern: CanvasPattern | null = null;

  constructor(canvas: HTMLCanvasElement, container: HTMLElement) {
    this.canvas = canvas;
    this.container = container;
    this.ctx = canvas.getContext('2d', { alpha: false })!;
    this.ctx.imageSmoothingEnabled = true;
    this.ctx.imageSmoothingQuality = 'high';

    this.initFeltPattern();
  }

  private initFeltPattern(): void {
    // Generate procedural micro-fiber roughness tile
    this.feltPatternCanvas = document.createElement('canvas');
    this.feltPatternCanvas.width = 32;
    this.feltPatternCanvas.height = 32;
    const pCtx = this.feltPatternCanvas.getContext('2d')!;

    // Base Simonis tournament weave
    pCtx.fillStyle = '#0b6656';
    pCtx.fillRect(0, 0, 32, 32);

    // Micro-fiber noise texture
    const imgData = pCtx.createImageData(32, 32);
    for (let i = 0; i < imgData.data.length; i += 4) {
      const v = (Math.random() - 0.5) * 14;
      imgData.data[i] = 11 + v;     // R
      imgData.data[i + 1] = 102 + v; // G
      imgData.data[i + 2] = 86 + v;  // B
      imgData.data[i + 3] = 255;
    }
    pCtx.putImageData(imgData, 0, 0);

    this.feltPattern = this.ctx.createPattern(this.feltPatternCanvas, 'repeat');
  }

  /**
   * High-DPI buffer correction and strict 16:9 stage viewport letterbox/pillarbox
   */
  public resize(tableWidthMeters: number, tableHeightMeters: number): void {
    this.tableWidthMeters = tableWidthMeters;
    this.tableHeightMeters = tableHeightMeters;
    const dpr = Math.min(window.devicePixelRatio || 1, 2.0);
    const containerW = this.container.clientWidth || window.innerWidth;
    const containerH = this.container.clientHeight || window.innerHeight;

    // 1. Bind physical buffer to device pixels
    this.canvas.width = Math.floor(containerW * dpr);
    this.canvas.height = Math.floor(containerH * dpr);
    this.canvas.style.width = `${containerW}px`;
    this.canvas.style.height = `${containerH}px`;

    // 2. Lock to fixed 16:9 stage viewport with strict 2:1 table auto-centering
    const stageAspect = 16 / 9;
    let stageW = this.canvas.width;
    let stageH = stageW / stageAspect;

    if (stageH > this.canvas.height) {
      stageH = this.canvas.height;
      stageW = stageH * stageAspect;
    }

    const paddingX = 40 * dpr;
    const paddingY = 60 * dpr;
    const availW = stageW - paddingX * 2;
    const availH = stageH - paddingY * 2;

    const tableAspect = tableWidthMeters / tableHeightMeters; // 2.0:1
    let drawW = availW;
    let drawH = drawW / tableAspect;

    if (drawH > availH) {
      drawH = availH;
      drawW = drawH * tableAspect;
    }

    this.scale = drawW / tableWidthMeters;
    this.tableDrawW = drawW;
    this.tableDrawH = drawH;
    this.railThickness = 0.09 * this.scale;

    // Center table inside canvas viewport
    this.offsetX = Math.floor((this.canvas.width - this.tableDrawW) * 0.5);
    this.offsetY = Math.floor((this.canvas.height - this.tableDrawH) * 0.5);
  }

  public beginScene(): void {
    const ctx = this.ctx;
    ctx.save();
    // Dark mahogany lounge ambient background
    ctx.fillStyle = '#060911';
    ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

    ctx.translate(this.offsetX, this.offsetY);
  }

  public endScene(): void {
    this.ctx.restore();
  }

  public drawRails(pockets: { x: number; y: number; r: number }[]): void {
    const ctx = this.ctx;
    const tw = this.tableDrawW;
    const th = this.tableDrawH;
    const rail = this.railThickness;

    // 1. Mahogany Wood Rail with Bevel Shadow
    ctx.save();
    ctx.shadowColor = 'rgba(0, 0, 0, 0.85)';
    ctx.shadowBlur = 35;
    ctx.fillStyle = '#220e05';
    ctx.beginPath();
    ctx.roundRect(-rail, -rail, tw + rail * 2, th + rail * 2, 22);
    ctx.fill();
    ctx.restore();

    // 2. Inner Golden Rosewood Bezel
    ctx.strokeStyle = '#50240b';
    ctx.lineWidth = 2.5;
    ctx.strokeRect(-rail * 0.72, -rail * 0.72, tw + rail * 1.44, th + rail * 1.44);

    // 3. Ceulemans Billiards Diamond Inlays
    this.drawDiamondMarkers(tw, th, rail);

    // 4. Simonis Worsted Cloth Bed
    this.drawFelt(tw, th);

    // 5. Pockets with drop wells
    this.drawPockets(pockets);
  }

  private drawDiamondMarkers(tw: number, th: number, rail: number): void {
    const ctx = this.ctx;
    ctx.fillStyle = '#fef08a';
    const diamondR = 3.5;

    const drawD = (x: number, y: number) => {
      ctx.beginPath();
      ctx.moveTo(x, y - diamondR);
      ctx.lineTo(x + diamondR, y);
      ctx.lineTo(x, y + diamondR);
      ctx.lineTo(x - diamondR, y);
      ctx.closePath();
      ctx.fill();
    };

    // Long rails (7 diamonds)
    const longStep = tw / 8;
    for (let i = 1; i <= 7; ++i) {
      if (i === 4) continue;
      const x = i * longStep;
      drawD(x, -rail * 0.5);
      drawD(x, th + rail * 0.5);
    }

    // Short rails (3 diamonds)
    const shortStep = th / 4;
    for (let i = 1; i <= 3; ++i) {
      if (i === 2) continue;
      const y = i * shortStep;
      drawD(-rail * 0.5, y);
      drawD(tw + rail * 0.5, y);
    }
  }

  private drawFelt(tw: number, th: number): void {
    const ctx = this.ctx;

    // Fill felt base with procedural micro-fiber texture
    if (this.feltPattern) {
      ctx.fillStyle = this.feltPattern;
      ctx.fillRect(0, 0, tw, th);
    }

    // Simonis worsted radial lighting falloff
    const feltGrad = ctx.createRadialGradient(tw * 0.5, th * 0.5, th * 0.15, tw * 0.5, th * 0.5, tw * 0.72);
    feltGrad.addColorStop(0, 'rgba(14, 112, 96, 0.45)');
    feltGrad.addColorStop(1, 'rgba(4, 40, 34, 0.85)');
    ctx.fillStyle = feltGrad;
    ctx.fillRect(0, 0, tw, th);

    // Headstring line
    const headstringX = (this.tableWidthMeters * 0.25) * this.scale;
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.12)';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(headstringX, 0);
    ctx.lineTo(headstringX, th);
    ctx.stroke();

    // Foot spot
    ctx.fillStyle = 'rgba(255, 255, 255, 0.25)';
    ctx.beginPath();
    ctx.arc((this.tableWidthMeters * 0.70) * this.scale, th * 0.5, 3.5, 0, Math.PI * 2);
    ctx.fill();

    // Cushion rubber shadow
    ctx.fillStyle = 'rgba(0, 0, 0, 0.4)';
    ctx.fillRect(0, 0, tw, 6);
    ctx.fillRect(0, th - 6, tw, 6);
    ctx.fillRect(0, 0, 6, th);
    ctx.fillRect(tw - 6, 0, 6, th);
  }

  private drawPockets(pockets: { x: number; y: number; r: number }[]): void {
    const ctx = this.ctx;
    for (let i = 0; i < pockets.length; ++i) {
      const p = pockets[i];
      const px = p.x * this.scale;
      const py = p.y * this.scale;
      const pr = p.r * this.scale;
      const isSide = (i === 1 || i === 4);

      ctx.save();

      // 1. Heavy Outer Drop Shadow into Slate
      ctx.shadowColor = 'rgba(0, 0, 0, 0.9)';
      ctx.shadowBlur = 10;

      // 2. Pocket Drop Well (Recessed 3D Depth)
      const wellGrad = ctx.createRadialGradient(px, py, pr * 0.15, px, py, pr);
      wellGrad.addColorStop(0, '#000000');
      wellGrad.addColorStop(0.7, '#07090e');
      wellGrad.addColorStop(0.92, '#111827');
      wellGrad.addColorStop(1, '#1f2937');

      ctx.fillStyle = wellGrad;
      ctx.beginPath();
      ctx.arc(px, py, pr, 0, Math.PI * 2);
      ctx.fill();
      ctx.restore();

      // 3. Molded Rubber Pocket Liner Rim
      ctx.save();
      ctx.strokeStyle = '#18181b';
      ctx.lineWidth = isSide ? 2.5 : 3.0;
      ctx.beginPath();
      ctx.arc(px, py, pr, 0, Math.PI * 2);
      ctx.stroke();

      // 4. Anodized Tournament Brass Casting Bezel
      ctx.strokeStyle = 'rgba(217, 119, 6, 0.4)';
      ctx.lineWidth = 1.0;
      ctx.beginPath();
      ctx.arc(px, py, pr - 1.2, 0, Math.PI * 2);
      ctx.stroke();
      ctx.restore();
    }
  }

  /**
   * Soft-radius contact ambient occlusion circles directly beneath each active ball.
   */
  public drawContactAmbientOcclusion(balls: BallState[], ballRadiusMeters: number): void {
    const ctx = this.ctx;
    const r = ballRadiusMeters * this.scale;

    for (const b of balls) {
      if (b.isSunk) continue;
      const bx = b.x * this.scale;
      const by = b.y * this.scale;

      ctx.save();
      const shadowGrad = ctx.createRadialGradient(bx, by + 1.5, r * 0.3, bx, by + 1.5, r * 1.15);
      shadowGrad.addColorStop(0, 'rgba(0, 0, 0, 0.65)');
      shadowGrad.addColorStop(0.6, 'rgba(0, 0, 0, 0.25)');
      shadowGrad.addColorStop(1, 'rgba(0, 0, 0, 0.0)');

      ctx.fillStyle = shadowGrad;
      ctx.beginPath();
      ctx.ellipse(bx + 1.0, by + 3.0, r * 1.1, r * 0.75, 0, 0, Math.PI * 2);
      ctx.fill();
      ctx.restore();
    }
  }

  public drawTrajectory(traj: TrajectoryPreview | null, ballRadiusMeters: number): void {
    if (!traj) return;
    const ctx = this.ctx;

    // Laser aiming line and post-impact curve / cushion bank
    if (traj.cuePoints.length >= 2) {
      ctx.save();
      ctx.strokeStyle = '#38bdf8';
      ctx.lineWidth = 2;
      ctx.setLineDash([6, 4]);
      ctx.beginPath();
      ctx.moveTo(traj.cuePoints[0].x * this.scale, traj.cuePoints[0].y * this.scale);
      ctx.lineTo(traj.cuePoints[1].x * this.scale, traj.cuePoints[1].y * this.scale);
      ctx.stroke();

      if (traj.cuePoints.length > 2) {
        ctx.strokeStyle = 'rgba(56, 189, 248, 0.65)';
        ctx.lineWidth = 2;
        ctx.setLineDash([4, 3]);
        ctx.beginPath();
        ctx.moveTo(traj.cuePoints[1].x * this.scale, traj.cuePoints[1].y * this.scale);
        for (let i = 2; i < traj.cuePoints.length; ++i) {
          ctx.lineTo(traj.cuePoints[i].x * this.scale, traj.cuePoints[i].y * this.scale);
        }
        ctx.stroke();
      }
      ctx.restore();
    }

    // Ghost Ball
    if (traj.hasHit) {
      ctx.save();
      const gx = traj.ghostX * this.scale;
      const gy = traj.ghostY * this.scale;
      const gr = ballRadiusMeters * this.scale;

      // Outer anti-aliasing halo
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.25)';
      ctx.lineWidth = 4;
      ctx.beginPath();
      ctx.arc(gx, gy, gr, 0, Math.PI * 2);
      ctx.stroke();

      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 1.8;
      ctx.setLineDash([3, 3]);
      ctx.beginPath();
      ctx.arc(gx, gy, gr, 0, Math.PI * 2);
      ctx.stroke();

      if (traj.targetPoints.length >= 2) {
        ctx.strokeStyle = '#f59e0b';
        ctx.lineWidth = 2.5;
        ctx.setLineDash([]);
        ctx.beginPath();
        ctx.moveTo(traj.targetPoints[0].x * this.scale, traj.targetPoints[0].y * this.scale);
        ctx.lineTo(traj.targetPoints[1].x * this.scale, traj.targetPoints[1].y * this.scale);
        ctx.stroke();
      }
      ctx.restore();
    }
  }

  public drawLegendRoute(rec: LegendRecommendation | null): void {
    if (!rec || rec.previewPoints.length < 2) return;
    const ctx = this.ctx;
    const pts = rec.previewPoints;

    ctx.save();
    ctx.strokeStyle = '#fbbf24';
    ctx.lineWidth = 3;
    ctx.setLineDash([8, 6]);
    ctx.beginPath();
    ctx.moveTo(pts[0].x * this.scale, pts[0].y * this.scale);
    for (let i = 1; i < pts.length; i++) {
      ctx.lineTo(pts[i].x * this.scale, pts[i].y * this.scale);
    }
    ctx.stroke();
    ctx.setLineDash([]);

    const endPt = pts[pts.length - 1];
    ctx.fillStyle = '#fbbf24';
    ctx.beginPath();
    ctx.arc(endPt.x * this.scale, endPt.y * this.scale, 7, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();
  }
}
