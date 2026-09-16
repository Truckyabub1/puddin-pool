/**
 * Universal Analog Stroke & Spin Controller
 * Mode A: Miniclip pull-back spring slider
 * Mode B: Pro physical manual velocity release stroke (v = dy / dt)
 * Gamepad & Keyboard (0.02 deg micro-aiming) & 3D tip HUD with Capacitor haptics.
 */

import { Haptics, ImpactStyle } from '@capacitor/haptics';

export enum StrokeMode {
  PullBack = 'PullBack',
  ProPhysical = 'ProPhysical',
}

export interface StrokeEvent {
  power: number;      // 0.0 to 1.0 normalized
  impulseMps: number; // 0.8 to 9.5 m/s
  spinX: number;      // -1.0 to 1.0
  spinY: number;      // -1.0 to 1.0
  elevationDeg: number; // 2.5 to 15.0 deg
}

export class StrokeController {
  private container: HTMLElement;
  private mode: StrokeMode = StrokeMode.PullBack;

  private englishX: number = 0.0;
  private englishY: number = 0.0;
  private elevationDeg: number = 2.5;

  // Pro Physical stroke velocity tracking
  private isProStroking: boolean = false;
  private lastStrokeY: number = 0;
  private lastStrokeTime: number = 0;
  private maxForwardVelocity: number = 0;

  // Pull back state
  private pullRatio: number = 0.5;
  private isPulling: boolean = false;
  private lastHapticNotch: number = -1;

  public onStrokeFired?: (event: StrokeEvent) => void;
  public onAimAdjusted?: (deltaRad: number) => void;
  public onElevationChanged?: (elevationDeg: number) => void;
  public onSpinChanged?: (spinX: number, spinY: number) => void;
  public onTensionChanged?: (tensionNorm: number) => void;

  constructor(containerId: string) {
    const root = document.getElementById(containerId);
    if (!root) throw new Error(`Container #${containerId} not found`);
    this.container = root;

    this.renderUI();
    this.setupListeners();
    this.setupGamepadLoop();
  }

  private renderUI(): void {
    this.container.innerHTML = `
      <div class="stroke-controller-root">
        <!-- 3D Tip-Spin & Elevation HUD -->
        <div class="tip-elevation-panel">
          <div class="panel-header">
            <span class="hud-pill-label">3D TIP & TILT</span>
            <div class="mode-toggle-group">
              <button id="btn-mode-pull" class="mode-btn active" title="Miniclip Pull-Back">SPRING</button>
              <button id="btn-mode-pro" class="mode-btn" title="Pro Analog Velocity Stroke">PRO</button>
            </div>
          </div>

          <div class="tip-3d-wrapper">
            <div id="tip-sphere" class="tip-sphere">
              <div class="sphere-grid-lat"></div>
              <div class="sphere-grid-long"></div>
              <div id="chalk-tip-dot" class="chalk-tip-dot"></div>
            </div>
            <div class="elevation-gauge">
              <span class="elevation-label">TILT: <strong id="elevation-val">2.5°</strong></span>
              <input type="range" id="elevation-slider" class="elevation-slider" min="2.5" max="15.0" step="0.5" value="2.5">
            </div>
          </div>
          <div class="tip-readout" id="tip-readout">Tip: (0.00, 0.00)</div>
        </div>

        <!-- Dynamic Stroke Track -->
        <div class="stroke-track-panel">
          <!-- Mode A: Pull-Back Spring Tray -->
          <div id="pullback-tray" class="pullback-tray">
            <div class="pull-labels">
              <span>CUE POWER</span>
              <span id="pull-val" class="pull-val">50%</span>
            </div>
            <input type="range" id="pull-slider" class="pull-slider" min="0.05" max="1.0" step="0.01" value="0.5">
            <button id="btn-fire-pull" class="fire-btn">⚡ STRIKE</button>
          </div>

          <!-- Mode B: Pro Physical Analog Swipe Zone -->
          <div id="pro-stroke-zone" class="pro-stroke-zone hidden">
            <div class="pro-indicator" id="pro-cue-grip"></div>
            <span class="pro-guide-text">↕ FLUID STROKE (Drag Up & Snap Forward)</span>
          </div>
        </div>
      </div>
    `;
  }

