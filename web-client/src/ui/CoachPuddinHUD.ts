/**
 * Coach Puddin Interactive HUD & Reaction Emote System
 * Mentor, legend advisor, dynamic emote generator, and shot analyst.
 */

import { WebAudioEngine } from '../audio/WebAudioEngine';

export enum CoachState {
  IDLE = 'IDLE',
  AIM_ADVICE = 'AIM_ADVICE',
  THINKING = 'THINKING',
  CHEER = 'CHEER',
  CONSOLING = 'CONSOLING'
}

export interface EmoteSticker {
  id: string;
  icon: string;
  label: string;
  puddinQuote: string;
}

export const PUDDIN_STICKERS: EmoteSticker[] = [
  { id: 'wise_nod', icon: '👴', label: 'Wise Nod', puddinQuote: 'Smooth stroke, son. Smooth stroke.' },
  { id: 'ghost_tip', icon: '🎱', label: 'Ghost Ball', puddinQuote: 'Look for the ghost ball center, not the pocket edge.' },
  { id: 'cocoa_sip', icon: '☕', label: 'Hot Cocoa', puddinQuote: 'Take your time. Let the felt settle.' },
  { id: 'glasses_tilt', icon: '👓', label: 'Glasses Adjust', puddinQuote: 'Let me double-check this cut angle...' },
  { id: 'cue_tap', icon: '👏', label: 'Cue Tap', puddinQuote: 'Pure masterclass! That cue ball has eyes.' },
  { id: 'hat_tip', icon: '🎩', label: 'Hat Tip', puddinQuote: 'Legendary break. Even Mosconi is smiling.' }
];

export class CoachPuddinHUD {
  private container: HTMLElement;
  private avatarCanvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  private speechBubble: HTMLElement;
  private speechTextEl: HTMLElement;
  private legendButton: HTMLButtonElement;
  private emotesButton: HTMLButtonElement;
  private emotesPanel: HTMLElement;
  private audioEngine: WebAudioEngine;

  private currentState: CoachState = CoachState.IDLE;
  private animationFrameId: number = 0;
  private animTimer: number = 0;
  private typewriterInterval: number | null = null;
  private fullMessage: string = '';
  private currentChars: number = 0;

  public onLegendAdviceRequested?: (legendName: string) => void;
  public onEmoteTriggered?: (sticker: EmoteSticker) => void;

  constructor(containerId: string, audioEngine: WebAudioEngine) {
    this.audioEngine = audioEngine;
    const root = document.getElementById(containerId);
    if (!root) {
      throw new Error(`Container #${containerId} not found.`);
    }
    this.container = root;

    this.container.innerHTML = `
      <div class="coach-puddin-wrapper">
        <div class="coach-avatar-container">
          <canvas class="coach-avatar-canvas" width="120" height="120"></canvas>
          <div class="coach-badge">COACH PUDDIN</div>
        </div>
        <div class="coach-speech-bubble" id="puddin-speech-bubble">
          <div class="coach-speech-header">
            <span class="coach-tag">HALL OF FAME MENTOR</span>
            <div class="coach-quick-actions">
              <button class="coach-action-btn" id="btn-ask-puddin" title="Ask Puddin for Legend Trajectory">💡 Ask Puddin</button>
              <button class="coach-action-btn" id="btn-puddin-emotes" title="PvP Emotes">💬 Emotes</button>
            </div>
          </div>
          <p class="coach-speech-text" id="puddin-speech-text">Welcome to the table, kid. Remember: cue ball speed controls everything.</p>
        </div>
        <div class="coach-emotes-drawer hidden" id="puddin-emotes-drawer"></div>
      </div>
    `;

    this.avatarCanvas = this.container.querySelector('.coach-avatar-canvas') as HTMLCanvasElement;
    this.ctx = this.avatarCanvas.getContext('2d')!;
    this.speechBubble = this.container.querySelector('#puddin-speech-bubble') as HTMLElement;
    this.speechTextEl = this.container.querySelector('#puddin-speech-text') as HTMLElement;
    this.legendButton = this.container.querySelector('#btn-ask-puddin') as HTMLButtonElement;
    this.emotesButton = this.container.querySelector('#btn-puddin-emotes') as HTMLButtonElement;
    this.emotesPanel = this.container.querySelector('#puddin-emotes-drawer') as HTMLElement;

    this.setupListeners();
    this.renderEmotesDrawer();
    this.startAvatarAnimation();
  }

