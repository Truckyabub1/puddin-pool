/**
 * CoachPuddinAvatar: Procedural Animated Mentor Bust & Elder Wisdom System
 * Elderly gentleman (silver hair, wire-rim glasses, navy/amber Fair Isle sweater, cue stick & cue ball in hand).
 * Animated emote states: IDLE, ADVICE, CHEER, CONSOLE.
 */

import { WebAudioEngine } from '../audio/WebAudioEngine';

export enum PuddinEmoteState {
  IDLE = 'IDLE',
  ADVICE = 'ADVICE',
  CHEER = 'CHEER',
  CONSOLE = 'CONSOLE',
}

export class CoachPuddinAvatar {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  private speechContainer: HTMLElement;
  private speechTextEl: HTMLElement;
  private audioElement: WebAudioEngine;

  private currentState: PuddinEmoteState = PuddinEmoteState.IDLE;
  private animTimer: number = 0;
  private animFrameId: number = 0;
  private typewriterTimer: number | null = null;
  private isCollapsed: boolean = false;

  constructor(avatarCanvasId: string, speechRootId: string, audio: WebAudioEngine) {
    const cvs = document.getElementById(avatarCanvasId) as HTMLCanvasElement;
    if (!cvs) throw new Error(`Canvas #${avatarCanvasId} not found`);
    this.canvas = cvs;
    this.ctx = cvs.getContext('2d')!;
    this.audioElement = audio;

    const speechRoot = document.getElementById(speechRootId);
    if (!speechRoot) throw new Error(`Speech container #${speechRootId} not found`);
    this.speechContainer = speechRoot;

    this.speechContainer.innerHTML = `
      <div class="elder-wisdom-card" id="elder-wisdom-bubble">
        <div class="elder-wisdom-header">
          <div class="elder-wisdom-title">
            <span class="mentor-star">★</span>
            <span class="mentor-heading">COACH PUDDIN'S WISDOM</span>
          </div>
          <button id="btn-toggle-wisdom" class="wisdom-collapse-btn" title="Toggle Wisdom">−</button>
        </div>
        <p class="elder-wisdom-body" id="elder-wisdom-text">Welcome to the felt, son. Stroke smooth, stay down, and let the geometry work for you.</p>
        <div class="legend-actions-row">
          <button id="btn-ask-puddin" class="legend-btn" title="Ask Coach Puddin for legendary shot advice">💡 Ask Puddin</button>
          <button id="btn-lock-line" class="legend-btn lock-btn hidden" title="Lock aim angle & spin to Coach Puddin's line">🎯 Lock Line</button>
        </div>
      </div>
    `;

    this.speechTextEl = this.speechContainer.querySelector('#elder-wisdom-text') as HTMLElement;
    const toggleBtn = this.speechContainer.querySelector('#btn-toggle-wisdom') as HTMLButtonElement;
    const askBtn = this.speechContainer.querySelector('#btn-ask-puddin') as HTMLButtonElement;
    const lockBtn = this.speechContainer.querySelector('#btn-lock-line') as HTMLButtonElement;

    toggleBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.isCollapsed = !this.isCollapsed;
      const card = this.speechContainer.querySelector('#elder-wisdom-bubble');
      if (card) card.classList.toggle('collapsed', this.isCollapsed);
      toggleBtn.textContent = this.isCollapsed ? '+' : '−';
    });

    askBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      if (this.onAskAdvice) this.onAskAdvice();
    });

    lockBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      if (this.onLockLine) this.onLockLine();
    });

    this.startAnimationLoop();
  }

  public onAskAdvice?: () => void;
  public onLockLine?: () => void;

  public setLockLineVisible(visible: boolean): void {
    const lockBtn = this.speechContainer.querySelector('#btn-lock-line');
    if (lockBtn) {
      if (visible) {
        lockBtn.classList.remove('hidden');
      } else {
        lockBtn.classList.add('hidden');
      }
    }
  }

  public setEmote(state: PuddinEmoteState, message?: string): void {
    this.currentState = state;
    if (message) {
      this.speak(message);
    }
  }

  public speak(message: string, pitch: number = 440): void {
    if (this.typewriterTimer) {
      clearInterval(this.typewriterTimer);
      this.typewriterTimer = null;
    }

    this.speechTextEl.textContent = '';
    let idx = 0;
    let blipCount = 0;

    this.typewriterTimer = window.setInterval(() => {
      if (idx < message.length) {
        idx++;
        this.speechTextEl.textContent = message.slice(0, idx);
        blipCount++;
        if (blipCount % 2 === 0 && message[idx - 1] !== ' ') {
          this.audioElement.playVoiceBlip(pitch);
        }
      } else {
        if (this.typewriterTimer) {
          clearInterval(this.typewriterTimer);
          this.typewriterTimer = null;
        }
      }
    }, 22);
  }

  private startAnimationLoop(): void {
    const render = (time: number) => {
      this.animTimer = time * 0.003;
      this.draw();
      this.animFrameId = requestAnimationFrame(render);
    };
    this.animFrameId = requestAnimationFrame(render);
  }

  private draw(): void {
    const ctx = this.ctx;
    const w = this.canvas.width;
    const h = this.canvas.height;
    ctx.clearRect(0, 0, w, h);

    const cx = w * 0.5;
    const cy = h * 0.5 + 4;

    // Breathing loop
    const breath = Math.sin(this.animTimer) * 1.5;

    // Halo background badge
    ctx.beginPath();
    ctx.arc(cx, cy, 46, 0, Math.PI * 2);
    ctx.fillStyle = '#0f172a';
    ctx.fill();
    ctx.lineWidth = 3;
    ctx.strokeStyle = '#d97706';
    ctx.stroke();

    // Navy Fair Isle Sweater Shoulders
    ctx.beginPath();
    ctx.moveTo(cx - 36, cy + 44);
    ctx.bezierCurveTo(cx - 30, cy + 14 + breath, cx + 30, cy + 14 + breath, cx + 36, cy + 44);
    ctx.closePath();
    ctx.fillStyle = '#1e3a8a';
    ctx.fill();

    // Fair Isle knit pattern
    ctx.strokeStyle = '#f59e0b';
    ctx.lineWidth = 2;
    ctx.beginPath();
    const py = cy + 26 + breath;
    for (let x = cx - 22; x < cx + 22; x += 7) {
      ctx.moveTo(x, py);
      ctx.lineTo(x + 3.5, py - 3.5);
      ctx.lineTo(x + 7, py);
    }
    ctx.stroke();

    // White shirt collar
    ctx.fillStyle = '#f8fafc';
    ctx.beginPath();
    ctx.moveTo(cx - 8, cy + 12 + breath);
    ctx.lineTo(cx, cy + 20 + breath);
    ctx.lineTo(cx + 8, cy + 12 + breath);
    ctx.fill();

    // Head / Face
    ctx.fillStyle = '#fcd34d';
    ctx.beginPath();
    ctx.ellipse(cx, cy - 6 + breath * 0.5, 18, 22, 0, 0, Math.PI * 2);
    ctx.fill();

    // Silver hair
    ctx.strokeStyle = '#e2e8f0';
    ctx.lineWidth = 5;
    ctx.beginPath();
    ctx.arc(cx, cy - 12 + breath * 0.5, 19, Math.PI * 0.9, Math.PI * 2.1);
    ctx.stroke();

    // Wire-rimmed glasses
    ctx.strokeStyle = '#d97706';
    ctx.lineWidth = 1.4;
    // Left rim
    ctx.beginPath();
    ctx.arc(cx - 6.5, cy - 6 + breath * 0.5, 5, 0, Math.PI * 2);
    ctx.stroke();
    // Right rim
    ctx.beginPath();
    ctx.arc(cx + 6.5, cy - 6 + breath * 0.5, 5, 0, Math.PI * 2);
    ctx.stroke();
    // Bridge
    ctx.beginPath();
    ctx.moveTo(cx - 1.5, cy - 6 + breath * 0.5);
    ctx.lineTo(cx + 1.5, cy - 6 + breath * 0.5);
    ctx.stroke();

    // Eyes (with gentle blink)
    const blinking = (Math.sin(this.animTimer * 0.6) > 0.96);
    if (blinking) {
      ctx.strokeStyle = '#334155';
      ctx.beginPath();
      ctx.moveTo(cx - 9, cy - 6 + breath * 0.5);
      ctx.lineTo(cx - 4, cy - 6 + breath * 0.5);
      ctx.moveTo(cx + 4, cy - 6 + breath * 0.5);
      ctx.lineTo(cx + 9, cy - 6 + breath * 0.5);
      ctx.stroke();
    } else {
      ctx.fillStyle = '#1e293b';
      ctx.beginPath();
      ctx.arc(cx - 6.5, cy - 6 + breath * 0.5, 1.8, 0, Math.PI * 2);
      ctx.arc(cx + 6.5, cy - 6 + breath * 0.5, 1.8, 0, Math.PI * 2);
      ctx.fill();
    }

    // Mouth expressions based on Emote state
    ctx.beginPath();
    if (this.currentState === PuddinEmoteState.CHEER) {
      ctx.arc(cx, cy + 4 + breath * 0.5, 6, 0, Math.PI);
      ctx.fillStyle = '#dc2626';
      ctx.fill();
    } else if (this.currentState === PuddinEmoteState.CONSOLE) {
      ctx.arc(cx, cy + 8 + breath * 0.5, 5, Math.PI * 1.1, Math.PI * 1.9);
      ctx.strokeStyle = '#475569';
      ctx.stroke();
    } else {
      ctx.arc(cx, cy + 3 + breath * 0.5, 4, 0.1, Math.PI - 0.1);
      ctx.strokeStyle = '#475569';
      ctx.stroke();
    }

    // Right Hand: Pool Cue Stick
    ctx.strokeStyle = '#b45309';
    ctx.lineWidth = 3;
    ctx.beginPath();
    if (this.currentState === PuddinEmoteState.ADVICE) {
      // Reaching forward pointing stick toward table
      ctx.moveTo(cx + 26, cy + 38);
      ctx.lineTo(cx + 44, cy - 10);
    } else {
      // Upright cue stick
      ctx.moveTo(cx + 28, cy + 42);
      ctx.lineTo(cx + 26, cy - 35);
    }
    ctx.stroke();

    // Cue tip blue chalk
    ctx.strokeStyle = '#38bdf8';
    ctx.lineWidth = 2.5;
    ctx.beginPath();
    if (this.currentState === PuddinEmoteState.ADVICE) {
      ctx.moveTo(cx + 44, cy - 10);
      ctx.lineTo(cx + 48, cy - 16);
    } else {
      ctx.moveTo(cx + 26, cy - 35);
      ctx.lineTo(cx + 25.5, cy - 40);
    }
    ctx.stroke();

    // Left Hand: Holding Cue Ball in palm
    const ballX = (this.currentState === PuddinEmoteState.CHEER) ? cx - 28 : cx - 24;
    const ballY = (this.currentState === PuddinEmoteState.CHEER) ? cy - 8 + breath : cy + 24 + breath;

    ctx.fillStyle = '#ffffff';
    ctx.beginPath();
    ctx.arc(ballX, ballY, 6, 0, Math.PI * 2);
    ctx.fill();
    ctx.strokeStyle = '#94a3b8';
    ctx.lineWidth = 1;
    ctx.stroke();
  }

  public destroy(): void {
    if (this.animFrameId) cancelAnimationFrame(this.animFrameId);
    if (this.typewriterTimer) clearInterval(this.typewriterTimer);
  }
}
