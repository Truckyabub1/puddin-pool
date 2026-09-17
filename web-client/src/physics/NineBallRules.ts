/**
 * Official WPA / BCA Tournament Rules Arbitrator (TypeScript)
 * Implements:
 * - WPA 9-Ball: Diamond rack, lowest-ball rotation, push-out, 3 consecutive foul penalty, 9-ball respotting.
 * - WPA 8-Ball: Open table on break, Solids (1-7) vs Stripes (9-15) group assignment, legal first contact,
 *               pocketing 8-ball for win, early 8-ball loss, and scratch on 8-ball loss.
 */

export enum NineBallPlayer {
  Player1 = 1,
  Player2 = 2,
}

export enum NineBallFoulType {
  None = 'None',
  CueBallScratch = 'CueBallScratch',
  WrongFirstContact = 'WrongFirstContact',
  NoRailAfterContact = 'NoRailAfterContact',
  IllegalBreak = 'IllegalBreak',
  ThreeConsecutiveFouls = 'ThreeConsecutiveFouls',
  EarlyEightBall = 'EarlyEightBall',
  EightBallScratch = 'EightBallScratch',
}

export enum NineBallGameStatus {
  RackInProgress = 'RackInProgress',
  Player1Win = 'Player1Win',
  Player2Win = 'Player2Win',
  PushOutPendingResponse = 'PushOutPendingResponse',
}

export interface NineBallShotInput {
  shooter: NineBallPlayer;
  isBreakShot: boolean;
  isPushOutCall: boolean;
  firstContactBallId: number;
  railHitAfterContact: boolean;
  objectBallsHitRailsCount?: number;
  ballsPocketed: number[];
  cueScratch: boolean;
}

export interface NineBallShotResult {
  isLegal: boolean;
  foulType: NineBallFoulType;
  gameStatus: NineBallGameStatus;
  nextShooter: NineBallPlayer;
  ballInHand: boolean;
  nineBallRespotted: boolean;
  player1FoulCount: number;
  player2FoulCount: number;
  coachPuddinWarningTriggered: boolean;
  coachPuddinMessage: string;
}

export class NineBallRules {
  public static readonly FOOT_SPOT_X = 2.24 * 0.70;
  public static readonly FOOT_SPOT_Y = 1.12 * 0.50;
  public static readonly BALL_RADIUS = 0.028575;

  private gameMode: string = '9ball';
  private currentPlayer: NineBallPlayer = NineBallPlayer.Player1;
  private gameStatus: NineBallGameStatus = NineBallGameStatus.RackInProgress;
  private player1ConsecutiveFouls: number = 0;
  private player2ConsecutiveFouls: number = 0;
  private isFirstShotAfterBreak: boolean = false;
  private pushOutAvailable: boolean = false;

  // 8-Ball State
  private isTableOpen: boolean = true;
  private solidsPlayer: NineBallPlayer | null = null;
  private stripesPlayer: NineBallPlayer | null = null;

  constructor() {
    this.reset();
  }

  public setGameMode(mode: string): void {
    this.gameMode = mode;
    this.reset();
  }

  public getGameMode(): string {
    return this.gameMode;
  }

  public reset(): void {
    this.currentPlayer = NineBallPlayer.Player1;
    this.gameStatus = NineBallGameStatus.RackInProgress;
    this.player1ConsecutiveFouls = 0;
    this.player2ConsecutiveFouls = 0;
    this.isFirstShotAfterBreak = false;
    this.pushOutAvailable = false;
    this.isTableOpen = true;
    this.solidsPlayer = null;
    this.stripesPlayer = null;
  }

  public getPushOutAvailable(): boolean {
    return this.pushOutAvailable;
  }

  public getCurrentPlayer(): NineBallPlayer {
    return this.currentPlayer;
  }

  public getGameStatus(): NineBallGameStatus {
    return this.gameStatus;
  }

  public getPlayerGroup(player: NineBallPlayer): string {
    if (this.gameMode !== '8ball' || this.isTableOpen) return 'OPEN';
    if (this.solidsPlayer === player) return 'SOLIDS (1-7)';
    if (this.stripesPlayer === player) return 'STRIPES (9-15)';
    return 'OPEN';
  }