  private setupListeners(): void {
    this.legendButton.addEventListener('click', (e) => {
      e.stopPropagation();
      this.audioEngine.playVoiceBlip(520);
      this.setState(CoachState.THINKING);
      this.say("Consulting the legends... Efren's 3-rail kick vs Mosconi's key ball safety.");
      if (this.onLegendAdviceRequested) {
        this.onLegendAdviceRequested('Efren Reyes');
      }
    });

    this.emotesButton.addEventListener('click', (e) => {
      e.stopPropagation();
      this.emotesPanel.classList.toggle('hidden');
    });

    // Close drawer when clicking outside
    document.addEventListener('click', () => {
      if (!this.emotesPanel.classList.contains('hidden')) {
        this.emotesPanel.classList.add('hidden');
      }
    });
  }

  private renderEmotesDrawer(): void {
    this.emotesPanel.innerHTML = PUDDIN_STICKERS.map(s => `
      <button class="puddin-emote-chip" data-id="${s.id}" title="${s.puddinQuote}">
        <span class="emote-icon">${s.icon}</span>
        <span class="emote-label">${s.label}</span>
      </button>
    `).join('');

    this.emotesPanel.querySelectorAll('.puddin-emote-chip').forEach(el => {
      el.addEventListener('click', (e) => {
        e.stopPropagation();
        const id = (el as HTMLElement).dataset.id;
        const sticker = PUDDIN_STICKERS.find(s => s.id === id);
        if (sticker) {
          this.triggerEmote(sticker);
          this.emotesPanel.classList.add('hidden');
        }
      });
    });
  }

  public triggerEmote(sticker: EmoteSticker): void {
    this.say(sticker.puddinQuote);
    this.setState(CoachState.CHEER);
    if (this.onEmoteTriggered) {
      this.onEmoteTriggered(sticker);
    }
  }

  public setState(state: CoachState): void {
    this.currentState = state;
  }

  public say(message: string, pitch: number = 440): void {
    if (this.typewriterInterval) {
      clearInterval(this.typewriterInterval);
      this.typewriterInterval = null;
    }

    this.fullMessage = message;
    this.currentChars = 0;
    this.speechTextEl.textContent = '';

    let blipCounter = 0;
    this.typewriterInterval = window.setInterval(() => {
      if (this.currentChars < this.fullMessage.length) {
        this.currentChars++;
        this.speechTextEl.textContent = this.fullMessage.slice(0, this.currentChars);

        // Sound blip on word characters
        blipCounter++;
        if (blipCounter % 2 === 0 && this.fullMessage[this.currentChars - 1] !== ' ') {
          this.audioEngine.playVoiceBlip(pitch);
        }
      } else {
        if (this.typewriterInterval) {
          clearInterval(this.typewriterInterval);
          this.typewriterInterval = null;
        }
      }
    }, 22);
  }

  private startAvatarAnimation(): void {
    const loop = (timestamp: number) => {
      this.animTimer = timestamp * 0.003;
      this.drawAvatar();
      this.animationFrameId = requestAnimationFrame(loop);
    };
    this.animationFrameId = requestAnimationFrame(loop);
  }

