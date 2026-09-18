/**
 * Puddin's Pool - Universal 120Hz Tournament Billiards Client
 * Integrates 3D Rigid-Body Dynamics, Valley Table Geometry, K-55 Cushions,
 * Coach Puddin Interactive Mentor & Avatar, TouchOverlay Dual-Thumb Controller,
 * and Unified WPA 8-Ball & 9-Ball Official Rules Arbitrator.
 */

import { ScreenOrientation } from '@capacitor/screen-orientation';
import { StatusBar } from '@capacitor/status-bar';
import { WebAudioEngine } from './audio/WebAudioEngine';
import { TableRenderer } from './render/TableRenderer';
import { BallRenderer } from './render/BallRenderer';
import { CoachPuddinAvatar, PuddinEmoteState } from './ui/CoachPuddinAvatar';
import { ControlsOverlay } from './ui/ControlsOverlay';
import { TouchOverlay } from './ui/TouchOverlay';
import { StrokeController } from './input/StrokeController';
import { WebBilliardsEngine, TrajectoryPreview, LegendRecommendation } from './physics/WebBilliardsEngine';
import { NineBallRules, NineBallPlayer, NineBallGameStatus, NineBallFoulType } from './physics/NineBallRules';
import { PuddinQuips } from './ui/PuddinQuips';

class PuddinPoolApp {
  private canvas: HTMLCanvasElement;
  private container: HTMLElement;
  private tableRenderer: TableRenderer;
  private ballRenderer: BallRenderer;
  private coachAvatar: CoachPuddinAvatar;
  private controls: ControlsOverlay;
  private touchOverlay: TouchOverlay;
  private strokeController: StrokeController;
  private physics: WebBilliardsEngine;
  private rules: NineBallRules;
  private audio: WebAudioEngine;

  private aimAngle: number = 0;
  private lastShotPower: number = 0.5;
  private isPointerDownOnTable: boolean = false;
  private isBallInHand: boolean = false;
  private isAwaitingBallPlacement: boolean = false;
  private isDraggingCueBall: boolean = false;
  private cueBallHoldTimer: number | null = null;
  private pointerDownTablePos: { x: number; y: number } | null = null;
  private isPushOutPending: boolean = false;
  private currentTrajectory: TrajectoryPreview | null = null;
  private activeLegendRec: LegendRecommendation | null = null;
  private currentCueTension: number = 0.25;
  private lastTimestamp: number = 0;
  private p1Score: number = 0;
  private p2Score: number = 0;
  private isRackWon: boolean = false;
  private autoRestartTimerId: number | null = null;
  private turnTimerRemaining: number = 35.0;
  private readonly TURN_TIME_LIMIT: number = 35.0;

  constructor() {
    this.canvas = document.getElementById('table-canvas') as HTMLCanvasElement;
    this.container = document.getElementById('game-root') || document.getElementById('app') as HTMLElement;

    this.tableRenderer = new TableRenderer(this.canvas, this.container);
    this.ballRenderer = new BallRenderer(this.canvas.getContext('2d')!);
    this.audio = new WebAudioEngine();
    this.coachAvatar = new CoachPuddinAvatar('coach-avatar-canvas', 'elder-wisdom-root', this.audio);
    this.controls = new ControlsOverlay('controls-root');
    this.touchOverlay = new TouchOverlay('touch-overlay-root');
    this.strokeController = new StrokeController('stroke-root');
    this.physics = new WebBilliardsEngine();
    this.rules = new NineBallRules();

    this.initCapacitor();
    this.initEvents();
    this.handleResize();
    window.addEventListener('resize', () => this.handleResize());
    window.addEventListener('orientationchange', () => this.handleResize());

    this.updateWpaHud();
    this.updateTrajectory();

    // Start 120Hz RAF render loop
    requestAnimationFrame((t) => this.renderLoop(t));
  }

  private async initCapacitor(): Promise<void> {
    try {
      await ScreenOrientation.lock({ orientation: 'landscape' });
    } catch {}

    try {
      await StatusBar.hide();
    } catch {}
  }