  private setupListeners(): void {
    // Mode Switcher
    const btnPull = this.container.querySelector('#btn-mode-pull') as HTMLButtonElement;
    const btnPro = this.container.querySelector('#btn-mode-pro') as HTMLButtonElement;
    const trayPull = this.container.querySelector('#pullback-tray') as HTMLElement;
    const zonePro = this.container.querySelector('#pro-stroke-zone') as HTMLElement;

    btnPull.addEventListener('click', () => {
      this.mode = StrokeMode.PullBack;
      btnPull.classList.add('active');
      btnPro.classList.remove('active');
      trayPull.classList.remove('hidden');
      zonePro.classList.add('hidden');
    });

    btnPro.addEventListener('click', () => {
      this.mode = StrokeMode.ProPhysical;
      btnPro.classList.add('active');
      btnPull.classList.remove('active');
      zonePro.classList.remove('hidden');
      trayPull.classList.add('hidden');
    });

    // 3D Tip Sphere dragging
    const tipSphere = this.container.querySelector('#tip-sphere') as HTMLElement;
    const tipDot = this.container.querySelector('#chalk-tip-dot') as HTMLElement;
    const tipReadout = this.container.querySelector('#tip-readout') as HTMLElement;

    let isDraggingTip = false;
    const updateTipFromPointer = (clientX: number, clientY: number) => {
      const rect = tipSphere.getBoundingClientRect();
      const radius = rect.width * 0.5;
      const cx = rect.left + radius;
      const cy = rect.top + radius;

      let dx = (clientX - cx) / radius;
      let dy = (clientY - cy) / radius;
      const dist = Math.hypot(dx, dy);
      if (dist > 0.85) {
        dx = (dx / dist) * 0.85;
        dy = (dy / dist) * 0.85;
      }

      this.englishX = parseFloat(dx.toFixed(2));
      this.englishY = parseFloat((-dy).toFixed(2)); // Up is follow (+)

      tipDot.style.transform = `translate(${dx * radius}px, ${dy * radius}px)`;
      tipReadout.textContent = `Tip: (${this.englishX.toFixed(2)}, ${this.englishY.toFixed(2)})`;

      if (this.onSpinChanged) {
        this.onSpinChanged(this.englishX, this.englishY);
      }
    };

    tipSphere.addEventListener('pointerdown', (e) => {
      isDraggingTip = true;
      tipSphere.setPointerCapture(e.pointerId);
      updateTipFromPointer(e.clientX, e.clientY);
    });

    tipSphere.addEventListener('pointermove', (e) => {
      if (isDraggingTip) updateTipFromPointer(e.clientX, e.clientY);
    });

    const stopTip = (e: PointerEvent) => {
      if (isDraggingTip) {
        isDraggingTip = false;
        try { tipSphere.releasePointerCapture(e.pointerId); } catch {}
      }
    };
    tipSphere.addEventListener('pointerup', stopTip);
    tipSphere.addEventListener('pointercancel', stopTip);

    // Elevation Slider
    const elevSlider = this.container.querySelector('#elevation-slider') as HTMLInputElement;
    const elevVal = this.container.querySelector('#elevation-val') as HTMLElement;
    elevSlider.addEventListener('input', () => {
      this.elevationDeg = parseFloat(elevSlider.value);
      elevVal.textContent = `${this.elevationDeg.toFixed(1)}°`;
      if (this.onElevationChanged) this.onElevationChanged(this.elevationDeg);
    });

    // Pull-Back Slider (Mode A)
    const pullSlider = this.container.querySelector('#pull-slider') as HTMLInputElement;
    const pullVal = this.container.querySelector('#pull-val') as HTMLElement;
    const fireBtn = this.container.querySelector('#btn-fire-pull') as HTMLButtonElement;

    pullSlider.addEventListener('input', () => {
      this.pullRatio = parseFloat(pullSlider.value);
      pullVal.textContent = `${Math.round(this.pullRatio * 100)}%`;
      if (this.onTensionChanged) this.onTensionChanged(this.pullRatio);

      // Haptic notch ramp
      const notch = Math.floor(this.pullRatio * 10);
      if (notch !== this.lastHapticNotch) {
        this.lastHapticNotch = notch;
        this.triggerHaptic(ImpactStyle.Light);
      }
    });

    fireBtn.addEventListener('click', () => {
      this.triggerHaptic(ImpactStyle.Heavy);
      this.fireStroke(this.pullRatio);
    });

    // Pro Physical Analog Stroke (Mode B)
    const proZone = this.container.querySelector('#pro-stroke-zone') as HTMLElement;
    const proGrip = this.container.querySelector('#pro-cue-grip') as HTMLElement;

    proZone.addEventListener('pointerdown', (e) => {
      this.isProStroking = true;
      this.lastStrokeY = e.clientY;
      this.lastStrokeTime = performance.now();
      this.maxForwardVelocity = 0;
      proZone.setPointerCapture(e.pointerId);
    });

    proZone.addEventListener('pointermove', (e) => {
      if (!this.isProStroking) return;
      const now = performance.now();
      const dt = (now - this.lastStrokeTime) / 1000.0;
      const dy = this.lastStrokeY - e.clientY; // Forward stroke is -Y (upward)

      if (dt > 0.008) {
        // Forward release velocity in pixels/second
        const forwardVel = dy / dt;
        if (forwardVel > this.maxForwardVelocity) {
          this.maxForwardVelocity = forwardVel;
        }
        this.lastStrokeY = e.clientY;
        this.lastStrokeTime = now;
      }

      // Visual grip feedback
      const rect = proZone.getBoundingClientRect();
      const relY = Math.max(0, Math.min(rect.height - 20, e.clientY - rect.top));
      proGrip.style.top = `${relY}px`;
    });

    const finishProStroke = (e: PointerEvent) => {
      if (this.isProStroking) {
        this.isProStroking = false;
        try { proZone.releasePointerCapture(e.pointerId); } catch {}
        proGrip.style.top = '50%';

        // Map max forward stroke velocity (approx 200px/s to 2500px/s) to 0.05 - 1.0 power
        if (this.maxForwardVelocity > 150) {
          const normPower = Math.min(1.0, Math.max(0.08, (this.maxForwardVelocity - 150) / 2000.0));
          this.triggerHaptic(ImpactStyle.Heavy);
          this.fireStroke(normPower);
        }
      }
    };
    proZone.addEventListener('pointerup', finishProStroke);
    proZone.addEventListener('pointercancel', finishProStroke);

    // Keyboard Micro-Aiming: Left/Right arrow keys = 0.02° per pulse
    window.addEventListener('keydown', (e) => {
      if (e.key === 'ArrowLeft') {
        if (this.onAimAdjusted) this.onAimAdjusted((-0.02 * Math.PI) / 180);
      } else if (e.key === 'ArrowRight') {
        if (this.onAimAdjusted) this.onAimAdjusted((0.02 * Math.PI) / 180);
      }
    });
  }

