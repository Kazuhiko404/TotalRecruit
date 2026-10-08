// Mock view-models. Each object is exactly what src/readout_model.cpp returns for the facts in
// the comment above it (the C++ tests pin that mapping); the renderer in readout.js only draws.
// Text values are the messages.json "readout" block, in the mod's player language (no accents,
// like the rest of messages.json).

var STATES = [
  {
    id: 'cage',
    title: 'Caged bandit',
    note: 'Cautiverio. Stress comes from an empty stomach, untreated wounds and the beatings (Golpes: what Golpear and Noquear left in your cage, fading over 24 game hours); the chance is JoinChance(...) and the price is FinalPriceInCats(...) -- the same numbers Evaluate uses. Time in the cage alone no longer decides anything (the easy wait was removed in v1-presets-close P4): only stress, personality, level and the price do.',
    // facts: route Cage, level 14 vs player 20, hunger raw 1.2/3, worstDamage .4, rested .7,
    // beaten .12 (beating stress, decayed), defeats 1/2, price 9570 (purse 12000)
    vm: {
      name: 'Kareem el Tuerto', route: 'cautiverio', routeLabel: 'Cautiverio',
      showStress: true, stressPct: 65, stress: { hunger: 30, wounds: 14, tired: 9, beaten: 12 },
      showChance: true, chancePct: 58, ready: false,
      showDefeats: true, defeats: 1, defeatsNeeded: 2,
      showFood: false,
      held: 'Encerrado: 2 h 15 min',
      why:  { text: 'Precio 9.570 cats (tienes 12.000)', tone: '' }
    }
  },
  {
    id: 'caredFor',
    title: 'Caged prisoner you cared for',
    note: 'Cautiverio, Balanced (v1-presets-close P4). Unhurt, fed and at full health in your cage: the roll gets a cared-for bonus, drawn once per NPC per game day between <code>submissionCaredForBonusMin</code> (5) and <code>submissionCaredForBonusMax</code> (20), seeded with <code>DailySeed(key, day, kCaredForBonusSalt)</code> so clicking or reloading never redraws it. The draw of the day is +12; it is already inside the 70%. It replaced the old "sano gracias a ti" automatic join: being looked after argues for joining, it does not decide. A wounded or hungry prisoner, or an animal, gets no row.',
    // facts: route Cage, level 14 vs player 20, hunger raw 3/3 (fed), worstDamage 0, health 1,
    // rested 1, beaten 0, defeats 1/2, caredFor +12 (today's draw), price 9570 (purse 12000)
    vm: {
      name: 'Kareem el Tuerto', route: 'cautiverio', routeLabel: 'Cautiverio',
      showStress: true, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: true, chancePct: 70, ready: false, caredForPct: 12,
      showDefeats: true, defeats: 1, defeatsNeeded: 2,
      showFood: false,
      held: 'Encerrado: 2 h 15 min',
      why:  { text: 'Precio 9.570 cats (tienes 12.000)', tone: '' }
    }
  },
  {
    id: 'caredForStreak',
    title: 'Caged prisoner cared for three days in a row',
    note: 'Cautiverio, Balanced (balanced-tuning T3). The cared-for bonus grows with the care streak: game days in a row on which the prisoner was seen unhurt, fed and at full health, today not counted (<code>CareStreakDays</code>). Each day of streak adds <code>submissionCaredForStreakPerDay</code> (3) up to <code>submissionCaredForStreakMax</code> (15, a five-day streak). Today: the draw +12 plus 3 days x 3 = +21, already inside the 79%. A day seen hurt, hungry or below full health resets the streak to 0; the streak is saved with the character. No streak: the row is the plain "+12%".',
    // facts: route Cage, level 14 vs player 20, fed, unhurt, health 1, rested 1, beaten 0,
    // defeats 1/2, caredFor +21 (draw +12, streak 3 days x 3), held 3 d 4 h, price 9570
    vm: {
      name: 'Kareem el Tuerto', route: 'cautiverio', routeLabel: 'Cautiverio',
      showStress: true, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: true, chancePct: 79, ready: false, caredForPct: 21, careStreakDays: 3,
      showDefeats: true, defeats: 1, defeatsNeeded: 2,
      showFood: false,
      held: 'Encerrado: 3 d 4 h',
      why:  { text: 'Precio 9.570 cats (tienes 12.000)', tone: '' }
    }
  },
  {
    id: 'caredNotYet',
    title: 'Honourable prisoner cared for, care streak too short',
    note: 'Cautiverio, Balanced (balanced-tuning T6). The honourable and the warm/kind take no cats: when the roll fails, the currency table sends them to care. Care used to make them join on the spot (healthy + fed); now it joins only once the care streak reaches <code>submissionCareJoinStreakDays</code> (4, clamp 0..30; 0 = the old join on the first cared-for day). Below it the main line says how many more cared-for days are missing (<code>readout.line.cared.notYet</code>, "{need}" = 4 - streak). Here the streak is 2 days: 2 more to go. With a streak of 4 the line is the old "Alimentado y curado: se une". The bonus row and the chance are unchanged: the roll still runs every day.',
    // facts: route Cage, honourable, level 20 vs player 20, fed, unhurt, health 1, rested 1,
    // beaten 0, caredFor +18 (draw +12, streak 2 days x 3), held 2 d 5 h (resigned +1),
    // roll failed -> currency care, streak 2 < 4 -> Reason_CaredForNotYet, need 2
    vm: {
      name: 'Paladin Seto', route: 'cautiverio', routeLabel: 'Cautiverio',
      showStress: true, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: true, chancePct: 49, ready: false, caredForPct: 18, careStreakDays: 2, resignedPct: 1,
      showDefeats: true, defeats: 1, defeatsNeeded: 2,
      showFood: false,
      held: 'Encerrado: 2 d 5 h',
      why:  { text: 'Cuidado: le faltan 2 dias de trato', tone: '' }
    }
  },
  {
    id: 'resigned',
    title: 'Prisoner caged for a week',
    note: 'Cautiverio, Balanced (balanced-tuning T4). Time in the cage argues on its own again, slowly and capped: every full game day held (the saved hold clock, <code>total_recruit.hold_start_hour</code>) adds <code>submissionResignationPerDay</code> (1) up to <code>submissionResignationMax</code> (10), divided by the resolve like stress and defeats (<code>ResignationChance</code>). A normal prisoner held 7 days: +7, already inside the 74%. Below one full day, or with no hold clock, there is no row. It never decides on its own: it is one more term of the roll.',
    // facts: route Cage, level 14 vs player 20, temperament normal, hunger raw 1.2/3,
    // worstDamage 0, rested 1, beaten 0, defeats 1/2, held 7 d 2 h -> resignation +7, price 9570
    vm: {
      name: 'Kareem el Tuerto', route: 'cautiverio', routeLabel: 'Cautiverio',
      showStress: true, stressPct: 30, stress: { hunger: 30, wounds: 0, tired: 0, beaten: 0 },
      showChance: true, chancePct: 74, ready: false, resignedPct: 7,
      showDefeats: true, defeats: 1, defeatsNeeded: 2,
      showFood: false,
      held: 'Encerrado: 7 d 2 h',
      why:  { text: 'Precio 9.570 cats (tienes 12.000)', tone: '' }
    }
  },
  {
    id: 'yield',
    title: 'Field-yield candidate',
    note: 'Sometido (its own tag, not Rendicion): beaten enough times in the open. The card asks as a bystander with no roll key, so it cannot know whether the roll passes: instead of a "not yet" that the click may contradict, the line states the odds (<code>kLineRollOpen</code>). With the key and a failed roll the line is "Aun no cede: vencelo otra vez" in the open, and "dejalo sin comer" only for someone in your own cage.',
    // facts: route FieldYield, defeats 2/2, stress .35, chance 41, peek (no roll key), not in your cage
    vm: {
      name: 'Soldado Holy Nation', route: 'sometido', routeLabel: 'Sometido',
      showStress: true, stressPct: 35, stress: { hunger: 5, wounds: 25, tired: 5, beaten: 0 },
      showChance: true, chancePct: 41, ready: false,
      showDefeats: true, defeats: 2, defeatsNeeded: 2,
      showFood: false,
      why:  { text: '41%: puede unirse si le hablas', tone: '' },
      alt:  null
    }
  },
  {
    id: 'animal',
    title: 'Animal being tamed',
    note: 'Animal: no chance and no stress, taming is food only. The bar is the stomach, the notch is the fullness this animal needs (maturity, level, temperament). "Hambre 56% / necesita 80%" is the line the player acts on.',
    // facts: Route Cage/Free animal, hunger raw 1.69/3 -> 56%, needed .80
    vm: {
      name: 'Perro hueso', route: 'animal', routeLabel: 'Animal',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: false,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: true, foodPct: 56, foodNeededPct: 80,
      why:  { text: 'Hambre 56% / necesita 80%', tone: '' },
      alt:  null
    }
  },
  {
    id: 'animalCagedBeaten',
    title: 'Animal in your cage, beaten (Balanced)',
    note: 'priority-one D. The same Perro hueso (level 20) in your cage after Golpear left half a beating bar (Golpes 50%). In Balanced the beating stress lowers the stomach bar a caged animal needs (<code>animal_beating_rules</code>): by <code>animalCageBeatingWeight</code> (0.5) x stress / resolve, resolve = <code>animalCageBeatingResolveBase</code> (1.0) + level x <code>animalCageBeatingResolvePerLevel</code> (0.02) = 1.4 here, so 80% x (1 - 0.18) = 66%. The card adds no row: the notch simply sits lower (no animal odds to show). The first meal from your hand is still needed; a loose animal and Cheat are unchanged. config.json keys: <code>animalCageBeatingCounts</code>, <code>animalCageBeatingWeight</code>, <code>animalCageBeatingResolveBase</code>, <code>animalCageBeatingResolvePerLevel</code>.',
    // facts: Route Cage animal, inMyCage, hunger 1.69/3 -> 56%, beatingStress .5, level 20, needed .80 -> .66
    vm: {
      name: 'Perro hueso', route: 'animal', routeLabel: 'Animal',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: false,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: true, foodPct: 56, foodNeededPct: 66,
      why:  { text: 'Hambre 56% / necesita 66%', tone: '' },
      alt:  null
    }
  },
  {
    id: 'refused',
    title: 'Refused NPC',
    note: 'Libre, grudge. The one honest no: there is no roll, so there is no chance to show -- a dash, and the reason says there is no way in. Stress and defeats are hidden because nothing here reads them.',
    // facts: Route GrudgeRefusal, temperament Honorable, grudge true, not in my cage
    vm: {
      name: 'Capitana Ilse', route: 'libre', routeLabel: 'Libre',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: false,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      why:  { text: 'Rencor: no perdona fuera de la jaula', tone: 'no' },
      alt:  null
    }
  },
  {
    id: 'ally',
    title: 'Friendly NPC in the open',
    note: 'Libre, ally, Balanced: the level tier decides (<code>ally_tier_rules.h</code>). A Normal of level 34 pays a friend\'s price, no roll, so no chance bar: the line is the discounted price (<code>readout.line.ally.paidJoin</code>; short of cats it is <code>paidAsk</code> with the purse). A Good ally under 20 still rolls and shows the odds; 80+ reads "Aliado: te acompana unos dias" or the tier refusal. With <code>allyTierEnabled</code> off every ally keeps the single roll: chance bar and "Quiza te siga; si no, hazte mas fuerte".',
    // facts: Route FreeWorld, Balanced, relation +40, level 34 vs 20, Normal -> Paid 20%; cats 1840, willJoin
    vm: {
      name: 'Mercader Ostrel', route: 'libre', routeLabel: 'Libre',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: false,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      why:  { text: 'Aliado: 1.840 cats con descuento', tone: 'ok' },
      alt:  null
    }
  },
  {
    id: 'allyDuel',
    title: 'Ally 80+ you can challenge (TotalSparring)',
    note: 'An 80+ ally the popup offers RETAR A DUELO for (<code>duel_challenge_rules.h</code> <code>CanOfferDuel</code>: TotalSparring present, no lost-duel cooldown). The second line is TotalSparring\'s head-to-head record of the selected squad member against them (<code>readout.line.duel.record</code>, wins-losses; <code>recordDraws</code> adds the draws). Read only when the NPC or the selected squad member changes or TotalSparring reported a bout, never on the 2 s refresh. The same line carries what the next win does (<code>duel_join_rules.h</code> <code>DuelNextWinOdds</code>): "Duelos: 2-1, si ganas 65% se une" in Balanced (<code>readout.line.duel.recordChance</code>, with draws <code>recordDrawsChance</code> "Duelos: 0-3-1, si ganas 25%"), "Duelos: 1-2, si ganas se une" in Cheat (<code>recordSure</code> / <code>recordDrawsSure</code>). A free animal of 40+ gets the same line (state "Wild animal 40+ ...").',
    // facts: Route FreeWorld, Balanced, level 85 Honorable, Reason_AllyTemporary; duel offered, record 2-1-0; base 25, per win 20
    vm: {
      name: 'Beep el Sabio', route: 'libre', routeLabel: 'Libre',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: true,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      why:  { text: 'Aliado: te acompana unos dias', tone: 'ok' },
      alt:  { text: 'Duelos: 2-1, si ganas 65% se une' }
    }
  },
  {
    id: 'allyDuelNeedWins',
    title: 'Ally you can challenge, under the minimum duel wins',
    note: 'duel-any-source U4. With <code>duelMinWinsBalanced</code> (or <code>duelMinWinsCheat</code>) above 1, a win under the minimum never recruits: no roll, no win cooldown. While the next win is still short of it, the record line says how many wins are missing, the next one included (<code>DuelNextWinOdds</code> <code>winsNeeded</code>): "Duelos: 1-0, faltan 3 victorias" (<code>readout.line.duel.recordNeedWins</code>; with draws <code>recordDrawsNeedWins</code> "Duelos: 1-0-1, faltan 3"). One missing win means the next one decides, so the line goes back to the odds ("si ganas 45% se une"; the curve counts from the first win). The popup row SI LE GANAS says the same count meanwhile: "Ganale 3 veces mas" (U6, popup state "Duel: SI LE GANAS while wins are still missing"). After such a win the rival says "Buena pelea. Ganame otra vez" (<code>duel.notice.needMoreWins</code>) and the roller "... aun no se une. Victorias que faltan: 2." (<code>duel.needMoreWins</code>).',
    // facts: Route FreeWorld, Balanced, level 85, Reason_AllyTemporary; duelMinWinsBalanced 4, record 1-0-0 -> winsNeeded 3
    vm: {
      name: 'Beep el Sabio', route: 'libre', routeLabel: 'Libre',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: true,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      why:  { text: 'Aliado: te acompana unos dias', tone: 'ok' },
      alt:  { text: 'Duelos: 1-0, faltan 3 victorias' }
    }
  },
  {
    id: 'rescue',
    title: 'Saved life (will join)',
    note: 'Rescate with a certain outcome (no roll left): the chance row is replaced by the result. Same card, the line goes green.',
    // facts: Route Rescue / Surrendered / ... willJoin without a roll
    vm: {
      name: 'Dalia', route: 'rescate', routeLabel: 'Rescate',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: true,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      why:  { text: 'Te debe la vida: se une', tone: 'ok' },
      alt:  null
    }
  },
  {
    id: 'animalDuel',
    title: 'Wild animal 40+ you can challenge (TotalSparring)',
    note: 'animal-duel-submission D1. A free animal (no owner: the taming route\'s <code>BelongsToSomeone</code>, read outside your cage) of level <code>duelAnimalsFromLevel</code> (40) or above, with <code>duelAnimalsEnabled</code> on, TotalSparring present and no cooldown, is offered RETAR A DUELO in Balanced and in Cheat (<code>CanOfferDuel</code>, animal branch: no ally tier, no 80+ cut). The second line is the same head-to-head record line a person gets; the next win is the same Balanced curve (25, 45, 65...) or "se une" in Cheat. An owned animal, one under 40, or <code>exclude.animals</code> gets no line and no button.',
    // facts: Route FreeWorld animal, Balanced, level 52 (SafeNpcLevel), not owned, never fed -> Reason_TameNotFed;
    // duel offered (animal branch), record 0-0, base 25
    vm: {
      name: 'Perro hueso alfa', route: 'animal', routeLabel: 'Animal',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: false,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: true, foodPct: 100, foodNeededPct: 80,
      why:  { text: 'No comio de tu mano: dale comida', tone: '' },
      alt:  { text: 'Duelos: 0-0, si ganas 25% se une' }
    }
  },
  {
    id: 'animalBeaten',
    title: 'Wild animal beaten once (Balanced)',
    note: 'animal-duel-submission D3. A free animal the player\'s squad knocked out once in the open: DERROTAS 1/2 shows next to the stomach bar, because two defeats open the same field-yield roll a person gets. Still the taming line until then. No stress bar: the animal card keeps it hidden. An owned animal shows no defeats row (the cage is its only way in).',
    // facts: Route FreeWorld animal, Balanced, defeats 1/2, not owned, hunger raw 1.5/3 -> 50%, needed .75, fed once
    vm: {
      name: 'Perro hueso alfa', route: 'animal', routeLabel: 'Animal',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: false,
      showDefeats: true, defeats: 1, defeatsNeeded: 2,
      showFood: true, foodPct: 50, foodNeededPct: 75,
      why:  { text: 'Hambre 50% / necesita 75%', tone: '' },
      alt:  null
    }
  },
  {
    id: 'animalYield',
    title: 'Wild animal beaten twice: the field-yield roll (Balanced)',
    note: 'animal-duel-submission D3. Two defeats in the open (<code>submissionDefeatsToBreak</code>) send a free animal down the field-yield route like a person: JoinChance with temperament Normal, <code>submissionFieldYieldBonus</code>, no loyalty penalty, the daily seed. The card peeks with no roll key, so it states the odds; the stomach bar stays, because a failed roll falls back to food (a fed animal past the bar still joins). With the key and a failed roll the line is "Aun no se somete: vencelo o dale comida". Owned animals never get here.',
    // facts: Route FieldYield animal, Balanced, defeats 2/2, chance 38, peek (no roll key), hunger 50% / needed 75%
    vm: {
      name: 'Perro hueso alfa', route: 'animal', routeLabel: 'Animal',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: true, chancePct: 38, ready: false,
      showDefeats: true, defeats: 2, defeatsNeeded: 2,
      showFood: true, foodPct: 50, foodNeededPct: 75,
      why:  { text: '38%: puede unirse si le hablas', tone: '' },
      alt:  null
    }
  },
  {
    id: 'cheatAnimalDefeated',
    title: 'Cheat: a wild animal your squad beat joins',
    note: 'animal-duel-submission D4. In Cheat one knockout by the player\'s squad (the defeat ledger: a fight or a duel; Golpear and Noquear in your cage are beating stress, not defeats) is the gesture: talk to it and it joins (<code>Reason_CheatAnimalDefeated</code>, "Lo venciste: te sigue"). Healing it (first aid or doctoring by one of your characters, <code>healing_ledger</code>) reads "Lo curaste: te sigue"; a meal still reads "Comio de tu mano: te sigue". An owned animal keeps the cage. Cheat reads no core facts, so no bars.',
    // facts: Preset_Cheat, cheatNeedsCage, animal, not owned, not fed, not healed, defeats 1 -> willJoin
    vm: {
      name: 'Garra', route: 'animal', routeLabel: 'Animal',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: true,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      why:  { text: 'Lo venciste: te sigue', tone: 'ok' },
      alt:  null
    }
  },
  {
    id: 'cheatHeldAnyCage',
    title: 'Cheat: a prisoner held in another faction\'s cage',
    note: 'prisoner-hold-time. The line under the header is how long the hold clock (<code>hold_tracking.cpp PeekCountedHoursHeld</code>) has had them, on the game clock, rounded down (<code>hold_duration_rules.h</code>: "{m} min", "{h} h {m} min", "{d} d {h} h"; a zero part is left out). The card only peeks: it never starts a clock. Shown in every preset for the player\'s cage; in Cheat with <code>cheatAnyCage</code> on, any cage, shackles or kidnap counts too. No clock yet (the sweep has not seen them caged) means no line. Cheat reads no core facts, so no bars.',
    // facts: Preset_Cheat, cheatNeedsCage, cheatAnyCage, in another faction's cage, hoursHeld 1.5 -> willJoin
    vm: {
      name: 'Esclavo de Shek', route: 'cautiverio', routeLabel: 'Cautiverio',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0, beaten: 0 },
      showChance: false, chancePct: 0, ready: true,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      held: 'Encerrado: 1 h 30 min',
      why:  { text: 'Quiere hablar: se une', tone: 'ok' },
      alt:  null
    }
  }
];