  public getFoulCount(player: NineBallPlayer): number {
    return player === NineBallPlayer.Player1 ? this.player1ConsecutiveFouls : this.player2ConsecutiveFouls;
  }

  public resolvePushOutResponse(accept: boolean): void {
    if (this.gameStatus !== NineBallGameStatus.PushOutPendingResponse) return;
    const opponent = this.getOpponent(this.currentPlayer);
    if (accept) {
      this.currentPlayer = opponent;
    }
    this.gameStatus = NineBallGameStatus.RackInProgress;
    this.pushOutAvailable = false;
    this.isFirstShotAfterBreak = false;
  }

  public evaluateShot(shot: NineBallShotInput, lowestBallOnTable: number, unsunkBalls: number[] = []): NineBallShotResult {
    if (this.gameMode === '8ball') {
      return this.evaluate8BallShot(shot, unsunkBalls);
    }

    return this.evaluate9BallShot(shot, lowestBallOnTable);
  }

  private evaluate9BallShot(shot: NineBallShotInput, lowestBallOnTable: number): NineBallShotResult {
    const ninePocketed = shot.ballsPocketed.includes(9);

    // Push-Out handler
    if (this.pushOutAvailable && shot.isPushOutCall) {
      this.pushOutAvailable = false;
      this.isFirstShotAfterBreak = false;

      if (shot.cueScratch) {
        if (shot.shooter === NineBallPlayer.Player1) this.player1ConsecutiveFouls++;
        else this.player2ConsecutiveFouls++;
        this.currentPlayer = this.getOpponent(shot.shooter);
        return {
          isLegal: false,
          foulType: NineBallFoulType.CueBallScratch,
          gameStatus: NineBallGameStatus.RackInProgress,
          nextShooter: this.currentPlayer,
          ballInHand: true,
          nineBallRespotted: ninePocketed,
          player1FoulCount: this.player1ConsecutiveFouls,
          player2FoulCount: this.player2ConsecutiveFouls,
          coachPuddinWarningTriggered: false,
          coachPuddinMessage: "Coach Puddin: 'Scratch on a push-out gives ball-in-hand! Discipline that cue ball, son.'",
        };
      }

      this.gameStatus = NineBallGameStatus.PushOutPendingResponse;
      return {
        isLegal: true,
        foulType: NineBallFoulType.None,
        gameStatus: NineBallGameStatus.PushOutPendingResponse,
        nextShooter: this.getOpponent(shot.shooter),
        ballInHand: false,
        nineBallRespotted: ninePocketed,
        player1FoulCount: this.player1ConsecutiveFouls,
        player2FoulCount: this.player2ConsecutiveFouls,
        coachPuddinWarningTriggered: false,
        coachPuddinMessage: "Coach Puddin: 'Push-out played! Opponent decides whether to shoot or pass it back.'",
      };
    }

    this.pushOutAvailable = false;

    // Foul check
    let foul = NineBallFoulType.None;
    if (shot.cueScratch) {
      foul = NineBallFoulType.CueBallScratch;
    } else if (shot.firstContactBallId !== lowestBallOnTable) {
      foul = NineBallFoulType.WrongFirstContact;
    } else if (shot.isBreakShot) {
      // Official WPA 9.2: Pocket an object ball OR drive at least 4 numbered balls to cushion rails
      const railCount = shot.objectBallsHitRailsCount !== undefined ? shot.objectBallsHitRailsCount : (shot.railHitAfterContact ? 4 : 0);
      if (shot.ballsPocketed.length === 0 && railCount < 4) {
        foul = NineBallFoulType.IllegalBreak;
      }
    } else if (shot.ballsPocketed.length === 0 && !shot.railHitAfterContact) {
      foul = NineBallFoulType.NoRailAfterContact;
    }

    if (foul !== NineBallFoulType.None) {
      const foulsRef = shot.shooter === NineBallPlayer.Player1 ? ++this.player1ConsecutiveFouls : ++this.player2ConsecutiveFouls;
      this.currentPlayer = this.getOpponent(shot.shooter);

      if (foulsRef >= 3) {
        this.gameStatus = shot.shooter === NineBallPlayer.Player1 ? NineBallGameStatus.Player2Win : NineBallGameStatus.Player1Win;
        return {
          isLegal: false,
          foulType: NineBallFoulType.ThreeConsecutiveFouls,
          gameStatus: this.gameStatus,
          nextShooter: this.currentPlayer,
          ballInHand: true,
          nineBallRespotted: ninePocketed,
          player1FoulCount: this.player1ConsecutiveFouls,
          player2FoulCount: this.player2ConsecutiveFouls,
          coachPuddinWarningTriggered: false,
          coachPuddinMessage: "Coach Puddin: 'Three consecutive fouls! That is a rack forfeit according to official WPA rules.'",
        };
      }

      let msg = "Coach Puddin: 'Foul! Opponent receives Ball-in-Hand anywhere on the felt.'";
      let warningTriggered = false;
      if (foulsRef === 2) {
        warningTriggered = true;
        msg = "Coach Puddin: 'Careful, son! That is TWO consecutive fouls! One more foul forfeits the rack.'";
      } else if (foul === NineBallFoulType.WrongFirstContact) {
        msg = `Coach Puddin: 'Foul! Lowest ball was the ${lowestBallOnTable}-ball. You must strike it first.'`;
      } else if (foul === NineBallFoulType.IllegalBreak) {
        msg = "Coach Puddin: 'Illegal break! A dry break requires at least 4 object balls to hit a rail. Opponent has Ball-in-Hand.'";
      } else if (foul === NineBallFoulType.NoRailAfterContact) {
        msg = "Coach Puddin: 'Foul! After contact, at least one ball must reach a cushion rail or drop in a pocket.'";
      }

      return {
        isLegal: false,
        foulType: foul,
        gameStatus: NineBallGameStatus.RackInProgress,
        nextShooter: this.currentPlayer,
        ballInHand: true,
        nineBallRespotted: ninePocketed,
        player1FoulCount: this.player1ConsecutiveFouls,
        player2FoulCount: this.player2ConsecutiveFouls,
        coachPuddinWarningTriggered: warningTriggered,
        coachPuddinMessage: msg,
      };
    }

    // Legal Shot
    if (shot.shooter === NineBallPlayer.Player1) this.player1ConsecutiveFouls = 0;
    else this.player2ConsecutiveFouls = 0;

    if (ninePocketed) {
      this.gameStatus = shot.shooter === NineBallPlayer.Player1 ? NineBallGameStatus.Player1Win : NineBallGameStatus.Player2Win;
      const msg = shot.isBreakShot
        ? "Coach Puddin: 'GOLDEN BREAK! 9-ball pocketed on the break! Pure masterclass.'"
        : "Coach Puddin: 'Sensational combination! 9-ball dropped legally for the WIN!'";
      return {
        isLegal: true,
        foulType: NineBallFoulType.None,
        gameStatus: this.gameStatus,
        nextShooter: shot.shooter,
        ballInHand: false,
        nineBallRespotted: false,
        player1FoulCount: this.player1ConsecutiveFouls,
        player2FoulCount: this.player2ConsecutiveFouls,
        coachPuddinWarningTriggered: false,
        coachPuddinMessage: msg,
      };
    }

    if (shot.isBreakShot) {
      this.isFirstShotAfterBreak = true;
      this.pushOutAvailable = true;
    }

    if (shot.ballsPocketed.length > 0) {
      this.currentPlayer = shot.shooter;
      return {
        isLegal: true,
        foulType: NineBallFoulType.None,
        gameStatus: NineBallGameStatus.RackInProgress,
        nextShooter: shot.shooter,
        ballInHand: false,
        nineBallRespotted: false,
        player1FoulCount: this.player1ConsecutiveFouls,
        player2FoulCount: this.player2ConsecutiveFouls,
        coachPuddinWarningTriggered: false,
        coachPuddinMessage: "Coach Puddin: 'Legal shot. Stay centered and trust your angle.'",
      };
    }

    // Dry legal shot -> turn passes
    this.currentPlayer = this.getOpponent(shot.shooter);
    return {
      isLegal: true,
      foulType: NineBallFoulType.None,
      gameStatus: NineBallGameStatus.RackInProgress,
      nextShooter: this.currentPlayer,
      ballInHand: false,
      nineBallRespotted: false,
      player1FoulCount: this.player1ConsecutiveFouls,
      player2FoulCount: this.player2ConsecutiveFouls,
      coachPuddinWarningTriggered: false,
      coachPuddinMessage: "Coach Puddin: 'Solid safety play. Table passes to opponent.'",
    };
  }

