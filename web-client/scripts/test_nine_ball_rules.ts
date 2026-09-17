import { NineBallRules, NineBallPlayer, NineBallFoulType, NineBallGameStatus } from '../src/physics/NineBallRules';
import { WebBilliardsEngine } from '../src/physics/WebBilliardsEngine';

function assert(condition: boolean, msg: string) {
  if (!condition) {
    console.error(`❌ ASSERTION FAILED: ${msg}`);
    process.exit(1);
  }
}

console.log('======================================================');
console.log('  OFFICIAL WPA 9-BALL RULES & PHYSICS ARBITRATOR TEST');
console.log('======================================================\n');

const engine = new WebBilliardsEngine();
const rules = new NineBallRules();

// 1. Diamond Rack Test
console.log('[TEST 1] Verifying 9-Ball Diamond Rack Geometry...');
engine.resetRack('9ball');
assert(engine.balls.length === 10, 'Should have cue ball + 9 object balls');
const b1 = engine.balls.find(b => b.id === 1);
const b9 = engine.balls.find(b => b.id === 9);
assert(!!b1 && !!b9, 'Balls 1 and 9 must exist');
const apexX = engine.TABLE_WIDTH * 0.70;
const apexY = engine.TABLE_HEIGHT * 0.50;
assert(Math.abs(b1.x - apexX) < 1e-3, 'Ball 1 must be at apex/foot spot');
assert(Math.abs(b1.y - apexY) < 1e-3, 'Ball 1 must be centered vertically');
assert(b9.x > b1.x, 'Ball 9 must be inside the diamond behind ball 1');
console.log('  -> PASS: 1-ball at apex, 9-ball centered in diamond rack.\n');

// 2. Lowest-Numbered Ball Contact Violation
console.log('[TEST 2] Testing Lowest-Ball Contact Rule...');
rules.reset();
const foulWrongContact = rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: false,
  isPushOutCall: false,
  firstContactBallId: 4, // Struck 4 when 1 is lowest
  railHitAfterContact: true,
  ballsPocketed: [],
  cueScratch: false,
}, 1);
assert(!foulWrongContact.isLegal, 'Shot must be illegal');
assert(foulWrongContact.foulType === NineBallFoulType.WrongFirstContact, 'Foul must be WrongFirstContact');
assert(foulWrongContact.ballInHand === true, 'Opponent must receive Ball-in-Hand');
assert(foulWrongContact.nextShooter === NineBallPlayer.Player2, 'Turn passes to Player 2');
assert(foulWrongContact.player1FoulCount === 1, 'Player 1 foul count incremented');
console.log('  -> PASS: Striking wrong ball first awards Ball-in-Hand to opponent.\n');

// 3. No-Rail After Contact Foul
console.log('[TEST 3] Testing No-Rail After Contact Foul...');
rules.reset();
const foulNoRail = rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: false,
  isPushOutCall: false,
  firstContactBallId: 1,
  railHitAfterContact: false,
  ballsPocketed: [],
  cueScratch: false,
}, 1);
assert(!foulNoRail.isLegal, 'Shot must be illegal');
assert(foulNoRail.foulType === NineBallFoulType.NoRailAfterContact, 'Foul must be NoRailAfterContact');
assert(foulNoRail.ballInHand === true, 'Must award Ball-in-Hand');
console.log('  -> PASS: No cushion hit after contact awards Ball-in-Hand.\n');

// 4. Break Shot 4-Ball Cushion Rule (WPA 9.2)
console.log('[TEST 4] Testing Break Shot 4-Ball Cushion Rule (WPA 9.2)...');
rules.reset();
// 4a. Dry break with only 2 object balls hitting rails -> Illegal Break Foul!
const illegalDryBreak = rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: true,
  isPushOutCall: false,
  firstContactBallId: 1,
  railHitAfterContact: true,
  objectBallsHitRailsCount: 2, // Less than 4!
  ballsPocketed: [],
  cueScratch: false,
}, 1);
assert(!illegalDryBreak.isLegal, 'Dry break with < 4 balls to rails must be foul');
assert(illegalDryBreak.foulType === NineBallFoulType.IllegalBreak, 'Foul type must be IllegalBreak');
assert(illegalDryBreak.ballInHand === true, 'Opponent receives Ball-in-Hand');
assert(illegalDryBreak.nextShooter === NineBallPlayer.Player2, 'Turn passes to Player 2');
console.log('  -> PASS: Dry break with 2 balls hitting rail ruled IllegalBreak with BIH.');

// 4b. Dry break with 4 distinct object balls hitting rails -> Legal Break!
rules.reset();
const legalDryBreak = rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: true,
  isPushOutCall: false,
  firstContactBallId: 1,
  railHitAfterContact: true,
  objectBallsHitRailsCount: 4,
  ballsPocketed: [],
  cueScratch: false,
}, 1);
assert(legalDryBreak.isLegal, 'Dry break with 4 balls to rails must be legal');
assert(legalDryBreak.ballInHand === false, 'No ball-in-hand');
assert(rules.getPushOutAvailable() === true, 'Push-out must be available on legal break');
console.log('  -> PASS: Dry break with 4 balls hitting rails is legal; Push-out option unlocked.\n');

// 5. Push-Out Mechanics (WPA 9.4)
console.log('[TEST 5] Testing Push-Out Mechanics (WPA 9.4)...');
rules.reset();
// Breaker makes legal dry break
rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: true,
  isPushOutCall: false,
  firstContactBallId: 1,
  railHitAfterContact: true,
  objectBallsHitRailsCount: 4,
  ballsPocketed: [],
  cueScratch: false,
}, 1);
assert(rules.getPushOutAvailable(), 'Push-out available after break');

