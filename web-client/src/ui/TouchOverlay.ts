/**
 * TouchOverlay: Ergonomic Mobile Landscape Dual-Thumb Controller
 * Left Thumb: Smooth 0.02 deg Rotary Precision Aim Dial with High-Contrast Ticks & Micro-Haptics
 * Right Thumb: Elongated Vertical Pull-Back Tension Cue Track (0-100%) with Release-to-Shoot
 */

import { Haptics, ImpactStyle } from '@capacitor/haptics';

export class TouchOverlay {
  private container: HTMLElement;
  private dialCanvas: HTMLCanvasElement | null = null;
  private dialCtx: CanvasRenderingContext2D | null = null;

  // Rotary Dial State
  private isDraggingDial: boolean = false;
  private dialAngle: number = 0;
  private lastTouchAngle: number = 0;
  private lastHapticTickIndex: number = 0;

  // Cue Pull-Back State
  private isPullingCue: boolean = false;
  private startPullY: number = 0;
  private currentTension: number = 0; // 0.0 to 1.0
  private lastTensionNotch: number = 0;

  // DOM references
  private tensionBar: HTMLElement | null = null;
  private cueGrip: HTMLElement | null = null;
  private tensionText: HTMLElement | null = null;

  // Callbacks
  public onAimAdjusted?: (deltaRad: number) => void;
  public onStrokeFired?: (powerNorm: number) => void;
  public onTensionChanged?: (tensionNorm: number) => void;

  constructor(containerId: string) {
    const root = document.getElementById(containerId);
    if (!root) throw new Error(`TouchOverlay container #${containerId} not found`);
    this.container = root;

    this.render();
    this.initDialCanvas();
    this.setupListeners();
  }

  private render(): void {
    this.container.innerHTML = `
      <!-- Left Zone: Rotary Aim Precision Dial -->
      <div class="touch-dial-wrapper">
        <div id="touch-dial-box" class="touch-dial-canvas-box" title="Drag to fine-tune aim (0.02°/tick)">
          <canvas id="dial-canvas" class="touch-dial-canvas" width="220" height="220"></canvas>
          <div class="dial-center-hub">
            <span class="dial-hub-text">AIM</span>
            <span class="dial-hub-text" style="font-size: 7px; color: #94a3b8;">±0.02°</span>
          </div>
        </div>
        <span class="dial-label-tag">PRECISION DIAL</span>
      </div>

      <!-- Right Zone: Vertical Pull-Back Tension Cue Track -->
      <div class="touch-cue-track-wrapper">
        <div id="cue-tension-gauge" class="cue-tension-gauge" title="Pull down to load cue tension, release to shoot">
          <div id="cue-tension-bar" class="cue-tension-bar"></div>
          
          <div class="cue-notches-overlay">
            <div class="cue-notch-line"><span class="notch-mark"></span><span class="notch-txt">100%</span><span class="notch-mark"></span></div>
            <div class="cue-notch-line"><span class="notch-mark"></span><span class="notch-txt">75%</span><span class="notch-mark"></span></div>
            <div class="cue-notch-line"><span class="notch-mark"></span><span class="notch-txt">50%</span><span class="notch-mark"></span></div>
            <div class="cue-notch-line"><span class="notch-mark"></span><span class="notch-txt">25%</span><span class="notch-mark"></span></div>
            <div class="cue-notch-line"><span class="notch-mark"></span><span class="notch-txt">0%</span><span class="notch-mark"></span></div>
          </div>

          <div id="cue-grip-knob" class="cue-grip-knob">
            <div class="grip-ridge"></div>
          </div>
        </div>
        <span id="cue-tension-readout" class="cue-tension-readout">0%</span>
      </div>
    `;

    this.tensionBar = this.container.querySelector('#cue-tension-bar');
    this.cueGrip = this.container.querySelector('#cue-grip-knob');
    this.tensionText = this.container.querySelector('#cue-tension-readout');
  }

  private initDialCanvas(): void {
    this.dialCanvas = this.container.querySelector('#dial-canvas');
    if (!this.dialCanvas) return;
    this.dialCtx = this.dialCanvas.getContext('2d');
    this.drawDial();
  }

  private drawDial(): void {
    if (!this.dialCanvas || !this.dialCtx) return;
    const ctx = this.dialCtx;
    const w = this.dialCanvas.width;
    const h = this.dialCanvas.height;
    const cx = w * 0.5;
    const cy = h * 0.5;
    const radius = w * 0.46;

    ctx.clearRect(0, 0, w, h);
    ctx.save();
    ctx.translate(cx, cy);
    ctx.rotate(this.dialAngle);

    // Outer gear rim
    ctx.beginPath();
    ctx.arc(0, 0, radius, 0, Math.PI * 2);
    ctx.strokeStyle = 'rgba(251, 191, 36, 0.35)';
    ctx.lineWidth = 4;
    ctx.stroke();

    // High-contrast tick marks (36 radial ticks, every 10 degrees)
    const numTicks = 36;
    for (let i = 0; i < numTicks; ++i) {
      const angle = (i * Math.PI * 2) / numTicks;
      const isMajor = i % 3 === 0;
      const innerR = isMajor ? radius - 16 : radius - 9;
      const x1 = Math.cos(angle) * radius;
      const y1 = Math.sin(angle) * radius;
      const x2 = Math.cos(angle) * innerR;
      const y2 = Math.sin(angle) * innerR;

      ctx.beginPath();
      ctx.moveTo(x1, y1);
      ctx.lineTo(x2, y2);
      ctx.strokeStyle = isMajor ? '#fbbf24' : 'rgba(255, 255, 255, 0.4)';
      ctx.lineWidth = isMajor ? 3.0 : 1.5;
      ctx.stroke();
    }

    // Inner knurled texture ring
    ctx.beginPath();
    ctx.arc(0, 0, radius - 24, 0, Math.PI * 2);
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.15)';
    ctx.lineWidth = 2;
    ctx.stroke();