  private evaluate8BallShot(shot: NineBallShotInput, unsunkBalls: number[]): NineBallShotResult {
    const eightPocketed = shot.ballsPocketed.includes(8);
    const shooter = shot.shooter;
    const opponent = this.getOpponent(shooter);

    // Count remaining balls in groups
    const remainingSolids = unsunkBalls.filter(id => id >= 1 && id <= 7).length;
    const remainingStripes = unsunkBalls.filter(id => id >= 9 && id <= 15).length;
    const shooterIsSolids = (this.solidsPlayer === shooter);
    const shooterRemaining = this.isTableOpen ? 7 : (shooterIsSolids ? remainingSolids : remainingStripes);

    // 1. Check premature 8-ball pocketing or scratch on 8-ball
    if (eightPocketed) {
      if (shot.cueScratch) {
        // Scratch while pocketing 8-ball = instant LOSS
        this.gameStatus = shooter === NineBallPlayer.Player1 ? NineBallGameStatus.Player2Win : NineBallGameStatus.Player1Win;
        return {
          isLegal: false,
          foulType: NineBallFoulType.EightBallScratch,
          gameStatus: this.gameStatus,
          nextShooter: opponent,
          ballInHand: false,
          nineBallRespotted: false,
          player1FoulCount: 0,
          player2FoulCount: 0,
          coachPuddinWarningTriggered: false,
          coachPuddinMessage: "Coach Puddin: 'Scratch while pocketing the 8-ball is an automatic LOSS of game!'",
        };
      }

      if (shot.isBreakShot) {
        // Clean 8-ball on break (no scratch): win per casual/BCA guidelines
        this.gameStatus = shooter === NineBallPlayer.Player1 ? NineBallGameStatus.Player1Win : NineBallGameStatus.Player2Win;
        return {
          isLegal: true,
          foulType: NineBallFoulType.None,
          gameStatus: this.gameStatus,
          nextShooter: shooter,
          ballInHand: false,
          nineBallRespotted: false,
          player1FoulCount: 0,
          player2FoulCount: 0,
          coachPuddinWarningTriggered: false,
          coachPuddinMessage: "Coach Puddin: '8-Ball on the break! Masterful golden break!'",
        };
      }

      if (shooterRemaining > 0) {
        // Pocketed 8-ball before clearing group = instant LOSS
        this.gameStatus = shooter === NineBallPlayer.Player1 ? NineBallGameStatus.Player2Win : NineBallGameStatus.Player1Win;
        return {
          isLegal: false,
          foulType: NineBallFoulType.EarlyEightBall,
          gameStatus: this.gameStatus,
          nextShooter: opponent,
          ballInHand: false,
          nineBallRespotted: false,
          player1FoulCount: 0,
          player2FoulCount: 0,
          coachPuddinWarningTriggered: false,
          coachPuddinMessage: "Coach Puddin: 'Early 8-ball pocketed before your group was cleared! Automatic LOSS.'",
        };
      }

      // Legal 8-ball pocketing on cleared group = WIN
      this.gameStatus = shooter === NineBallPlayer.Player1 ? NineBallGameStatus.Player1Win : NineBallGameStatus.Player2Win;
      return {
        isLegal: true,
        foulType: NineBallFoulType.None,
        gameStatus: this.gameStatus,
        nextShooter: shooter,
        ballInHand: false,
        nineBallRespotted: false,
        player1FoulCount: 0,
        player2FoulCount: 0,
        coachPuddinWarningTriggered: false,
        coachPuddinMessage: "Coach Puddin: 'CHAMPIONSHIP STROKE! 8-Ball down clean for the match WIN!'",
      };
    }

    // 2. Foul checking in 8-ball
    let foul = NineBallFoulType.None;
    if (shot.cueScratch) {
      foul = NineBallFoulType.CueBallScratch;
    } else if (shot.firstContactBallId <= 0) {
      foul = NineBallFoulType.WrongFirstContact;
    } else if (!this.isTableOpen) {
      // Must hit shooter's group first, or 8-ball if group cleared
      if (shooterRemaining === 0) {
        if (shot.firstContactBallId !== 8) {
          foul = NineBallFoulType.WrongFirstContact;
        }
      } else {
        const contactIsSolid = (shot.firstContactBallId >= 1 && shot.firstContactBallId <= 7);
        const contactIsStripe = (shot.firstContactBallId >= 9 && shot.firstContactBallId <= 15);
        if (shooterIsSolids && !contactIsSolid) foul = NineBallFoulType.WrongFirstContact;
        if (!shooterIsSolids && !contactIsStripe) foul = NineBallFoulType.WrongFirstContact;
      }
    } else {
      // Open table: cannot hit 8-ball first
      if (shot.firstContactBallId === 8 && unsunkBalls.length > 1) {
        foul = NineBallFoulType.WrongFirstContact;
      }
    }

    if (foul === NineBallFoulType.None && shot.ballsPocketed.length === 0 && !shot.railHitAfterContact) {
      foul = NineBallFoulType.NoRailAfterContact;
    }

    if (foul !== NineBallFoulType.None) {
      this.currentPlayer = opponent;
      return {
        isLegal: false,
        foulType: foul,
        gameStatus: NineBallGameStatus.RackInProgress,
        nextShooter: opponent,
        ballInHand: true,
        nineBallRespotted: false,
        player1FoulCount: 0,
        player2FoulCount: 0,
        coachPuddinWarningTriggered: false,
        coachPuddinMessage: (foul === NineBallFoulType.WrongFirstContact)
          ? "Coach Puddin: 'Foul in 8-Ball! You must strike your own group first. Opponent has Ball-in-Hand.'"
          : "Coach Puddin: 'Foul! Opponent receives Ball-in-Hand.'",
      };
    }

    // 3. Group Assignment on Open Table (after break)
    if (this.isTableOpen && !shot.isBreakShot && shot.ballsPocketed.length > 0) {
      const pocketedSolids = shot.ballsPocketed.filter(id => id >= 1 && id <= 7).length;
      const pocketedStripes = shot.ballsPocketed.filter(id => id >= 9 && id <= 15).length;
      if (pocketedSolids > pocketedStripes) {
        this.isTableOpen = false;
        this.solidsPlayer = shooter;
        this.stripesPlayer = opponent;
      } else if (pocketedStripes > pocketedSolids) {
        this.isTableOpen = false;
        this.stripesPlayer = shooter;
        this.solidsPlayer = opponent;
      }
    }

    // 4. Continued turn or pass
    const shooterPocketedOwn = shot.ballsPocketed.some(id => {
      if (this.isTableOpen) return id !== 8;
      if (shooterIsSolids) return id >= 1 && id <= 7;
      return id >= 9 && id <= 15;
    });

    if (shooterPocketedOwn) {
      this.currentPlayer = shooter;
      return {
        isLegal: true,
        foulType: NineBallFoulType.None,
        gameStatus: NineBallGameStatus.RackInProgress,
        nextShooter: shooter,
        ballInHand: false,
        nineBallRespotted: false,
        player1FoulCount: 0,
        player2FoulCount: 0,
        coachPuddinWarningTriggered: false,
        coachPuddinMessage: "Coach Puddin: 'Nice pot! Stay on your run.'",
      };
    }

    this.currentPlayer = opponent;
    return {
      isLegal: true,
      foulType: NineBallFoulType.None,
      gameStatus: NineBallGameStatus.RackInProgress,
      nextShooter: opponent,
      ballInHand: false,
      nineBallRespotted: false,
      player1FoulCount: 0,
      player2FoulCount: 0,
      coachPuddinWarningTriggered: false,
      coachPuddinMessage: "Coach Puddin: 'Table passes to opponent. Keep your eyes sharp.'",
    };
  }

  private getOpponent(p: NineBallPlayer): NineBallPlayer {
    return p === NineBallPlayer.Player1 ? NineBallPlayer.Player2 : NineBallPlayer.Player1;
  }
}