  private setupGamepadLoop(): void {
    const pollGamepad = () => {
      const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
      if (gamepads && gamepads[0]) {
        const gp = gamepads[0];
        // D-Pad micro aim
        if (gp.buttons[14] && gp.buttons[14].pressed) {
          if (this.onAimAdjusted) this.onAimAdjusted((-0.02 * Math.PI) / 180);
        } else if (gp.buttons[15] && gp.buttons[15].pressed) {
          if (this.onAimAdjusted) this.onAimAdjusted((0.02 * Math.PI) / 180);
        }

        // Right trigger pull
        if (gp.buttons[7] && gp.buttons[7].value > 0.1) {
          this.pullRatio = gp.buttons[7].value;
          const pullVal = this.container.querySelector('#pull-val');
          if (pullVal) pullVal.textContent = `${Math.round(this.pullRatio * 100)}%`;
        }
      }
      requestAnimationFrame(pollGamepad);
    };
    requestAnimationFrame(pollGamepad);
  }

  private fireStroke(powerNorm: number): void {
    // Impulse speed mapping: 0.8 m/s to 9.5 m/s
    const mps = 0.8 + powerNorm * 8.7;
    if (this.onStrokeFired) {
      this.onStrokeFired({
        power: powerNorm,
        impulseMps: mps,
        spinX: this.englishX,
        spinY: this.englishY,
        elevationDeg: this.elevationDeg,
      });
    }
  }

  public setTip(spinX: number, spinY: number): void {
    this.englishX = Math.max(-0.85, Math.min(0.85, spinX));
    this.englishY = Math.max(-0.85, Math.min(0.85, spinY));

    const tipDot = this.container.querySelector('#chalk-tip-dot') as HTMLElement;
    const tipReadout = this.container.querySelector('#tip-readout') as HTMLElement;
    const tipSphere = this.container.querySelector('#tip-sphere') as HTMLElement;

    if (tipDot && tipSphere) {
      const rect = tipSphere.getBoundingClientRect();
      const radius = (rect.width || 54) * 0.5;
      tipDot.style.transform = `translate(${this.englishX * radius}px, ${-this.englishY * radius}px)`;
    }
    if (tipReadout) {
      tipReadout.textContent = `Tip: (${this.englishX.toFixed(2)}, ${this.englishY.toFixed(2)})`;
    }
    if (this.onSpinChanged) {
      this.onSpinChanged(this.englishX, this.englishY);
    }
  }

  public getSpin(): { x: number; y: number } {
    return { x: this.englishX, y: this.englishY };
  }

  public getTension(): number {
    return this.pullRatio;
  }

  private async triggerHaptic(style: ImpactStyle): Promise<void> {
    try {
      await Haptics.impact({ style });
    } catch {
      if (navigator.vibrate) navigator.vibrate(10);
    }
  }
}