    ctx.restore();
  }

  private setupListeners(): void {
    const dialBox = this.container.querySelector('#touch-dial-box') as HTMLElement;
    const cueGauge = this.container.querySelector('#cue-tension-gauge') as HTMLElement;

    // 1. Rotary Aim Dial Listeners
    const getAngleFromCenter = (clientX: number, clientY: number): number => {
      const rect = dialBox.getBoundingClientRect();
      const cx = rect.left + rect.width * 0.5;
      const cy = rect.top + rect.height * 0.5;
      return Math.atan2(clientY - cy, clientX - cx);
    };

    dialBox.addEventListener('pointerdown', (e) => {
      e.stopPropagation();
      this.isDraggingDial = true;
      this.lastTouchAngle = getAngleFromCenter(e.clientX, e.clientY);
      try { dialBox.setPointerCapture(e.pointerId); } catch {}
    });

    dialBox.addEventListener('pointermove', (e) => {
      if (!this.isDraggingDial) return;
      e.stopPropagation();
      const currentAngle = getAngleFromCenter(e.clientX, e.clientY);
      let delta = currentAngle - this.lastTouchAngle;

      // Handle wraparound [-PI, PI]
      if (delta > Math.PI) delta -= Math.PI * 2;
      if (delta < -Math.PI) delta += Math.PI * 2;

      this.lastTouchAngle = currentAngle;
      this.dialAngle += delta;
      this.drawDial();

      // Precision scaling: 0.02 deg per small angular motion
      // A full 360 deg dial turn adjusts aiming by approx 10 degrees
      const microAimRad = delta * 0.035;
      if (this.onAimAdjusted) {
        this.onAimAdjusted(microAimRad);
      }

      // Micro-vibration tick on passing tick threshold
      const tickStep = (Math.PI * 2) / 36;
      const currentTickIdx = Math.floor(this.dialAngle / tickStep);
      if (currentTickIdx !== this.lastHapticTickIndex) {
        this.lastHapticTickIndex = currentTickIdx;
        this.triggerTickHaptic();
      }
    });

    const stopDial = (e: PointerEvent) => {
      if (this.isDraggingDial) {
        this.isDraggingDial = false;
        try { dialBox.releasePointerCapture(e.pointerId); } catch {}
      }
    };
    dialBox.addEventListener('pointerup', stopDial);
    dialBox.addEventListener('pointercancel', stopDial);

    // 2. Vertical Pull-Back Cue Tension Slider
    cueGauge.addEventListener('pointerdown', (e) => {
      e.stopPropagation();
      this.isPullingCue = true;
      const rect = cueGauge.getBoundingClientRect();
      this.startPullY = rect.top;
      this.updateCueTension(e.clientY, rect);
      try { cueGauge.setPointerCapture(e.pointerId); } catch {}
    });

    cueGauge.addEventListener('pointermove', (e) => {
      if (!this.isPullingCue) return;
      e.stopPropagation();
      const rect = cueGauge.getBoundingClientRect();
      this.updateCueTension(e.clientY, rect);
    });

    const releaseCue = (e: PointerEvent) => {
      if (this.isPullingCue) {
        this.isPullingCue = false;
        try { cueGauge.releasePointerCapture(e.pointerId); } catch {}

        const shotPower = this.currentTension;
        this.resetCueVisuals();

        if (shotPower >= 0.05) {
          this.triggerHeavyHaptic();
          if (this.onStrokeFired) {
            this.onStrokeFired(shotPower);
          }
        }
      }
    };
    cueGauge.addEventListener('pointerup', releaseCue);
    cueGauge.addEventListener('pointercancel', releaseCue);
  }

  private updateCueTension(clientY: number, rect: DOMRect): void {
    const trackHeight = rect.height - 34; // Allow knob height
    const offsetY = clientY - rect.top;
    const clampedY = Math.max(0, Math.min(trackHeight, offsetY));
    this.currentTension = clampedY / trackHeight;

    // Update UI elements
    if (this.tensionBar) {
      this.tensionBar.style.height = `${(this.currentTension * 100).toFixed(1)}%`;
    }
    if (this.cueGrip) {
      this.cueGrip.style.top = `${clampedY + 6}px`;
    }
    if (this.tensionText) {
      const pct = Math.round(this.currentTension * 100);
      this.tensionText.textContent = `${pct}%`;
    }

    // Haptic feedback at 25%, 50%, 75%, 100% notches
    const notch = Math.floor(this.currentTension * 4);
    if (notch !== this.lastTensionNotch) {
      this.lastTensionNotch = notch;
      this.triggerTickHaptic();
    }

    if (this.onTensionChanged) {
      this.onTensionChanged(this.currentTension);
    }
  }

  private resetCueVisuals(): void {
    this.currentTension = 0;
    this.lastTensionNotch = 0;
    if (this.tensionBar) {
      this.tensionBar.style.height = '0%';
    }
    if (this.cueGrip) {
      this.cueGrip.style.top = '6px';
    }
    if (this.tensionText) {
      this.tensionText.textContent = '0%';
    }
  }

  private async triggerTickHaptic(): Promise<void> {
    try {
      await Haptics.impact({ style: ImpactStyle.Light });
    } catch {
      if (navigator.vibrate) navigator.vibrate(5);
    }
  }

  private async triggerHeavyHaptic(): Promise<void> {
    try {
      await Haptics.impact({ style: ImpactStyle.Heavy });
    } catch {
      if (navigator.vibrate) navigator.vibrate(25);
    }
  }
}