// Where the card goes and when (src/readout.cpp, src/readout_ui.cpp, src/screen_projection.cpp,
// src/offer_popup.cpp). By default the card follows the selected NPC: worked out when the selection
// changes and again every selectionRefreshMs. The Hablar click pins it: it stays over the head
// while the offer window that click opened is up, and only steps sideways beside that window when
// it would cover it. When the window closes (or after pinTimeoutMs when none opened) it stays only
// if the NPC is still selected.
var PLACEMENT = {
  arrowGap: 8,          // kArrowGap (CardMetrics.gap)
  screenMargin: 4,      // kScreenMargin (CardMetrics.margin)
  belowGap: 32,         // kArrowGap * 4 (CardMetrics.belowGap): under the head when no room above
  headHeightUnits: 20,  // kHeadHeightUnits (readout_target.cpp): world units over the feet
  anchorLift: 6,        // kAnchorLiftPx (readout_target.cpp): pixels over the projected head
  pinTimeoutMs: 8000,   // kPinTimeoutMs
  selectionRefreshMs: 2000 // kSelectionRefreshMs
};

// The "wants to talk" notice over the head (src/head_notice.cpp, src/head_notice_rules.h). Raised by
// hail.cpp (a ready captive calls out) and submission_director.cpp (a beaten NPC walks up); gone
// after its lifetime, when that NPC is selected, when they die or join, or at a load.
// temp_member.cpp raises squad notices (RaiseSquadHeadNotice) over a temporary member: kept while
// they are in the squad, never dismissed by selecting them.
// Lifetime by kind (HeadNoticeKind, head_notice_rules.h): a notice that asks the player to act
// (wantsToTalk, approaching, helperOffer) stays headNoticeSeconds; a one-line informational notice
// (temporary member's and helper's lines) stays headNoticeInfoSeconds.
var HEAD_NOTICE = {
  seconds: 90,           // headNoticeSeconds default (config.h), clamp 10..3600: prompts
  infoSeconds: 25,       // headNoticeInfoSeconds default (config.h), clamp 5..3600: informational lines
  cap: 8,                // kHeadNoticeCap: one per NPC, the oldest goes when full
  wantsToTalk: 'Quiere hablar contigo',          // messages.json headNotice.wantsToTalk
  approaching: 'Se acerca para hablar contigo'   // messages.json headNotice.approaching
};

