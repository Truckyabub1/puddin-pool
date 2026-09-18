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
  private scoreDisplay: HTMLElement;
  private turnDisplay: HTMLElement;
  private foulDisplay: HTMLElement;
  private bihBadge: HTMLElement;

  // Miniclip iOS Match HUD elements
  private p1Card: HTMLElement;
  private p2Card: HTMLElement;
  private p1ScorePill: HTMLElement;
  private p2ScorePill: HTMLElement;
  private p1GroupPill: HTMLElement;
  private p2GroupPill: HTMLElement;
  private p1TimerCircle: SVGCircleElement;
  private p2TimerCircle: SVGCircleElement;
  private matchStatusBanner: HTMLElement;
  private accoladeToast: HTMLElement;
  private autorestartCard: HTMLElement;
  private autorestartTitle: HTMLElement;
  private autorestartSub: HTMLElement;
  private autorestartTimer: HTMLElement;
  private btnInstantRestart: HTMLButtonElement;
  private accoladeTimeout: number | null = null;

  private pushoutModal: HTMLElement;
  private pushoutDesc: HTMLElement;
  private btnPushoutAccept: HTMLButtonElement;
  private btnPushoutPass: HTMLButtonElement;

  private victoryModal: HTMLElement;
  private victoryTitle: HTMLElement;
  private victorySub: HTMLElement;
  private modalScoreP1: HTMLElement;
  private modalScoreP2: HTMLElement;
  private btnNextRack: HTMLButtonElement;

  public onAimAngleChanged?: (angleRad: number) => void;
  public onPushOutCalled?: () => void;
  public onPushOutResponseResolved?: (accept: boolean) => void;
  public onNextRackRequested?: () => void;
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
              <label for="mode-select" class="nav-label">VARIATION:</label>
              <select id="mode-select" class="game-select">
                <option value="9ball" selected>WPA 9-Ball</option>
                <option value="8ball">WPA 8-Ball</option>
                <option value="10ball">WPA 10-Ball</option>
                <option value="straight">14.1 Straight Pool</option>
                <option value="practice">Practice / Free-Play</option>
              </select>
            </div>
          </div>

          <!-- WPA Match & Turn Tracker -->
          <div class="match-tracker-chip">
            <span id="score-display" class="score-pill">P1: 0 | P2: 0</span>
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

            <button id="btn-install-app" class="nav-btn" style="background: rgba(16, 185, 129, 0.85); border-color: #34d399;" title="Install Puddin's Pool App to Device">📲 App</button>
            <button id="btn-rerack" class="nav-btn">🔄 Rack</button>
          </div>
        </div>

        <!-- Miniclip iOS Pro Match HUD Bar -->
        <div class="miniclip-hud-bar">
          <!-- Player 1 Profile Card -->
          <div id="p1-card" class="miniclip-player-card p1-active">
            <div class="avatar-ring-container">
              <svg class="timer-svg" viewBox="0 0 44 44">
                <circle class="timer-bg" cx="22" cy="22" r="18" />
                <circle id="p1-timer-circle" class="timer-progress" cx="22" cy="22" r="18" stroke-dasharray="113.1" stroke-dashoffset="0" />
              </svg>
              <div class="player-avatar-circle">🤠</div>
            </div>
            <div class="player-meta">
              <div class="player-name-row">
                <span class="player-name-text">PLAYER 1</span>
                <span id="p1-score-badge" class="player-score-badge">0</span>
              </div>
              <div id="p1-group-pill" class="player-group-badge">ROTATION</div>
            </div>
          </div>

          <!-- Center Match Status Pill -->
          <div class="match-center-column">
            <div class="match-vs-badge">VS</div>
            <div id="match-status-banner" class="match-status-banner">BREAK SHOT</div>
          </div>

          <!-- Player 2 Profile Card -->
          <div id="p2-card" class="miniclip-player-card">
            <div class="player-meta right-align">
              <div class="player-name-row">
                <span id="p2-score-badge" class="player-score-badge">0</span>
                <span class="player-name-text">PLAYER 2</span>
              </div>
              <div id="p2-group-pill" class="player-group-badge">ROTATION</div>
            </div>
            <div class="avatar-ring-container">
              <svg class="timer-svg" viewBox="0 0 44 44">
                <circle class="timer-bg" cx="22" cy="22" r="18" />
                <circle id="p2-timer-circle" class="timer-progress" cx="22" cy="22" r="18" stroke-dasharray="113.1" stroke-dashoffset="113.1" />
              </svg>
              <div class="player-avatar-circle">🎩</div>
            </div>
          </div>
        </div>

        <!-- Floating Shot Accolade Toast (Miniclip feedback banner) -->
        <div id="accolade-toast" class="accolade-toast hidden"></div>

        <!-- Auto-Restart Winner Banner (Miniclip 9-ball end & restart) -->
        <div id="autorestart-card" class="autorestart-card hidden">
          <div class="autorestart-trophy">🏆</div>
          <div class="autorestart-content">
            <h3 id="autorestart-title" class="autorestart-title">9-BALL POCKETED! PLAYER 1 WINS!</h3>
            <p id="autorestart-sub" class="autorestart-sub">Game ended • Starting new rack in <strong id="autorestart-timer">3</strong>s...</p>
          </div>
          <button id="btn-instant-restart" class="autorestart-btn">⚡ Restart Now</button>
        </div>

        <!-- Push-Out Decision Modal (WPA 9.4) -->
        <div id="pushout-modal" class="pushout-modal-backdrop hidden">
          <div class="pushout-modal-card">
            <div class="pushout-modal-header">
              <span class="pushout-icon">📢</span>
              <div>
                <h3 class="pushout-title">Push-Out Option</h3>
                <p id="pushout-desc" class="pushout-desc">Opponent played a Push-Out. Choose your shot option:</p>
              </div>
            </div>
            <div class="pushout-actions">
              <button id="btn-pushout-accept" class="modal-primary-btn">Accept & Shoot</button>
              <button id="btn-pushout-pass" class="modal-secondary-btn">Pass Back to Shooter</button>
            </div>
          </div>
        </div>

        <!-- Victory / Rack Won Modal -->
        <div id="victory-modal" class="victory-modal-backdrop hidden">
          <div class="victory-modal-card">
            <div class="victory-trophy">🏆</div>
            <h2 id="victory-title" class="victory-title">PLAYER 1 WINS THE RACK!</h2>
            <p id="victory-sub" class="victory-sub">9-Ball Pocketed Legally</p>
            <div class="victory-score-display">
              <span id="modal-score-p1" class="victory-score-num">P1: 1</span>
              <span class="victory-score-sep">-</span>
              <span id="modal-score-p2" class="victory-score-num">P2: 0</span>
            </div>
            <button id="btn-next-rack" class="modal-primary-btn">▶ Next Rack (Winner Breaks)</button>
          </div>
        </div>

        <!-- Universal App Install & Sideload Modal -->
        <div id="install-modal" class="install-modal-backdrop hidden">
          <div class="install-modal-card">
            <div class="install-modal-header">
              <div class="install-modal-title">
                <span class="install-header-icon">📲</span>
                <div>
                  <h3 class="install-header-h3">Install Puddin's Pool</h3>
                  <p class="install-header-sub">100% Free • Standalone 120Hz • No Account Needed</p>
                </div>
              </div>
              <button id="btn-close-install" class="install-close-btn" title="Close Modal">✕</button>
            </div>

            <div class="install-tabs">
              <button class="install-tab active" data-tab="tab-ios">🍏 iOS (Apple)</button>
              <button class="install-tab" data-tab="tab-android-pwa">🤖 Android PWA</button>
              <button class="install-tab" data-tab="tab-android-apk">📦 Direct APK</button>
            </div>

            <div class="install-tab-content active" id="tab-ios">
              <div class="step-guide">
                <div class="step-row">
                  <span class="step-badge">1</span>
                  <span>Open this page in <strong>Safari</strong> on your iPhone or iPad.</span>
                </div>
                <div class="step-row">
                  <span class="step-badge">2</span>
                  <span>Tap the <strong>Share</strong> button (box with up arrow ⎋) at bottom/top.</span>
                </div>
                <div class="step-row">
                  <span class="step-badge">3</span>
                  <span>Scroll down and tap <strong>Add to Home Screen</strong>.</span>
                </div>
              </div>
              <div class="install-badge-note">
                ✨ Launches full-screen in standalone 120Hz mode with zero browser bars. Completely free and anonymous.
              </div>
            </div>

            <div class="install-tab-content" id="tab-android-pwa">
              <div class="step-guide">
                <div class="step-row">
                  <span class="step-badge">⚡</span>
                  <span>Install directly to your Android device via Chrome's native WebAPK engine.</span>
                </div>
              </div>
              <button id="btn-trigger-pwa" class="modal-primary-btn">📲 Tap to Install to Android</button>
              <div class="install-badge-note">
                Integrates into your Android App Drawer, home screen, and works 100% offline.
              </div>
            </div>

            <div class="install-tab-content" id="tab-android-apk">
              <div class="step-guide">
                <div class="step-row">
                  <span class="step-badge">📥</span>
                  <span>Download the standalone native Android package (.APK) directly without Google Play.</span>
                </div>
              </div>
              <a id="btn-download-apk" href="https://github.com/Truckyabub1/puddin-pool/releases/latest/download/PuddinsPool.apk" target="_blank" rel="noopener noreferrer" class="modal-primary-btn apk-highlight-btn">⬇️ Download Standalone APK</a>
              <div class="install-badge-note">
                Works on any Android 8.0+ device. Sideload directly with zero accounts, zero fees, and zero tracking.
              </div>
            </div>
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
    this.scoreDisplay = this.container.querySelector('#score-display') as HTMLElement;
    this.turnDisplay = this.container.querySelector('#turn-display') as HTMLElement;
    this.foulDisplay = this.container.querySelector('#foul-display') as HTMLElement;
    this.bihBadge = this.container.querySelector('#bih-badge') as HTMLElement;

    // Miniclip iOS Player HUD bindings
    this.p1Card = this.container.querySelector('#p1-card') as HTMLElement;
    this.p2Card = this.container.querySelector('#p2-card') as HTMLElement;
    this.p1ScorePill = this.container.querySelector('#p1-score-badge') as HTMLElement;
    this.p2ScorePill = this.container.querySelector('#p2-score-badge') as HTMLElement;
    this.p1GroupPill = this.container.querySelector('#p1-group-pill') as HTMLElement;
    this.p2GroupPill = this.container.querySelector('#p2-group-pill') as HTMLElement;
    this.p1TimerCircle = this.container.querySelector('#p1-timer-circle') as unknown as SVGCircleElement;
    this.p2TimerCircle = this.container.querySelector('#p2-timer-circle') as unknown as SVGCircleElement;
    this.matchStatusBanner = this.container.querySelector('#match-status-banner') as HTMLElement;
    this.accoladeToast = this.container.querySelector('#accolade-toast') as HTMLElement;
    this.autorestartCard = this.container.querySelector('#autorestart-card') as HTMLElement;
    this.autorestartTitle = this.container.querySelector('#autorestart-title') as HTMLElement;
    this.autorestartSub = this.container.querySelector('#autorestart-sub') as HTMLElement;
    this.autorestartTimer = this.container.querySelector('#autorestart-timer') as HTMLElement;
    this.btnInstantRestart = this.container.querySelector('#btn-instant-restart') as HTMLButtonElement;

    this.pushoutModal = this.container.querySelector('#pushout-modal') as HTMLElement;
    this.pushoutDesc = this.container.querySelector('#pushout-desc') as HTMLElement;
    this.btnPushoutAccept = this.container.querySelector('#btn-pushout-accept') as HTMLButtonElement;
    this.btnPushoutPass = this.container.querySelector('#btn-pushout-pass') as HTMLButtonElement;

    this.victoryModal = this.container.querySelector('#victory-modal') as HTMLElement;
    this.victoryTitle = this.container.querySelector('#victory-title') as HTMLElement;
    this.victorySub = this.container.querySelector('#victory-sub') as HTMLElement;
    this.modalScoreP1 = this.container.querySelector('#modal-score-p1') as HTMLElement;
    this.modalScoreP2 = this.container.querySelector('#modal-score-p2') as HTMLElement;
    this.btnNextRack = this.container.querySelector('#btn-next-rack') as HTMLButtonElement;

    this.setupEvents();
  }

  private setupEvents(): void {
    // Instant Restart on Auto-Restart Banner
    this.btnInstantRestart.addEventListener('click', (e) => {
      e.stopPropagation();
      this.triggerHaptic(ImpactStyle.Medium);
      this.hideAutoRestartBanner();
      if (this.onNextRackRequested) {
        this.onNextRackRequested();
      }
    });

    // Push-Out Modal Responses (WPA 9.4)
    this.btnPushoutAccept.addEventListener('click', (e) => {
      e.stopPropagation();
      this.triggerHaptic(ImpactStyle.Medium);
      this.hidePushOutModal();
      if (this.onPushOutResponseResolved) {
        this.onPushOutResponseResolved(true);
      }
    });

    this.btnPushoutPass.addEventListener('click', (e) => {
      e.stopPropagation();
      this.triggerHaptic(ImpactStyle.Medium);
      this.hidePushOutModal();
      if (this.onPushOutResponseResolved) {
        this.onPushOutResponseResolved(false);
      }
    });

    // Victory Modal Next Rack
    this.btnNextRack.addEventListener('click', (e) => {
      e.stopPropagation();
      this.triggerHaptic(ImpactStyle.Medium);
      this.hideVictoryModal();
      if (this.onNextRackRequested) {
        this.onNextRackRequested();
      }
    });

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

    // Universal Install & Sideload Modal Logic
    const installBtn = this.container.querySelector('#btn-install-app') as HTMLButtonElement;
    const installModal = this.container.querySelector('#install-modal') as HTMLElement;
    const closeModalBtn = this.container.querySelector('#btn-close-install') as HTMLButtonElement;
    const tabBtns = this.container.querySelectorAll<HTMLButtonElement>('.install-tab');
    const tabContents = this.container.querySelectorAll<HTMLElement>('.install-tab-content');
    const triggerPwaBtn = this.container.querySelector('#btn-trigger-pwa') as HTMLButtonElement;

    let deferredPrompt: any = null;

    window.addEventListener('beforeinstallprompt', (e: Event) => {
      e.preventDefault();
      deferredPrompt = e;
    });

    // Auto-select tab based on device OS
    const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent) && !(window as any).MSStream;
    const isAndroid = /Android/.test(navigator.userAgent);

    const selectTab = (tabId: string) => {
      tabBtns.forEach((b) => b.classList.toggle('active', b.dataset.tab === tabId));
      tabContents.forEach((c) => c.classList.toggle('active', c.id === tabId));
    };

    if (isAndroid) {
      selectTab('tab-android-pwa');
    } else if (isIOS) {
      selectTab('tab-ios');
    }

    tabBtns.forEach((btn) => {
      btn.addEventListener('click', (e) => {
        e.stopPropagation();
        this.triggerHaptic(ImpactStyle.Light);
        const target = btn.dataset.tab;
        if (target) selectTab(target);
      });
    });

    installBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.triggerHaptic(ImpactStyle.Medium);
      installModal.classList.remove('hidden');
    });

    closeModalBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      installModal.classList.add('hidden');
      if (window.location.hash === '#install' || window.location.hash === '#open-install') {
        history.replaceState(null, '', window.location.pathname);
      }
    });

    installModal.addEventListener('click', (e) => {
      if (e.target === installModal) {
        installModal.classList.add('hidden');
        if (window.location.hash === '#install' || window.location.hash === '#open-install') {
          history.replaceState(null, '', window.location.pathname);
        }
      }
    });

    // Check on load or hashchange
    const checkHash = () => {
      if (window.location.hash === '#install' || window.location.hash === '#open-install') {
        installModal.classList.remove('hidden');
      }
    };
    checkHash();
    window.addEventListener('hashchange', checkHash);

    if (triggerPwaBtn) {
      triggerPwaBtn.addEventListener('click', async (e) => {
        e.stopPropagation();
        this.triggerHaptic(ImpactStyle.Medium);
        if (deferredPrompt) {
          deferredPrompt.prompt();
          const { outcome } = await deferredPrompt.userChoice;
          if (outcome === 'accepted') {
            installBtn.textContent = '✅ Installed';
            installModal.classList.add('hidden');
          }
          deferredPrompt = null;
        } else {
          alert("Chrome menu (⋮) -> 'Install app' or download the direct APK below!");
        }
      });
    }
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

  public setScore(p1: number, p2: number): void {
    if (this.scoreDisplay) {
      this.scoreDisplay.textContent = `P1: ${p1} | P2: ${p2}`;
    }
  }

  public showPushOutModal(shooterName: string, opponentName: string): void {
    if (this.pushoutDesc) {
      this.pushoutDesc.textContent = `${shooterName} played a Push-Out. ${opponentName}, choose whether to shoot from this position or pass the shot back:`;
    }
    this.pushoutModal?.classList.remove('hidden');
  }

  public hidePushOutModal(): void {
    this.pushoutModal?.classList.add('hidden');
  }

  public showVictoryModal(winnerTitle: string, subText: string, p1Score: number, p2Score: number): void {
    if (this.victoryTitle) this.victoryTitle.textContent = winnerTitle;
    if (this.victorySub) this.victorySub.textContent = subText;
    if (this.modalScoreP1) this.modalScoreP1.textContent = `P1: ${p1Score}`;
    if (this.modalScoreP2) this.modalScoreP2.textContent = `P2: ${p2Score}`;
    this.victoryModal?.classList.remove('hidden');
  }

  public hideVictoryModal(): void {
    this.victoryModal?.classList.add('hidden');
  }

  public setPlayerProfiles(
    p1Score: number,
    p2Score: number,
    p1Group: string,
    p2Group: string,
    isP1Active: boolean,
    matchStatus: string
  ): void {
    if (this.p1ScorePill) this.p1ScorePill.textContent = `${p1Score}`;
    if (this.p2ScorePill) this.p2ScorePill.textContent = `${p2Score}`;
    if (this.p1GroupPill) this.p1GroupPill.textContent = p1Group;
    if (this.p2GroupPill) this.p2GroupPill.textContent = p2Group;
    if (this.matchStatusBanner) this.matchStatusBanner.textContent = matchStatus;

    if (this.p1Card && this.p2Card) {
      this.p1Card.classList.toggle('p1-active', isP1Active);
      this.p2Card.classList.toggle('p2-active', !isP1Active);
    }
  }

  public setTurnTimer(isP1Active: boolean, percentNorm: number): void {
    const circumference = 113.1;
    const offset = circumference * (1.0 - Math.max(0, Math.min(1, percentNorm)));
    const color = percentNorm < 0.25 ? '#ef4444' : percentNorm < 0.5 ? '#f59e0b' : '#38bdf8';

    if (isP1Active && this.p1TimerCircle) {
      this.p1TimerCircle.style.strokeDashoffset = `${offset}`;
      this.p1TimerCircle.style.stroke = color;
      if (this.p2TimerCircle) this.p2TimerCircle.style.strokeDashoffset = `${circumference}`;
    } else if (!isP1Active && this.p2TimerCircle) {
      this.p2TimerCircle.style.strokeDashoffset = `${offset}`;
      this.p2TimerCircle.style.stroke = color;
      if (this.p1TimerCircle) this.p1TimerCircle.style.strokeDashoffset = `${circumference}`;
    }
  }

  public showAccolade(text: string, durationMs = 2000): void {
    if (!this.accoladeToast) return;
    if (this.accoladeTimeout) window.clearTimeout(this.accoladeTimeout);
    this.accoladeToast.textContent = text;
    this.accoladeToast.classList.remove('hidden');
    this.accoladeToast.classList.add('pop-in');

    this.accoladeTimeout = window.setTimeout(() => {
      this.accoladeToast?.classList.add('hidden');
      this.accoladeToast?.classList.remove('pop-in');
    }, durationMs);
  }

  public showAutoRestartBanner(title: string, sub: string, countdownSec: number): void {
    if (this.autorestartTitle) this.autorestartTitle.textContent = title;
    if (this.autorestartSub) {
      this.autorestartSub.innerHTML = `${sub} • Starting new rack in <strong id="autorestart-timer">${countdownSec}</strong>s...`;
    }
    this.autorestartCard?.classList.remove('hidden');
  }

  public updateAutoRestartCountdown(sec: number): void {
    const el = this.container.querySelector('#autorestart-timer');
    if (el) el.textContent = `${sec}`;
  }

  public hideAutoRestartBanner(): void {
    this.autorestartCard?.classList.add('hidden');
  }

  private async triggerHaptic(style: ImpactStyle): Promise<void> {
    try {
      await Haptics.impact({ style });
    } catch {
      if (navigator.vibrate) navigator.vibrate(10);
    }
  }
}