  private initEvents(): void {
    // 1. Controls Overlay Bindings
    this.controls.onAimAngleChanged = (angleRad) => {
      this.aimAngle = angleRad;
      this.updateTrajectory();
    };

    this.controls.onTableSizeChanged = (lengthMeters, widthMeters) => {
      this.physics.setTableDimensions(lengthMeters, widthMeters);
      this.rules.reset();
      this.isBallInHand = false;
      this.handleResize();
      this.updateWpaHud();
      this.updateTrajectory();
      const isValley = (lengthMeters < 2.0);
      this.coachAvatar.setEmote(
        PuddinEmoteState.ADVICE,
        isValley 
          ? "Switched to Valley 7-Foot Bar Table (38\"x76\"). Watch those tight 4.5\" corner pockets!"
          : "Switched to 9-Foot Pro Tournament Table (44\"x88\"). Simonis 860 cloth is rolling fast."
      );
    };

    this.controls.onGameModeChanged = (mode) => {
      if (this.autoRestartTimerId) {
        window.clearInterval(this.autoRestartTimerId);
        this.autoRestartTimerId = null;
      }
      this.controls.hideAutoRestartBanner();
      this.physics.resetRack(mode);
      this.rules.setGameMode(mode);
      this.isBallInHand = false;
      this.isRackWon = false;
      this.turnTimerRemaining = this.TURN_TIME_LIMIT;
      this.coachAvatar.setEmote(PuddinEmoteState.ADVICE);

      if (mode === '9ball') {
        this.coachAvatar.speak("WPA 9-Ball: Strike lowest ball first! Sinking the 9-ball legally wins, ends the game, and starts over.");
      } else if (mode === '8ball') {
        this.coachAvatar.speak("WPA 8-Ball: Open table on break. First legal pot claims Solids or Stripes. Pocket 8-ball after clearing group.");
      } else if (mode === '10ball') {
        this.coachAvatar.speak("WPA 10-Ball: Strike lowest ball first! 10-ball on break respots; legal combo or 10-ball pot wins.");
      } else if (mode === 'straight') {
        this.coachAvatar.speak("14.1 Straight Pool: Any ball pocketed scores 1 point! First to 14 points takes the match.");
      } else if (mode === 'practice') {
        this.coachAvatar.speak("Practice Mode: Unlimited guidelines, free cue ball positioning anywhere, and instant table resets.");
      } else {
        this.coachAvatar.speak(`Mode switched to ${mode}. Rack reset and ready.`);
      }
      this.updateWpaHud();
      this.updateTrajectory();
    };

    this.controls.onRerackRequested = () => {
      if (this.autoRestartTimerId) {
        window.clearInterval(this.autoRestartTimerId);
        this.autoRestartTimerId = null;
      }
      this.controls.hideAutoRestartBanner();
      this.physics.resetRack(this.physics.currentMode);
      this.rules.reset();
      this.isBallInHand = false;
      this.isPushOutPending = false;
      this.isRackWon = false;
      this.turnTimerRemaining = this.TURN_TIME_LIMIT;
      this.activeLegendRec = null;
      this.coachAvatar.setLockLineVisible(false);
      this.coachAvatar.setEmote(PuddinEmoteState.IDLE, "Fresh rack set tight! Player 1 breaks.");
      this.updateWpaHud();
      this.updateTrajectory();
    };

    this.controls.onNextRackRequested = () => {
      if (this.autoRestartTimerId) {
        window.clearInterval(this.autoRestartTimerId);
        this.autoRestartTimerId = null;
      }
      this.controls.hideAutoRestartBanner();
      this.physics.resetRack(this.physics.currentMode);
      this.rules.reset();
      this.isBallInHand = false;
      this.isPushOutPending = false;
      this.isRackWon = false;
      this.turnTimerRemaining = this.TURN_TIME_LIMIT;
      this.activeLegendRec = null;
      this.coachAvatar.setLockLineVisible(false);
      const breaker = this.rules.getCurrentPlayer() === NineBallPlayer.Player1 ? 'Player 1' : 'Player 2';
      this.coachAvatar.setEmote(PuddinEmoteState.IDLE, `New rack set tight! ${breaker} has the break.`);
      this.updateWpaHud();
      this.updateTrajectory();
    };

    this.controls.onPushOutCalled = () => {
      if (!this.rules.getPushOutAvailable() || !this.physics.isQuiescent || this.isRackWon) return;
      this.isPushOutPending = true;
      this.coachAvatar.setEmote(PuddinEmoteState.ADVICE, "Push-Out announced! Normal lowest-ball and cushion rules waived for this shot.");
      this.controls.setPushOutVisible(false);
    };

    this.controls.onPushOutResponseResolved = (accept: boolean) => {
      this.rules.resolvePushOutResponse(accept);
      const curP = this.rules.getCurrentPlayer() === NineBallPlayer.Player1 ? 'Player 1' : 'Player 2';
      if (accept) {
        this.coachAvatar.speak(`${curP} accepted the shot! Table is live.`);
      } else {
        this.coachAvatar.speak(`Shot passed back! ${curP} must take the shot.`);
      }
      this.updateWpaHud();
      this.updateTrajectory();
    };

    // 2. StrokeController Bindings
    this.strokeController.onStrokeFired = (evt) => {
      if (!this.physics.isQuiescent || this.isRackWon) return;
      this.isBallInHand = false;
      this.isAwaitingBallPlacement = false;
      this.isDraggingCueBall = false;
      this.lastShotPower = evt.power;
      this.currentCueTension = 0.0;
      this.audio.init();
      this.audio.playCueStrike(evt.power);

      this.physics.strikeCueBall(this.aimAngle, evt.impulseMps, evt.spinX, evt.spinY, evt.elevationDeg);
      this.activeLegendRec = null;
      this.coachAvatar.setLockLineVisible(false);
      this.coachAvatar.setEmote(PuddinEmoteState.ADVICE);
    };

    this.strokeController.onAimAdjusted = (deltaRad) => {
      this.controls.adjustAimAngle(deltaRad);
    };

    this.strokeController.onSpinChanged = () => {
      this.updateTrajectory();
    };

    this.strokeController.onTensionChanged = (tensionNorm) => {
      this.currentCueTension = tensionNorm;
    };

    // 3. Ergonomic Mobile TouchOverlay Bindings (Left Dial & Right Slider)
    this.touchOverlay.onAimAdjusted = (deltaRad) => {
      this.controls.adjustAimAngle(deltaRad);
    };

    this.touchOverlay.onTensionChanged = (tensionNorm) => {
      this.currentCueTension = tensionNorm;
    };

    this.touchOverlay.onStrokeFired = (powerNorm) => {
      if (!this.physics.isQuiescent || this.isRackWon) return;
      this.isBallInHand = false;
      this.isAwaitingBallPlacement = false;
      this.isDraggingCueBall = false;
      this.lastShotPower = powerNorm;
      this.currentCueTension = 0.0;
      this.audio.init();
      this.audio.playCueStrike(powerNorm);

      const impulseMps = 0.8 + powerNorm * 8.7;
      const spin = this.strokeController.getSpin();
      this.physics.strikeCueBall(this.aimAngle, impulseMps, spin.x, spin.y, 2.5);
      this.activeLegendRec = null;
      this.coachAvatar.setLockLineVisible(false);
      this.coachAvatar.setEmote(PuddinEmoteState.ADVICE);
    };

    // 4. Interactive Coach Puddin Legend System Bindings
    this.coachAvatar.onAskAdvice = () => {
      if (!this.physics.isQuiescent) return;
      const rec = this.physics.getLegendRecommendation();
      this.activeLegendRec = rec;
      this.coachAvatar.speak(rec.advice);
      this.coachAvatar.setLockLineVisible(true);
      this.coachAvatar.setEmote(PuddinEmoteState.ADVICE);
    };

    this.coachAvatar.onLockLine = () => {
      if (!this.activeLegendRec) return;
      this.aimAngle = this.activeLegendRec.recommendedAimAngle;
      this.controls.setAimAngle(this.aimAngle);
      this.strokeController.setTip(
        this.activeLegendRec.recommendedSpin[0],
        this.activeLegendRec.recommendedSpin[1]
      );
      this.updateTrajectory();
      this.coachAvatar.speak("Line locked, son! Stroke it smooth.");
    };

    // 5. Table Touch & Aim Interaction
    this.canvas.addEventListener('pointerdown', (e) => {
      this.audio.init();
      if (!this.physics.isQuiescent) return;

      const rect = this.canvas.getBoundingClientRect();
      const px = (e.clientX - rect.left) * (this.canvas.width / rect.width);
      const py = (e.clientY - rect.top) * (this.canvas.height / rect.height);

      const tableX = (px - this.tableRenderer.offsetX) / this.tableRenderer.scale;
      const tableY = (py - this.tableRenderer.offsetY) / this.tableRenderer.scale;

      if (this.isBallInHand || this.physics.isBreakShot) {
        if (this.isBallInHand && this.isAwaitingBallPlacement) {
          // First tap/click places cue ball and enables aiming
          const minX = this.physics.BALL_RADIUS * 1.5;
          const maxX = this.physics.isBreakShot
            ? (this.physics.TABLE_WIDTH * 0.25 - this.physics.BALL_RADIUS)
            : (this.physics.TABLE_WIDTH - this.physics.BALL_RADIUS * 1.5);
          const minY = this.physics.BALL_RADIUS * 1.5;
          const maxY = this.physics.TABLE_HEIGHT - this.physics.BALL_RADIUS * 1.5;
          const clampedX = Math.max(minX, Math.min(maxX, tableX));
          const clampedY = Math.max(minY, Math.min(maxY, tableY));

          if (this.physics.placeCueBall(clampedX, clampedY)) {
            this.audio.playCushionCollision(0.8);
            this.isAwaitingBallPlacement = false;
            this.updateTrajectory();
          }
          return;
        }

        // Cue ball is already placed: check if tapping on cue ball to reposition
        const cue = this.physics.balls[0];
        const distToCue = cue ? Math.hypot(tableX - cue.x, tableY - cue.y) : 999;
        const grabRadius = this.physics.BALL_RADIUS * 2.2;

        if (distToCue <= grabRadius) {
          // Tap & hold or drag to move ball again
          this.pointerDownTablePos = { x: tableX, y: tableY };
          if (this.cueBallHoldTimer) window.clearTimeout(this.cueBallHoldTimer);
          this.cueBallHoldTimer = window.setTimeout(() => {
            this.isDraggingCueBall = true;
          }, 150);
          return;
        }

        // Tapped away from cue ball: tap to aim
      }

      this.isPointerDownOnTable = true;
      this.updateAimFromPointer(px, py);
    });

    window.addEventListener('pointermove', (e) => {
      if (!this.physics.isQuiescent) return;

      const rect = this.canvas.getBoundingClientRect();
      const px = (e.clientX - rect.left) * (this.canvas.width / rect.width);
      const py = (e.clientY - rect.top) * (this.canvas.height / rect.height);
      const tableX = (px - this.tableRenderer.offsetX) / this.tableRenderer.scale;
      const tableY = (py - this.tableRenderer.offsetY) / this.tableRenderer.scale;

      if ((this.isBallInHand || this.physics.isBreakShot) && this.pointerDownTablePos && !this.isDraggingCueBall) {
        const moveDist = Math.hypot(tableX - this.pointerDownTablePos.x, tableY - this.pointerDownTablePos.y);
        if (moveDist > this.physics.BALL_RADIUS * 0.4) {
          this.isDraggingCueBall = true;
          if (this.cueBallHoldTimer) {
            window.clearTimeout(this.cueBallHoldTimer);
            this.cueBallHoldTimer = null;
          }
        }
      }

      if (this.isDraggingCueBall) {
        const minX = this.physics.BALL_RADIUS * 1.5;
        const maxX = this.physics.isBreakShot
          ? (this.physics.TABLE_WIDTH * 0.25 - this.physics.BALL_RADIUS)
          : (this.physics.TABLE_WIDTH - this.physics.BALL_RADIUS * 1.5);
        const minY = this.physics.BALL_RADIUS * 1.5;
        const maxY = this.physics.TABLE_HEIGHT - this.physics.BALL_RADIUS * 1.5;
        const clampedX = Math.max(minX, Math.min(maxX, tableX));
        const clampedY = Math.max(minY, Math.min(maxY, tableY));

        if (this.physics.isPlacementValid(clampedX, clampedY)) {
          this.physics.placeCueBall(clampedX, clampedY);
          this.updateTrajectory();
        }
        return;
      }

      if (this.isPointerDownOnTable) {
        this.updateAimFromPointer(px, py);
      }
    });

    const stopTableAim = () => {
      if (this.cueBallHoldTimer) {
        window.clearTimeout(this.cueBallHoldTimer);
        this.cueBallHoldTimer = null;
      }
      if (this.isDraggingCueBall) {
        this.isDraggingCueBall = false;
        this.audio.playCushionCollision(0.8);
        this.updateTrajectory();
      }
      this.pointerDownTablePos = null;
      this.isPointerDownOnTable = false;
    };
    window.addEventListener('pointerup', stopTableAim);
    window.addEventListener('pointercancel', stopTableAim);
  }