// Incoming Player 2 calls Push-Out
const pushOutShot = rules.evaluateShot({
  shooter: NineBallPlayer.Player2,
  isBreakShot: false,
  isPushOutCall: true,
  firstContactBallId: 7, // Hitting 7 is allowed during push-out!
  railHitAfterContact: false, // Cushion rule waived!
  ballsPocketed: [],
  cueScratch: false,
}, 1);
assert(pushOutShot.isLegal, 'Push-out shot must be legal without hitting lowest ball or rail');
assert(pushOutShot.gameStatus === NineBallGameStatus.PushOutPendingResponse, 'Game status must be PushOutPendingResponse');

// Opponent (Player 1) passes the shot back to Player 2
rules.resolvePushOutResponse(false);
assert(rules.getCurrentPlayer() === NineBallPlayer.Player2, 'When passed back, shooter remains Player 2');
assert(rules.getGameStatus() === NineBallGameStatus.RackInProgress, 'Rack resumes in progress');
assert(!rules.getPushOutAvailable(), 'Push-out no longer available');
console.log('  -> PASS: Push-out waives contact/rail rules, and pass-back correctly restores shooter.\n');

// 6. Golden Break (9-Ball on break)
console.log('[TEST 6] Testing Golden Break...');
rules.reset();
const goldenBreak = rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: true,
  isPushOutCall: false,
  firstContactBallId: 1,
  railHitAfterContact: true,
  objectBallsHitRailsCount: 4,
  ballsPocketed: [9],
  cueScratch: false,
}, 1);
assert(goldenBreak.isLegal, 'Golden break must be legal');
assert(goldenBreak.gameStatus === NineBallGameStatus.Player1Win, 'Player 1 wins instantly on Golden Break');
console.log('  -> PASS: Golden break declared Player 1 Winner!\n');

// 7. 9-Ball Respotting on Scratch / Foul & Overflow Prevention
console.log('[TEST 7] Testing 9-Ball Respotting & Cushion Overflow Prevention...');
engine.resetRack('9ball');
// Sink 9-ball
const b9Ball = engine.balls.find(b => b.id === 9)!;
b9Ball.isSunk = true;

// Foul with 9 pocketed
rules.reset();
const scratchRes = rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: false,
  isPushOutCall: false,
  firstContactBallId: 1,
  railHitAfterContact: true,
  ballsPocketed: [9, 3],
  cueScratch: true,
}, 1);
assert(!scratchRes.isLegal, 'Scratch is foul');
assert(scratchRes.nineBallRespotted === true, '9-ball must be respotted');

// Respot 9-ball
engine.respotNineBall();
assert(!b9Ball.isSunk, '9-ball is no longer sunk');
assert(b9Ball.x <= engine.TABLE_WIDTH - engine.BALL_RADIUS, '9-ball must never overflow into foot rail');
assert(b9Ball.x >= engine.BALL_RADIUS, '9-ball must be on table');
console.log('  -> PASS: 9-ball successfully respotted without cushion overflow.\n');

// 8. Three-Consecutive-Foul Rule
console.log('[TEST 8] Testing Three Consecutive Foul Rack Forfeit...');
rules.reset();
// Foul 1
const f1 = rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: false,
  isPushOutCall: false,
  firstContactBallId: -1,
  railHitAfterContact: false,
  ballsPocketed: [],
  cueScratch: false,
}, 1);
assert(f1.player1FoulCount === 1, 'Foul 1 recorded');

// Player 2 legal shot
rules.evaluateShot({
  shooter: NineBallPlayer.Player2,
  isBreakShot: false,
  isPushOutCall: false,
  firstContactBallId: 1,
  railHitAfterContact: true,
  ballsPocketed: [],
  cueScratch: false,
}, 1);

// Foul 2 by Player 1
const f2 = rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: false,
  isPushOutCall: false,
  firstContactBallId: -1,
  railHitAfterContact: false,
  ballsPocketed: [],
  cueScratch: false,
}, 1);
assert(f2.player1FoulCount === 2, 'Foul 2 recorded');
assert(f2.coachPuddinWarningTriggered === true, 'Warning must trigger on 2nd foul');

// Player 2 legal shot
rules.evaluateShot({
  shooter: NineBallPlayer.Player2,
  isBreakShot: false,
  isPushOutCall: false,
  firstContactBallId: 1,
  railHitAfterContact: true,
  ballsPocketed: [],
  cueScratch: false,
}, 1);

// Foul 3 by Player 1
const f3 = rules.evaluateShot({
  shooter: NineBallPlayer.Player1,
  isBreakShot: false,
  isPushOutCall: false,
  firstContactBallId: -1,
  railHitAfterContact: false,
  ballsPocketed: [],
  cueScratch: false,
}, 1);
assert(f3.foulType === NineBallFoulType.ThreeConsecutiveFouls, '3rd foul triggers ThreeConsecutiveFouls');
assert(f3.gameStatus === NineBallGameStatus.Player2Win, 'Player 2 declared winner on 3 fouls');
console.log('  -> PASS: 3 consecutive fouls forfeited the rack to Player 2!\n');

console.log('======================================================');
console.log('  ALL WPA 9-BALL RULES & ARBITRATION TESTS PASSED (8/8)');
console.log('======================================================');