// What a temporary member (80+ ally, src/temp_member.h) says over their head: messages.json
// tempMember.notice.*. {days} is filled in.
var TEMP_MEMBER = {
  join: 'Te acompanare 6 dias',                       // on joining
  daily: 'Me quedan 4 dias',                          // once per game day after the join day (tempMemberDailyNotice)
  dailyOne: 'Me queda 1 dia',                         // the same with one day left (tempMember.notice.dailyOne)
  hungry: 'Tengo hambre... no me trates asi',         // a hunger strike
  mistreated: 'No me trates asi',                     // attacked / knocked out by the squad
  warning: 'Si sigues asi me voy',                    // the strike before the last
  expiring: 'Me ire pronto',                          // tempMemberExpiryWarningHours before the end
  farewell: 'Me voy. Fue un honor',                   // time up
  farewellMistreated: 'Me largo',                     // tempMemberMaxStrikes reached
  card: 'Temporal: le quedan 4 dias'                  // readout.line.temp.daysLeft on their card
};

// What a free helper (src/helper.h) says over their head: messages.json headNotice.helperOffer
// and helper.notice.*. Helpers stay in their own faction, so these are ordinary notices.
var HELPER = {
  offer: 'Quiere ofrecerte ayuda',      // walking up to offer (headNotice.helperOffer)
  start: 'Te cubro las espaldas',       // accepted: the escort begins
  expiring: 'Me ire pronto',            // helperExpiryWarningHours before the end
  farewell: 'Hasta aqui llego. Suerte', // time up
  betrayed: 'No vuelvo a ayudarte',     // attacked or knocked out by the squad
  rearmed: 'Sigo contigo',              // found again after a load
  card: 'Ayudante: le quedan 3 dias'    // readout.line.helper.daysLeft on their card
};