  private updateAimFromPointer(px: number, py: number): void {
    const cue = this.physics.balls[0];
    if (!cue || cue.isSunk) return;

    const tableX = (px - this.tableRenderer.offsetX) / this.tableRenderer.scale;
    const tableY = (py - this.tableRenderer.offsetY) / this.tableRenderer.scale;

    const dx = tableX - cue.x;
    const dy = tableY - cue.y;
    const angle = Math.atan2(dy, dx);
    this.aimAngle = angle;
    this.controls.setAimAngle(angle);
    this.activeLegendRec = null;
    this.coachAvatar.setLockLineVisible(false);
    this.updateTrajectory();
  }

  private updateTrajectory(): void {
    const spin = this.strokeController ? this.strokeController.getSpin() : { x: 0, y: 0 };
    this.currentTrajectory = this.physics.predictTrajectory(this.aimAngle, spin.x, spin.y);
  }

  private handleResize(): void {
    this.tableRenderer.resize(this.physics.TABLE_WIDTH, this.physics.TABLE_HEIGHT);
  }

  private updateWpaHud(): void {
    const isP1 = (this.rules.getCurrentPlayer() === NineBallPlayer.Player1);
    const p1Fouls = this.rules.getFoulCount(NineBallPlayer.Player1);
    const p2Fouls = this.rules.getFoulCount(NineBallPlayer.Player2);
    const curP = isP1 ? 'P1' : 'P2';
    const group = this.rules.getPlayerGroup(this.rules.getCurrentPlayer());
    const p1Group = this.rules.getPlayerGroup(NineBallPlayer.Player1);
    const p2Group = this.rules.getPlayerGroup(NineBallPlayer.Player2);

    const pText = (group !== 'OPEN' && this.rules.getGameMode() === '8ball')
      ? `${curP} (${group})`
      : `${curP} SHOOTING`;

    const fText = `Fouls: P1:${p1Fouls} | P2:${p2Fouls}`;
    this.controls.setTurnInfo(pText, fText, this.isBallInHand);
    this.controls.setPushOutVisible(this.rules.getPushOutAvailable());

    let matchStatus = this.physics.isBreakShot ? "BREAK SHOT" : (this.isBallInHand ? "BALL IN HAND" : `${curP} TO SHOOT`);
    if (this.isRackWon) matchStatus = "RACK FINISHED";
    this.controls.setPlayerProfiles(this.p1Score, this.p2Score, p1Group, p2Group, isP1, matchStatus);
  }