  private drawAvatar(): void {
    const ctx = this.ctx;
    const w = this.avatarCanvas.width;
    const h = this.avatarCanvas.height;
    ctx.clearRect(0, 0, w, h);

    const cx = w / 2;
    const cy = h / 2 + 6;

    // Gentle breathing loop
    const breath = Math.sin(this.animTimer) * 1.5;

    // Background circle halo
    ctx.beginPath();
    ctx.arc(cx, cy, 54, 0, Math.PI * 2);
    ctx.fillStyle = '#1e293b';
    ctx.fill();
    ctx.lineWidth = 3;
    ctx.strokeStyle = '#d97706';
    ctx.stroke();

    // Fair Isle knit sweater body (navy / amber patterns)
    ctx.beginPath();
    ctx.moveTo(cx - 36, cy + 50);
    ctx.bezierCurveTo(cx - 32, cy + 18 + breath, cx + 32, cy + 18 + breath, cx + 36, cy + 50);
    ctx.closePath();
    ctx.fillStyle = '#1e3a8a'; // Navy sweater
    ctx.fill();

    // Fair Isle knit zigzag pattern
    ctx.beginPath();
    ctx.strokeStyle = '#f59e0b';
    ctx.lineWidth = 2;
    const yPattern = cy + 32 + breath;
    for (let x = cx - 24; x < cx + 24; x += 8) {
      ctx.moveTo(x, yPattern);
      ctx.lineTo(x + 4, yPattern - 4);
      ctx.lineTo(x + 8, yPattern);
    }
    ctx.stroke();

    // White shirt collar
    ctx.beginPath();
    ctx.moveTo(cx - 10, cy + 14 + breath);
    ctx.lineTo(cx, cy + 24 + breath);
    ctx.lineTo(cx + 10, cy + 14 + breath);
    ctx.fillStyle = '#f8fafc';
    ctx.fill();

    // Head / Face
    ctx.beginPath();
    ctx.ellipse(cx, cy - 6 + breath * 0.5, 22, 25, 0, 0, Math.PI * 2);
    ctx.fillStyle = '#fcd34d'; // Warm skin tone
    ctx.fill();

    // Silver hair
    ctx.beginPath();
    ctx.arc(cx, cy - 14 + breath * 0.5, 23, Math.PI * 0.9, Math.PI * 2.1);
    ctx.lineWidth = 6;
    ctx.strokeStyle = '#e2e8f0'; // Silver
    ctx.stroke();

    // Wire-rimmed glasses
    ctx.lineWidth = 1.5;
    ctx.strokeStyle = '#d97706'; // Gold/wire frame
    // Left glass
    ctx.beginPath();
    ctx.arc(cx - 8, cy - 6 + breath * 0.5, 6, 0, Math.PI * 2);
    ctx.stroke();
    // Right glass
    ctx.beginPath();
    ctx.arc(cx + 8, cy - 6 + breath * 0.5, 6, 0, Math.PI * 2);
    ctx.stroke();
    // Bridge
    ctx.beginPath();
    ctx.moveTo(cx - 2, cy - 6 + breath * 0.5);
    ctx.lineTo(cx + 2, cy - 6 + breath * 0.5);
    ctx.stroke();

    // Eyes (with gentle blink)
    const isBlinking = (Math.sin(this.animTimer * 0.6) > 0.96);
    if (isBlinking) {
      ctx.beginPath();
      ctx.moveTo(cx - 11, cy - 6 + breath * 0.5);
      ctx.lineTo(cx - 5, cy - 6 + breath * 0.5);
      ctx.moveTo(cx + 5, cy - 6 + breath * 0.5);
      ctx.lineTo(cx + 11, cy - 6 + breath * 0.5);
      ctx.strokeStyle = '#334155';
      ctx.stroke();
    } else {
      ctx.beginPath();
      ctx.arc(cx - 8, cy - 6 + breath * 0.5, 2, 0, Math.PI * 2);
      ctx.arc(cx + 8, cy - 6 + breath * 0.5, 2, 0, Math.PI * 2);
      ctx.fillStyle = '#1e293b';
      ctx.fill();
    }

    // Smiling mouth
    ctx.beginPath();
    if (this.currentState === CoachState.CHEER) {
      ctx.arc(cx, cy + 5 + breath * 0.5, 7, 0, Math.PI);
      ctx.fillStyle = '#dc2626';
      ctx.fill();
    } else if (this.currentState === CoachState.CONSOLING) {
      ctx.arc(cx, cy + 10 + breath * 0.5, 6, Math.PI * 1.1, Math.PI * 1.9);
      ctx.strokeStyle = '#475569';
      ctx.stroke();
    } else {
      // Gentle smile
      ctx.arc(cx, cy + 4 + breath * 0.5, 5, 0.1, Math.PI - 0.1);
      ctx.strokeStyle = '#475569';
      ctx.stroke();
    }

    // Upright pool cue stick held in hand
    ctx.beginPath();
    ctx.moveTo(cx + 34, cy + 48);
    ctx.lineTo(cx + 30, cy - 40);
    ctx.lineWidth = 3.5;
    ctx.strokeStyle = '#b45309'; // Maple cue wood
    ctx.stroke();

    // White cue tip
    ctx.beginPath();
    ctx.moveTo(cx + 30, cy - 40);
    ctx.lineTo(cx + 29.5, cy - 46);
    ctx.lineWidth = 3;
    ctx.strokeStyle = '#38bdf8'; // Blue chalk tip
    ctx.stroke();
  }

  public destroy(): void {
    if (this.animationFrameId) {
      cancelAnimationFrame(this.animationFrameId);
    }
    if (this.typewriterInterval) {
      clearInterval(this.typewriterInterval);
    }
  }
}
