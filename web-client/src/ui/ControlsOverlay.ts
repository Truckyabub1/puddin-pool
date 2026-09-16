/**
 * ControlsOverlay: Top Navigation Bar, Table Specification Selector (Valley 7-Foot vs Tourney 9-Foot),
 * WPA Match & Turn Tracker, Push-Out Button, and 0.05 deg Fine Aim Dial.
 */

import { Haptics, ImpactStyle } from '@capacitor/haptics';

export interface AimState {
  aimAngle: number; // Radians
  gameMode: string;
}

export class ControlsOverlay {
  private container: HTMLElement;
  private state: AimState = {
    aimAngle: 0,
    gameMode: '9ball',
  };

  private pushOutBtn: HTMLButtonElement;
  private fineLeftBtn: HTMLButtonElement;
  private fineRightBtn: HTMLButtonElement;
  private angleDisplay: HTMLElement;
  private gameModeSelect: HTMLSelectElement;
  private tableSelect: HTMLSelectElement;
  private turnDisplay: HTMLElement;
  private foulDisplay: HTMLElement;
  private bihBadge: HTMLElement;

  public onAimAngleChanged?: (angleRad: number) => void;
  public onPushOutCalled?: () => void;
  public onRerackRequested?: () => void;
  public onGameModeChanged?: (mode: string) => void;
  public onTableSizeChanged?: (lengthMeters: number, widthMeters: number) => void;

  constructor(containerId: string) {
    const root = document.getElementById(containerId);
    if (!root) throw new Error(`Container #${containerId} not found`);
    this.container = root;

    this.container.innerHTML = `
      <div class="controls-overlay">
        <!-- Top Nav Bar with Table Size & Mode Selectors -->
        <div class="top-nav-bar">
          <div class="top-selectors-group">
            <div class="mode-selector-wrapper">
              <label for="table-select" class="nav-label">TABLE:</label>
              <select id="table-select" class="game-select">
                <option value="valley7" selected>Valley 7-Ft Bar Box</option>
                <option value="tourney9">9-Ft Tournament</option>
              </select>
            </div>

            <div class="mode-selector-wrapper">
              <label for="mode-select" class="nav-label">RULES:</label>
              <select id="mode-select" class="game-select">
                <option value="9ball" selected>WPA 9-Ball</option>
                <option value="8ball">WPA 8-Ball</option>
              </select>
            </div>
          </div>

          <!-- WPA Match & Turn Tracker -->
          <div class="match-tracker-chip">
            <span id="turn-display" class="turn-text">P1 SHOOTING</span>
            <span id="foul-display" class="foul-pill">Fouls: P1:0 | P2:0</span>
            <span id="bih-badge" class="bih-badge hidden">BALL IN HAND</span>
          </div>

          <div class="top-actions-group">
            <button id="btn-push-out" class="nav-btn hidden" style="background: rgba(37, 99, 235, 0.85); border-color: #60a5fa;" title="Announce legal Push-Out shot">
              📢 Push-Out
            </button>

            <div class="fine-aim-stepper">
              <button id="btn-fine-left" class="fine-btn" title="Rotate CCW 0.05°">◀</button>
              <span id="angle-display" class="angle-text">0.00°</span>
              <button id="btn-fine-right" class="fine-btn" title="Rotate CW 0.05°">▶</button>
            </div>

            <button id="btn-rerack" class="nav-btn">🔄 Rack</button>
          </div>
        </div>
      </div>
    `;

    this.pushOutBtn = this.container.querySelector('#btn-push-out') as HTMLButtonElement;
    this.fineLeftBtn = this.container.querySelector('#btn-fine-left') as HTMLButtonElement;
    this.fineRightBtn = this.container.querySelector('#btn-fine-right') as HTMLButtonElement;
    this.angleDisplay = this.container.querySelector('#angle-display') as HTMLElement;
    this.gameModeSelect = this.container.querySelector('#mode-select') as HTMLSelectElement;
    this.tableSelect = this.container.querySelector('#table-select') as HTMLSelectElement;
    this.turnDisplay = this.container.querySelector('#turn-display') as HTMLElement;
    this.foulDisplay = this.container.querySelector('#foul-display') as HTMLElement;
    this.bihBadge = this.container.querySelector('#bih-badge') as HTMLElement;

    this.setupEvents();
  }

  private setupEvents(): void {
    // Table Specification Selector
    this.tableSelect.addEventListener('change', () => {
      if (this.onTableSizeChanged) {
        if (this.tableSelect.value === 'valley7') {
          // Valley 7-Foot: 1.9304m x 0.9652m
          this.onTableSizeChanged(1.9304, 0.9652);
        } else {
          // 9-Foot Tournament: 2.24m x 1.12m
          this.onTableSizeChanged(2.24, 1.12);
        }
      }
    });

    // Game Mode Selector
    this.gameModeSelect.addEventListener('change', () => {
      this.state.gameMode = this.gameModeSelect.value;
      if (this.onGameModeChanged) {
        this.onGameModeChanged(this.state.gameMode);
      }
    });

    // Rerack Button
    const rerackBtn = this.container.querySelector('#btn-rerack') as HTMLButtonElement;
    rerackBtn.addEventListener('click', () => {
      this.triggerHaptic(ImpactStyle.Medium);
      if (this.onRerackRequested) {
        this.onRerackRequested();
      }
    });

    // Push-Out Button
    this.pushOutBtn.addEventListener('click', () => {
      this.triggerHaptic(ImpactStyle.Medium);
      if (this.onPushOutCalled) {
        this.onPushOutCalled();
      }
    });

    // Fine Aim Buttons (+-0.05 degrees)
    const stepRad = (0.05 * Math.PI) / 180;
    this.fineLeftBtn.addEventListener('click', () => {
      this.adjustAimAngle(-stepRad);
      this.triggerHaptic(ImpactStyle.Light);
    });
    this.fineRightBtn.addEventListener('click', () => {
      this.adjustAimAngle(stepRad);
      this.triggerHaptic(ImpactStyle.Light);
    });
  }

  public setAimAngle(rad: number): void {
    let normalized = rad % (Math.PI * 2);
    if (normalized < 0) normalized += Math.PI * 2;
    this.state.aimAngle = normalized;
    const deg = (normalized * 180) / Math.PI;
    this.angleDisplay.textContent = `${deg.toFixed(2)}°`;
  }

  public adjustAimAngle(deltaRad: number): void {
    this.setAimAngle(this.state.aimAngle + deltaRad);
    if (this.onAimAngleChanged) {
      this.onAimAngleChanged(this.state.aimAngle);
    }
  }

  public getAimAngle(): number {
    return this.state.aimAngle;
  }

  public setPushOutVisible(visible: boolean): void {
    if (visible) {
      this.pushOutBtn.classList.remove('hidden');
    } else {
      this.pushOutBtn.classList.add('hidden');
    }
  }

  public setTurnInfo(playerText: string, foulText: string, ballInHand: boolean): void {
    this.turnDisplay.textContent = playerText;
    this.foulDisplay.textContent = foulText;
    if (ballInHand) {
      this.bihBadge.classList.remove('hidden');
    } else {
      this.bihBadge.classList.add('hidden');
    }
  }

  private async triggerHaptic(style: ImpactStyle): Promise<void> {
    try {
      await Haptics.impact({ style });
    } catch {
      if (navigator.vibrate) navigator.vibrate(10);
    }
  }
}