  private onShotCompleted(): void {
    const lowest = this.physics.getLowestBall();
    const unsunk = this.physics.balls.filter(b => b.id > 0 && !b.isSunk).map(b => b.id);

    const result = this.rules.evaluateShot({
      shooter: this.rules.getCurrentPlayer(),
      isBreakShot: this.physics.isBreakShot,
      isPushOutCall: this.isPushOutPending,
      firstContactBallId: this.physics.firstContactBallId,
      railHitAfterContact: this.physics.railHitAfterContact,
      objectBallsHitRailsCount: this.physics.objectBallsHitRailIds.size,
      ballsPocketed: this.physics.ballsPocketedThisShot,
      cueScratch: this.physics.cueScratchThisShot,
    }, lowest, unsunk);

    const wasBreak = this.physics.isBreakShot;
    this.isPushOutPending = false;
    this.physics.isBreakShot = false;
    this.turnTimerRemaining = this.TURN_TIME_LIMIT;

    if (result.accoladeText) {
      this.controls.showAccolade(result.accoladeText);
    }

    if (result.nineBallRespotted || result.tenBallRespotted) {
      this.physics.respotNineBall();
    }

    if (result.ballInHand) {
      this.isBallInHand = true;
      this.isAwaitingBallPlacement = true;
      if (this.physics.balls[0].isSunk) {
        this.physics.placeCueBall(this.physics.TABLE_WIDTH * 0.25, this.physics.TABLE_HEIGHT * 0.5);
      }
    }

    if (result.gameStatus === NineBallGameStatus.PushOutPendingResponse) {
      const shooterName = this.rules.getCurrentPlayer() === NineBallPlayer.Player1 ? 'Player 1' : 'Player 2';
      const opponentName = this.rules.getCurrentPlayer() === NineBallPlayer.Player1 ? 'Player 2' : 'Player 1';
      this.controls.showPushOutModal(shooterName, opponentName);
    }

    const isWin = (result.gameStatus === NineBallGameStatus.Player1Win || result.gameStatus === NineBallGameStatus.Player2Win);
    if (isWin) {
      this.isRackWon = true;
      const winner = (result.gameStatus === NineBallGameStatus.Player1Win) ? NineBallPlayer.Player1 : NineBallPlayer.Player2;
      const winnerName = (winner === NineBallPlayer.Player1) ? 'PLAYER 1' : 'PLAYER 2';
      if (winner === NineBallPlayer.Player1) {
        this.p1Score++;
      } else {
        this.p2Score++;
      }
      this.controls.setScore(this.p1Score, this.p2Score);
      this.audio.playVictoryStinger();

      const winTitle = `${winnerName} WINS THE GAME!`;
      const winSub = wasBreak
        ? 'GOLDEN BREAK ON THE BREAK SHOT!'
        : (result.foulType === NineBallFoulType.ThreeConsecutiveFouls 
            ? 'Opponent 3 Consecutive Fouls Forfeit' 
            : (this.physics.currentMode === '9ball' 
                ? '9-Ball Sunk Legally' 
                : (this.physics.currentMode === '10ball' 
                    ? '10-Ball Sunk Legally' 
                    : (this.physics.currentMode === 'straight' ? '14 Points High Run Reached' : '8-Ball Pocketed Clean'))));

      // Sinking 9-ball / Winning shot ends game and starts over automatically in 3 seconds!
      let remaining = 3;
      this.controls.showAutoRestartBanner(winTitle, winSub, remaining);
      if (this.autoRestartTimerId) window.clearInterval(this.autoRestartTimerId);
      this.autoRestartTimerId = window.setInterval(() => {
        remaining--;
        this.controls.updateAutoRestartCountdown(remaining);
        if (remaining <= 0) {
          if (this.autoRestartTimerId) {
            window.clearInterval(this.autoRestartTimerId);
            this.autoRestartTimerId = null;
          }
          this.controls.hideAutoRestartBanner();
          this.controls.onNextRackRequested?.();
        }
      }, 1000);
    }

    const isLoss = !result.isLegal && (
      result.foulType === NineBallFoulType.EarlyEightBall ||
      result.foulType === NineBallFoulType.EightBallScratch ||
      result.foulType === NineBallFoulType.ThreeConsecutiveFouls
    );

    const quip = PuddinQuips.getQuip({
      isBreak: wasBreak,
      power: this.lastShotPower,
      cueScratch: this.physics.cueScratchThisShot,
      isFoul: !result.isLegal,
      foulReason: result.foulType,
      ballsPocketed: this.physics.ballsPocketedThisShot,
      ninePocketed: this.physics.ballsPocketedThisShot.includes(9),
      eightPocketed: this.physics.ballsPocketedThisShot.includes(8),
      isWin,
      isLoss,
    });

    const emote = isWin
      ? PuddinEmoteState.CHEER
      : (!result.isLegal || this.physics.cueScratchThisShot)
        ? PuddinEmoteState.CONSOLE
        : PuddinEmoteState.IDLE;

    this.coachAvatar.setEmote(emote, quip);

    this.updateWpaHud();
    this.updateTrajectory();
  }

