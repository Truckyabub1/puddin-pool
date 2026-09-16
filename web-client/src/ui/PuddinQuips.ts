/**
 * PuddinQuips: Subtle, Sharp, and Hilarious Smart-Ass Commentary Engine
 * Contextual post-shot one-liners delivered by Coach Puddin after each turn.
 */

export interface ShotContext {
  isBreak: boolean;
  power: number;              // 0.0 to 1.0
  cueScratch: boolean;
  isFoul: boolean;
  foulReason?: string;
  ballsPocketed: number[];
  ninePocketed: boolean;
  eightPocketed: boolean;
  isWin: boolean;
  isLoss: boolean;
}

export class PuddinQuips {
  private static lastQuipIndex = -1;

  private static readonly SCRATCH_QUIPS = [
    "White ball stays on the green, genius.",
    "Did that pocket look thirsty to you?",
    "Great shot... if you were playing against yourself.",
    "Cue ball took a dive. You owe the house a quarter.",
    "The object is to pocket the OTHER balls, son.",
    "Gravity works. You just proved it again.",
  ];

  private static readonly OVERPOWERED_QUIPS = [
    "Easy on the lumber, slugger. This isn't baseball.",
    "Trying to crack the slate or just your wrist?",
    "Power without aim is just expensive noise.",
    "Cue ball almost made it into the parking lot.",
    "Simmer down, Hercules. It's a game of millimeters.",
  ];

  private static readonly WEAK_QUIPS = [
    "Hit it with your purse next time, son.",
    "Did you blow on it or actually stroke it?",
    "The felt grew a millimeter while that ball was rolling.",
    "Put some mustard on it next time.",
    "That ball died of old age halfway there.",
  ];

  private static readonly DRY_MISS_QUIPS = [
    "Aiming is completely optional today, apparently.",
    "The balls didn't move, but the air sure felt it.",
    "The table geometry is playing you, kid.",
    "That ball had an appointment in another zip code.",
    "Don't quit your day job just yet.",
    "Well, the cue stick stayed in one piece at least.",
  ];

  private static readonly FLUKE_QUIPS = [
    "Don't pretend you planned that.",
    "God loves a fool, and today that's you.",
    "Ugly counts on the scoreboard, I guess.",
    "Banked it off pure luck and sheer ignorance.",
    "I'd act confident too if I got away with that.",
  ];

  private static readonly CLEAN_POT_QUIPS = [
    "Look at that, you accidentally looked like a player.",
    "Don't let it go to your head.",
    "Even a blind squirrel finds a nut.",
    "Acceptable. Let's see if you can do it twice.",
    "Stroke was almost decent. Don't ruin it.",
  ];

  private static readonly BREAK_QUIPS = [
    "Lot of noise. Let's see if there's any skill behind it.",
    "Well, they're scattered. Now the hard part.",
    "Table looks like a yard sale. Get to work.",
    "Balls moved. Let's see if your brain does too.",
  ];

  private static readonly FOUL_QUIPS = [
    "Colorblind, or just naturally optimistic?",
    "Wrong ball, chief. Numbers go in order.",
    "Rules are in the rulebook, not your imagination.",
    "Ball-in-hand. You just gift-wrapped the rack.",
  ];

  private static readonly WIN_QUIPS = [
    "Well, I'll be damned. Pure daylight robbery.",
    "You actually won. Drinks are on you.",
    "Miracles happen every day. Here's proof.",
  ];

  private static readonly LOSS_QUIPS = [
    "Pack your cue. That was painful to watch.",
    "Back to the practice rack, kid.",
    "I've seen better form at a bowling alley.",
  ];

  public static getQuip(ctx: ShotContext): string {
    let pool: string[];

    if (ctx.isWin) {
      pool = this.WIN_QUIPS;
    } else if (ctx.isLoss) {
      pool = this.LOSS_QUIPS;
    } else if (ctx.cueScratch) {
      pool = this.SCRATCH_QUIPS;
    } else if (ctx.isBreak) {
      pool = (ctx.ballsPocketed.length > 0)
        ? ["Nice break. Don't blow the runout.", "Balls dropped on the break. Now play smart."]
        : this.BREAK_QUIPS;
    } else if (ctx.isFoul) {
      pool = this.FOUL_QUIPS;
    } else if (ctx.ballsPocketed.length > 0) {
      if (ctx.power > 0.85) {
        pool = ["Slammed it in, but the cue ball is lost in the woods.", ...this.FLUKE_QUIPS];
      } else if (ctx.power < 0.25) {
        pool = ["Rolled in by the grace of clean felt.", ...this.CLEAN_POT_QUIPS];
      } else {
        pool = this.CLEAN_POT_QUIPS;
      }
    } else {
      // Missed shot
      if (ctx.power > 0.85) {
        pool = this.OVERPOWERED_QUIPS;
      } else if (ctx.power < 0.25) {
        pool = this.WEAK_QUIPS;
      } else {
        pool = this.DRY_MISS_QUIPS;
      }
    }

    // Avoid consecutive repeats
    let idx = Math.floor(Math.random() * pool.length);
    if (idx === this.lastQuipIndex && pool.length > 1) {
      idx = (idx + 1) % pool.length;
    }
    this.lastQuipIndex = idx;
    return pool[idx];
  }
}
