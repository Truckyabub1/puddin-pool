/**
 * Procedural Web Audio Engine for Puddin's Pool
 * Real-time synthesis for zero-latency, zero-asset-download (<5MB budget) billiards acoustics.
 */

export class WebAudioEngine {
  private ctx: AudioContext | null = null;
  private masterGain: GainNode | null = null;
  private rollingGain: GainNode | null = null;
  private rollingSource: AudioBufferSourceNode | null = null;
  private isMuted: boolean = false;

  constructor() {
    // AudioContext will be lazily initialized on first user interaction
  }

  public init(): void {
    if (this.ctx) return;
    const AudioCtx = window.AudioContext || (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
    this.ctx = new AudioCtx();
    this.masterGain = this.ctx.createGain();
    this.masterGain.gain.setValueAtTime(0.85, this.ctx.currentTime);
    this.masterGain.connect(this.ctx.destination);

    this.initRollingLoop();
  }

  public resume(): void {
    if (this.ctx && this.ctx.state === 'suspended') {
      this.ctx.resume().catch(() => {});
    }
  }

  /**
   * Play cue stick tip strike against cue ball.
   * @param power Normalized impulse 0.0 to 1.0
   */
  public playCueStrike(power: number): void {
    if (!this.ctx || !this.masterGain || this.isMuted) return;
    this.resume();

    const t = this.ctx.currentTime;
    const gain = this.ctx.createGain();
    gain.gain.setValueAtTime(Math.min(1.0, power * 1.2), t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.045);
    gain.connect(this.masterGain);

    // Deep leather thud
    const osc = this.ctx.createOscillator();
    osc.type = 'triangle';
    osc.frequency.setValueAtTime(320, t);
    osc.frequency.exponentialRampToValueAtTime(80, t + 0.04);
    osc.connect(gain);
    osc.start(t);
    osc.stop(t + 0.045);

    // High frequency chalk contact noise
    const noise = this.createNoiseBurst(0.015);
    if (noise) {
      const filter = this.ctx.createBiquadFilter();
      filter.type = 'bandpass';
      filter.frequency.setValueAtTime(3500, t);
      filter.Q.setValueAtTime(2.0, t);
      noise.connect(filter);
      filter.connect(gain);
      noise.start(t);
      noise.stop(t + 0.02);
    }
  }

  /**
   * Ball-ball collision clack.
   * High Q resonant frequencies matching phenolic resin billiard balls (aramith).
   * @param impulse Collision impulse (relative speed in m/s)
   */
  public playBallCollision(impulse: number): void {
    if (!this.ctx || !this.masterGain || this.isMuted) return;
    this.resume();

    const t = this.ctx.currentTime;
    const norm = Math.min(1.0, impulse / 4.0);
    if (norm < 0.03) return;

    const gain = this.ctx.createGain();
    gain.gain.setValueAtTime(norm * 0.9, t);
    gain.gain.exponentialRampToValueAtTime(0.0005, t + 0.035);
    gain.connect(this.masterGain);

    // Phenolic primary resonance (approx 2400Hz - 2800Hz)
    const osc1 = this.ctx.createOscillator();
    osc1.type = 'sine';
    osc1.frequency.setValueAtTime(2450 + (Math.random() * 80 - 40), t);
    osc1.connect(gain);
    osc1.start(t);
    osc1.stop(t + 0.035);

    // Phenolic upper partial (approx 4800Hz)
    const osc2 = this.ctx.createOscillator();
    osc2.type = 'sine';
    osc2.frequency.setValueAtTime(4900 + (Math.random() * 120 - 60), t);
    const gain2 = this.ctx.createGain();
    gain2.gain.setValueAtTime(norm * 0.45, t);
    gain2.gain.exponentialRampToValueAtTime(0.0005, t + 0.02);
    osc2.connect(gain2);
    gain2.connect(gain);
    osc2.start(t);
    osc2.stop(t + 0.025);
  }

  /**
   * Cushion rail collision thud with rubber absorption & cloth friction.
   */
  public playCushionCollision(impulse: number, incidentAngle: number = 0.78): void {
    if (!this.ctx || !this.masterGain || this.isMuted) return;
    this.resume();

    const t = this.ctx.currentTime;
    const norm = Math.min(1.0, impulse / 4.0);
    if (norm < 0.03) return;

    const gain = this.ctx.createGain();
    gain.gain.setValueAtTime(norm * 0.75, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.07);
    gain.connect(this.masterGain);

    // Rubber wood rail sub-tone
    const osc = this.ctx.createOscillator();
    osc.type = 'sine';
    const baseFreq = 140 + Math.sin(incidentAngle) * 40;
    osc.frequency.setValueAtTime(baseFreq, t);
    osc.frequency.exponentialRampToValueAtTime(65, t + 0.065);
    osc.connect(gain);
    osc.start(t);
    osc.stop(t + 0.07);

    // Muffled cloth noise
    const noise = this.createNoiseBurst(0.04);
    if (noise) {
      const filter = this.ctx.createBiquadFilter();
      filter.type = 'lowpass';
      filter.frequency.setValueAtTime(600, t);
      noise.connect(filter);
      filter.connect(gain);
      noise.start(t);
      noise.stop(t + 0.045);
    }
  }

  /**
   * Pocket drop sound (leather net drop & ball drop to ball return).
   */
  public playPocketDrop(): void {
    if (!this.ctx || !this.masterGain || this.isMuted) return;
    this.resume();

    const t = this.ctx.currentTime;
    const gain = this.ctx.createGain();
    gain.gain.setValueAtTime(0.8, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.22);
    gain.connect(this.masterGain);

    const osc = this.ctx.createOscillator();
    osc.type = 'triangle';
    osc.frequency.setValueAtTime(180, t);
    osc.frequency.exponentialRampToValueAtTime(45, t + 0.2);
    osc.connect(gain);
    osc.start(t);
    osc.stop(t + 0.22);
  }

  /**
   * Coach Puddin speech typewriter chirp / voice blip.
   */
  public playVoiceBlip(characterPitch: number = 440): void {
    if (!this.ctx || !this.masterGain || this.isMuted) return;
    this.resume();

    const t = this.ctx.currentTime;
    const gain = this.ctx.createGain();
    gain.gain.setValueAtTime(0.12, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.03);
    gain.connect(this.masterGain);

    const osc = this.ctx.createOscillator();
    osc.type = 'sine';
    const pitch = characterPitch + (Math.random() * 40 - 20);
    osc.frequency.setValueAtTime(pitch, t);
    osc.connect(gain);
    osc.start(t);
    osc.stop(t + 0.03);
  }

  /**
   * Update rolling cloth friction hum based on total kinetic velocity.
   */
  public updateRollingSound(totalVelocity: number): void {
    if (!this.rollingGain || !this.ctx || this.isMuted) return;
    const targetGain = Math.min(0.18, totalVelocity * 0.03);
    this.rollingGain.gain.setTargetAtTime(targetGain, this.ctx.currentTime, 0.05);
  }

  private initRollingLoop(): void {
    if (!this.ctx || !this.masterGain) return;
    // Generate 1 second white noise buffer for looped filtered rolling
    const bufferSize = this.ctx.sampleRate * 1.0;
    const buffer = this.ctx.createBuffer(1, bufferSize, this.ctx.sampleRate);
    const data = buffer.getChannelData(0);
    for (let i = 0; i < bufferSize; i++) {
      data[i] = Math.random() * 2 - 1;
    }

    this.rollingSource = this.ctx.createBufferSource();
    this.rollingSource.buffer = buffer;
    this.rollingSource.loop = true;

    const filter = this.ctx.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.setValueAtTime(280, this.ctx.currentTime);
    filter.Q.setValueAtTime(1.5, this.ctx.currentTime);

    this.rollingGain = this.ctx.createGain();
    this.rollingGain.gain.setValueAtTime(0.0, this.ctx.currentTime);

    this.rollingSource.connect(filter);
    filter.connect(this.rollingGain);
    this.rollingGain.connect(this.masterGain);
    this.rollingSource.start(0);
  }

  private createNoiseBurst(duration: number): AudioBufferSourceNode | null {
    if (!this.ctx) return null;
    const length = Math.floor(this.ctx.sampleRate * duration);
    const buffer = this.ctx.createBuffer(1, length, this.ctx.sampleRate);
    const data = buffer.getChannelData(0);
    for (let i = 0; i < length; i++) {
      data[i] = Math.random() * 2 - 1;
    }
    const source = this.ctx.createBufferSource();
    source.buffer = buffer;
    return source;
  }

  public toggleMute(): boolean {
    this.isMuted = !this.isMuted;
    if (this.masterGain && this.ctx) {
      this.masterGain.gain.setValueAtTime(this.isMuted ? 0 : 0.85, this.ctx.currentTime);
    }
    return this.isMuted;
  }
}