  private renderLoop(timestamp: number): void {
    if (!this.lastTimestamp) this.lastTimestamp = timestamp;
    const dt = Math.min((timestamp - this.lastTimestamp) / 1000.0, 0.05);
    this.lastTimestamp = timestamp;

    if (!this.physics.isQuiescent) {
      const substeps = 4;
      const subDt = dt / substeps;
      for (let s = 0; s < substeps; ++s) {
        this.physics.step(
          subDt,
          (speed) => this.audio.playBallCollision(speed),
          () => this.audio.playCushionCollision(1.5),
          () => {
            this.audio.playPocketDrop();
          }
        );
      }

      let totalV = 0;
      for (const b of this.physics.balls) {
        if (!b.isSunk) totalV += Math.hypot(b.vx, b.vy);
      }
      this.audio.updateRollingSound(totalV);

      if (this.physics.isQuiescent) {
        this.onShotCompleted();
      }
    } else if (!this.isRackWon && this.physics.currentMode !== 'practice') {
      // Turn Timer countdown in active competitive modes
      this.turnTimerRemaining -= dt;
      const isP1 = (this.rules.getCurrentPlayer() === NineBallPlayer.Player1);
      const ratio = Math.max(0, this.turnTimerRemaining / this.TURN_TIME_LIMIT);
      this.controls.setTurnTimer(isP1, ratio);

      if (this.turnTimerRemaining <= 0) {
        // Time expired foul
        this.turnTimerRemaining = this.TURN_TIME_LIMIT;
        this.audio.playFoulBuzzer();
        this.controls.showAccolade("⏰ TIME FOUL!");
        this.isBallInHand = true;
        this.isAwaitingBallPlacement = true;
        this.coachAvatar.speak("Shot clock expired! Opponent receives Ball-in-Hand.");
        this.updateWpaHud();
      }
    }

    // Render Scene with TableRenderer and BallRenderer
    this.tableRenderer.beginScene();
    this.tableRenderer.drawRails(this.physics.pockets, this.currentTrajectory?.targetPocketIndex);
    this.tableRenderer.drawLegendRoute(this.activeLegendRec);
    this.tableRenderer.drawTrajectory(this.currentTrajectory, this.physics.BALL_RADIUS);
    this.tableRenderer.drawContactAmbientOcclusion(this.physics.balls, this.physics.BALL_RADIUS);

    const lowestBallId = (this.physics.isQuiescent && !this.isRackWon) ? this.physics.getLowestBall() : undefined;
    this.ballRenderer.drawBalls(
      this.physics.balls,
      this.physics.BALL_RADIUS,
      this.tableRenderer.scale,
      this.isBallInHand,
      lowestBallId
    );

    if (this.physics.isQuiescent && !this.isRackWon) {
      this.ballRenderer.drawCueStick(
        this.physics.balls[0],
        this.aimAngle,
        this.currentCueTension,
        this.tableRenderer.scale,
        this.physics.BALL_RADIUS
      );
    }

    this.tableRenderer.endScene();

    requestAnimationFrame((t) => this.renderLoop(t));
  }
}

// Initialize on DOM load
window.addEventListener('DOMContentLoaded', () => {
  new PuddinPoolApp();

  if ('serviceWorker' in navigator && window.location.protocol.startsWith('http')) {
    navigator.serviceWorker.register('./sw.js').catch(() => {});
  }
});