// The proactive offer (src/offer_requests.h, src/submission_director.cpp): after a gesture of
// the player (a won duel, a heal by the squad, the defeats) the NPC or animal walks up and opens
// the submission window by itself. A person speaks and carries the head notice; an animal never
// does: the roller narrates it (src/offer_narration_rules.h). A bad roll calms it in place.
var OFFER_EVENT = {
  personApproach: 'Bandido Hambriento: Espera... no quiero pelear mas.',      // submissionEvent.approach (over its head, spoken)
  personNotice: 'Bandido Hambriento se acerca para hablar contigo.',          // submissionEvent.approachNotice (roller)
  animalNotice: 'Perro hueso alfa se acerca con la cabeza gacha.',            // submissionEvent.animal.approachNotice (roller)
  personCalm: 'Bandido Hambriento no quiere unirse, pero deja de pelear por 20 s.',  // submissionEvent.calmNotice
  animalCalm: 'Perro hueso alfa se calma y deja de atacarte por 20 s.',        // submissionEvent.animal.calmNotice
  personIgnored: 'Bandido Hambriento: lo ignoraste y se aleja.',              // submissionEvent.ignoredNotice
  animalIgnored: 'Perro hueso alfa se aleja trotando.',                       // submissionEvent.animal.ignoredNotice
  personJoined: 'Bandido Hambriento: se une a ti.',                           // submissionEvent.joinedNotice
  animalJoined: 'Perro hueso alfa te sigue: ahora es parte de tu grupo.',     // submissionEvent.animal.joinedNotice
  failCalmSeconds: 20                                                         // offerFailCalmSeconds default (config.h)
};
