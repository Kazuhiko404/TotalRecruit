// Mock OfferViews for the recruit popup. Each object is exactly what BuildOfferView /
// BuildMercyOfferView (src/offer_view.cpp) return for the facts in the comment above it;
// tests/rules_tests/offer_view_tests.cpp pins that mapping. popup.js only draws.
// Texts are the messages.json "popup" block (no accents, like the rest of the file).

var POPUP_LAYOUT = {
  clientW: 540,      // kClientWidth
  margin: 14,        // kMargin
  portrait: 110,     // kPortraitSide (framed by 1px: 112 on screen)
  colGap: 14,        // kColumnGap: portrait frame -> right column
  nameH: 22,         // kNameH
  tagW: 84,          // kTagW
  tagH: 18,          // kTagH
  ruleGap: 3,        // kRuleGap: name row -> brass rule
  bodyGap: 6,        // kBodyGap: rule -> body line
  infoGap: 8,        // kInfoGap: body -> info box
  infoRowH: 22,      // kInfoRowH
  infoPad: 4,        // kInfoPad (top and bottom inside the box)
  infoLabelW: 120,   // kInfoLabelW
  sectionGap: 12,    // kSectionGap: top block -> reason -> buttons
  reasonBar: 3,      // kReasonBarW: the red bar on the reason plate's left
  reasonPadX: 10,    // kReasonPadX
  reasonPadY: 7,     // kReasonPadY
  buttonH: 32,       // kButtonHeight
  buttonGap: 8,      // kButtonGap
  aloneW: 150,       // kAloneButtonW: Cancel / Close when it is the only button
  buttonsPerRow: 3,  // kButtonsPerRow: more and Cancel wraps alone to a second row, right
  // The haggle section (cats prices only), between the top block and the reason.
  haggleRowH: 24,    // kHaggleRowH: label | slider | value (the 22 px thumb fits)
  haggleLabelW: 84,  // kHaggleLabelW: "OFERTA"
  haggleValueW: 110, // kHaggleValueW: "7.650 cats", right-aligned
  haggleGap: 8,      // kHaggleGap: slider -> value
  haggleScaleH: 16,  // kHaggleScaleH: min / max under the slider ends
  haggleLineH: 20    // kHaggleLineH: the odds, the short line or the offended line
};

var POPUP_STATES = [
  {
    id: 'cats',
    title: 'Joins for cats (haggle off)',
    note: 'Cautiverio, the roll passed and the price is due: <code>Reason_CatsJoin</code>. Price in amber, the purse it is checked against, the odds the engine rolled. Money only moves on ACEPTAR (CompleteOffer re-checks and charges). This is the window with <code>haggleEnabled</code> off: no slider; with it on, the same NPC opens as state 2.',
    // facts: verdict willJoin, Currency_Cats, cats 9570, rolled 58%; purse read 12000
    vm: {
      title: 'Reclutamiento', name: 'Kareem el Tuerto', route: 'cautiverio', routeTag: 'Cautiverio',
      body: 'Kareem el Tuerto se unira a tu grupo por 9.570 cats.',
      showPrice: true, priceText: '9.570 cats', priceFree: false,
      showPurse: true, purseText: '12.000 cats', affordable: true,
      showChance: true, chanceText: '58%',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', cancel: 'CANCELAR'
    }
  },
  {
    id: 'haggle',
    title: 'Haggle: offer below the ask',
    note: 'The same Kareem with <code>haggleEnabled</code> on. <b>Drag the slider</b>: it runs from half the ask to the ask in steps of 10, starting at the ask. Below the ask the amber button reads OFRECER and the line gives the live odds (green from 70%, amber 30-69, red under 30); at the ask it is ACEPTAR again, 100%, exactly as before. OFRECER is one attempt on the day\'s seed: accepted, he joins for the offer; refused, he is offended (state 4). Sample: speaker best stat 46 vs level 30 (leverage +8), Normal temperament.',
    // facts: verdict willJoin, Currency_Cats, cats 9570, rolled 58%; purse 12000;
    // haggle: leverage +8, Temperament_Normal, not offended today; slider at 7650
    hg: { ask: 9570, purse: 12000, leverage: 8, temperament: 'normal', locked: false, offer: 7650 },
    vm: {
      title: 'Reclutamiento', name: 'Kareem el Tuerto', route: 'cautiverio', routeTag: 'Cautiverio',
      body: 'Kareem el Tuerto se unira a tu grupo por 9.570 cats.',
      showPrice: true, priceText: '9.570 cats', priceFree: false,
      showPurse: true, purseText: '12.000 cats', affordable: true,
      showChance: true, chanceText: '58%',
      reason: '', reasonRed: false,
      accept: 'OFRECER', extra: '', cancel: 'CANCELAR'
    }
  },
  {
    id: 'haggleShort',
    title: 'Haggle: offer the purse cannot meet',
    note: 'A mercenary asks 9.570 and the purse holds 6.000 (<code>Reason_CatsAsk</code>). With the slider the purse is checked against the <b>offer</b>: at the ask TIENES is red, the line says how much is missing for that offer and OFRECER is hidden. Drag below 6.000 and OFRECER appears: the price he quoted is out of reach, a lower one may not be. Brave (-10), weak speaker (leverage -6).',
    // facts: verdict refused, Reason_CatsAsk, cats 9570, purse 6000; leverage -6, Brave
    hg: { ask: 9570, purse: 6000, leverage: -6, temperament: 'brave', locked: false, offer: 0 },
    vm: {
      title: 'Reclutamiento', name: 'Mercenario de Gesha', route: 'libre', routeTag: 'Libre',
      body: 'Mercenario de Gesha se unira a tu grupo por 9.570 cats.',
      showPrice: true, priceText: '9.570 cats', priceFree: false,
      showPurse: true, purseText: '6.000 cats', affordable: false,
      showChance: false, chanceText: '',
      reason: '', reasonRed: false,
      accept: '', extra: '', cancel: 'CANCELAR'
    }
  },
  {
    id: 'offended',
    title: 'Haggle refused: offended today',
    note: 'Kareem refused an offer: <code>SessionHaggleLedger</code> recorded the offence for today, the popup reopened on the new verdict. The ask is up by <code>haggleOffenceMarkup</code> (9.570 &times; 1.2 = 11.490), no slider, and the red line says why. ACEPTAR at the raised price still works. The next in-game day both reset.',
    // facts: verdict willJoin, cats 11490 (OffendedAsk), purse 12000; haggle locked today
    hg: { ask: 11490, purse: 12000, leverage: 8, temperament: 'normal', locked: true, offer: 0 },
    vm: {
      title: 'Reclutamiento', name: 'Kareem el Tuerto', route: 'cautiverio', routeTag: 'Cautiverio',
      body: 'Kareem el Tuerto se unira a tu grupo por 11.490 cats.',
      showPrice: true, priceText: '11.490 cats', priceFree: false,
      showPurse: true, purseText: '12.000 cats', affordable: true,
      showChance: true, chanceText: '58%',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', cancel: 'CANCELAR'
    }
  },
  {
    id: 'short',
    title: "Can't afford (haggle off)",
    note: 'Libre, a mercenary who wants 9.570 and the purse holds 3.200: <code>Reason_CatsAsk</code>. Same price block, the purse in red, the red line says by how much, and ACEPTAR is hidden: there is nothing to accept. Only CANCELAR. With haggling on, the shortfall moves into the slider\'s line instead (state 3); this plate is also what an offended NPC the purse cannot meet shows.',
    // facts: verdict refused, Reason_CatsAsk, Currency_Cats, cats 9570, purse 3200
    vm: {
      title: 'Reclutamiento', name: 'Mercenario de Gesha', route: 'libre', routeTag: 'Libre',
      body: 'Mercenario de Gesha se unira a tu grupo por 9.570 cats.',
      showPrice: true, priceText: '9.570 cats', priceFree: false,
      showPurse: true, purseText: '3.200 cats', affordable: false,
      showChance: false, chanceText: '',
      reason: 'No te alcanzan los cats (te faltan 6.370)', reasonRed: true,
      accept: '', extra: '', cancel: 'CANCELAR'
    }
  },
  {
    id: 'loyal',
    title: 'Loyalty / honour refusal',
    note: 'Libre, a Holy Nation soldier: <code>Reason_MilitaryLoyalty</code>. Nothing to pay and nothing to do here and now, so no info box: the reason in red and CERRAR alone. The same for honour, the grudge and the pestering lockout.',
    // facts: verdict refused, Reason_MilitaryLoyalty, no remedy
    vm: {
      title: 'Reclutamiento', name: 'Paladin de Okran', route: 'libre', routeTag: 'Libre',
      body: 'Paladin de Okran no se une todavia.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      reason: 'Sirve con lealtad a su faccion: gana reputacion, o encierralo y sometelo', reasonRed: true,
      accept: '', extra: '', cancel: 'CERRAR'
    }
  },
  {
    id: 'remedy',
    title: 'Refusal with a remedy',
    note: 'Hungry and you carry food, but not in your cage (or beating switched off): <code>Reason_Starving</code> with the feed remedy. The reason in red and the button that acts on it; DARLE COMIDA feeds and reopens the popup with the new verdict (its Cancel then reads AHORA NO). The beating is not a remedy any more: in your cage it is GOLPEAR and NOQUEAR (the next two states).',
    // facts: verdict refused, Currency_Care, wantsFood, player has food; cage commands not allowed
    vm: {
      title: 'Reclutamiento', name: 'Hambriento Fremp', route: 'cautiverio', routeTag: 'Cautiverio',
      body: 'Hambriento Fremp no se une todavia.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      reason: 'Hambriento: dale de comer', reasonRed: true,
      accept: '', extra: 'DARLE COMIDA', cancel: 'CERRAR'
    }
  },
  {
    id: 'cagedDefiant',
    title: 'Caged and defiant: GOLPEAR / NOQUEAR',
    note: 'The user\'s Holy Warden (2026-10-03): Cautiverio, 87%, still defiant (<code>Reason_Defiant</code>), awake in your cage with <code>allowBeating</code> on. Any refusal from a prisoner the context menu would offer Golpear and Noquear for gets them here too: one predicate, <code>CageCommandsAllowed</code>. Same orders, same actor (the selected character; nobody selected says "Selecciona a alguien de tu grupo para hacerlo"), NOQUEAR with the game\'s odds. The window closes: the order is the answer. A yes never carries them (ACEPTAR / CANCELAR only), nor does the haggle slider. With beating off the buttons go and the reason reads "Sigue desafiante: dejalo con hambre".',
    // facts: verdict refused, viaCaptivity, Reason_Defiant, rolled 87%; cage {inPlayerCage, awake, allowBeating}; knockout odds 0.72
    vm: {
      title: 'Reclutamiento', name: 'Holy Warden', route: 'cautiverio', routeTag: 'Cautiverio',
      body: 'Holy Warden no se une todavia.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: true, chanceText: '87%',
      reason: 'Sigue desafiante: dejalo con hambre o hierelo', reasonRed: true,
      accept: '', extra: '', beat: 'GOLPEAR', knockout: 'NOQUEAR 72%', cancel: 'CERRAR'
    }
  },
  {
    id: 'cagedResting',
    title: 'Caged and defiant, GOLPEAR resting',
    note: 'The same Holy Warden two game hours after a blow, with <code>submissionBeatingCooldownHours</code> = 6 (R2.3: an interval between beatings). GOLPEAR stays in the row and carries the hours left, rounded up (<code>BeatLabel</code>, <code>beatHoursLeft</code> from <code>BeatCooldownLedger</code>, session-only like the defeat episodes). A click closes the window and only says "Podrás volver a golpearlo en 4 h" (<code>cage.beatResting</code>); the click re-reads the facts, so a window left open while he was freed or knocked out says "Ya no puedes hacerle eso" (<code>cage.notAllowed</code>). NOQUEAR never rests.',
    // facts: as cagedDefiant; cage.beatHoursLeft 3.2
    vm: {
      title: 'Reclutamiento', name: 'Holy Warden', route: 'cautiverio', routeTag: 'Cautiverio',
      body: 'Holy Warden no se une todavia.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: true, chanceText: '87%',
      reason: 'Sigue desafiante: dejalo con hambre o hierelo', reasonRed: true,
      accept: '', extra: '', beat: 'GOLPEAR (4 h)', knockout: 'NOQUEAR 72%', cancel: 'CERRAR'
    }
  },
  {
    id: 'cagedHungry',
    title: 'Caged and hungry: four buttons, two rows',
    note: 'The same prisoner starving, and you carry food: DARLE COMIDA plus the cage commands. Four buttons do not fit one row (DARLE COMIDA would not hold), so the actions share the first row and CERRAR wraps alone to the right of a second one (<code>kButtonsPerRow</code> = 3).',
    // facts: verdict refused, Reason_Starving, wantsFood, player has food; cage commands allowed; no odds read
    vm: {
      title: 'Reclutamiento', name: 'Holy Warden', route: 'cautiverio', routeTag: 'Cautiverio',
      body: 'Holy Warden no se une todavia.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: true, chanceText: '87%',
      reason: 'Hambriento: dale de comer', reasonRed: true,
      accept: '', extra: 'DARLE COMIDA', beat: 'GOLPEAR', knockout: 'NOQUEAR 72%', cancel: 'CERRAR'
    }
  },
  {
    id: 'mercy',
    title: 'Mercy plea',
    note: 'Piedad: a crippled enemy begs (mercy.cpp OfferMercy). The plea is the body; three answers: take them, let them go (IGNORAR, only when their temperament lets the player buy their life) or refuse and fight on.',
    // facts: mercy plea, CanBuyTheirLife true
    vm: {
      title: 'Reclutamiento', name: 'Bandido Hambriento', route: 'piedad', routeTag: 'Piedad',
      body: 'Bandido Hambriento: Piedad. Ya no puedo pelear. Hare lo que me pidas.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: 'IGNORAR', cancel: 'RECHAZAR'
    }
  },
  {
    id: 'mercyCaged',
    title: 'Mercy plea from someone who cannot leave',
    note: 'The same plea from someone in a cage, shackled or carried (<code>SafeInPrison</code>): there is nowhere to let them go, so IGNORAR is never offered, whatever their temperament. Take them or leave them where they are. The submission event follows the same rule: no PERDONAR for anyone who cannot leave (the director already skips the caged; this covers one picked up on the way).',
    // facts: mercy plea, CanBuyTheirLife true, inCage true
    vm: {
      title: 'Reclutamiento', name: 'Bandido Hambriento', route: 'piedad', routeTag: 'Piedad',
      body: 'Bandido Hambriento: Piedad. Ya no puedo pelear. Hare lo que me pidas.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', cancel: 'RECHAZAR'
    }
  },
  {
    id: 'submission',
    title: 'Submitted NPC asks to join',
    note: 'Sometido, the event (submission_director.cpp): a beaten NPC that yields walks up to the player, calm, and kneels. Free; three answers: ACEPTAR (joins), PERDONAR (runs off and stays calm for <code>submissionForgiveSeconds</code>, you can still talk to it), RECHAZAR (defeats wiped, no recruiting for <code>submissionRejectCooldownHours</code>, hostile again). The X is no answer: calm in place for the same window.',
    // facts: BuildSubmissionOfferView("Bandido Hambriento")
    vm: {
      title: 'Reclutamiento', name: 'Bandido Hambriento', route: 'sometido', routeTag: 'Sometido',
      body: 'Bandido Hambriento se arrodilla y te pide unirse a tu grupo.',
      showPrice: true, priceText: 'Gratis', priceFree: true,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: 'PERDONAR', cancel: 'RECHAZAR'
    }
  },
  {
    id: 'helperOffer',
    title: 'Free helper offers to escort you',
    note: 'Ayuda, the event (helper.cpp): an allied Good or Neutral NPC more than <code>helperMinLevelLead</code> (10) levels over the leader walked up (the submission walk-up, <code>approach_driver.h</code>) after winning its daily roll (<code>helperOfferChancePerDay</code> 0.15). Free; two answers: ACEPTAR (it escorts and fights for you for the days shown, in its own faction: no orders) and RECHAZAR (no offer again for <code>helperRejectCooldownDays</code>). The X is no answer: it walks off and does not ask again that day. No chance row: the roll was made before it set off.',
    // facts: BuildHelperOfferView("Beep el Sabio", 4)
    vm: {
      title: 'Reclutamiento', name: 'Beep el Sabio', route: 'libre', routeTag: 'Ayuda',
      body: 'Beep el Sabio te ofrece acompanarte 4 dias, gratis. No podras darle ordenes.',
      showPrice: true, priceText: 'Gratis', priceFree: true,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', cancel: 'RECHAZAR'
    }
  },
  {
    id: 'rescue',
    title: 'Rescue',
    note: 'Rescate: you saved them and they owe you (<code>Reason_RescueJoins</code>). Free: PRECIO says Gratis in green so the player knows no money moves.',
    // facts: verdict willJoin, viaRescue, Currency_None
    vm: {
      title: 'Reclutamiento', name: 'Esclavo Fugado', route: 'rescate', routeTag: 'Rescate',
      body: 'Esclavo Fugado te debe la vida y desea unirse a tu grupo.',
      showPrice: true, priceText: 'Gratis', priceFree: true,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', cancel: 'CANCELAR'
    }
  },
  {
    id: 'allyPaid',
    title: 'Ally pays: friendship discount (Balanced)',
    note: 'Balanced, an allied Normal of level 34 (relation 40): the 20-49 tier pays, friendship takes up to 20% off (<code>ally_tier_rules.h</code>). The discount is already in the price (<code>factionMultiplier</code> = 0.86 for this ally, <code>character_facts.cpp</code>), so the slider, the purse check and the offended markup all work on it. No roll: paying is the gate. The body names the tier; PRECIO carries the tag. Haggle off here; on, it opens with the slider like state 2.',
    // facts: verdict willJoin, Currency_Cats, Reason_CatsJoin, cats 1840, allyTiered, allyLevel 34, allyDiscountPercent 14; purse 5000
    vm: {
      title: 'Reclutamiento', name: 'Gor el Cazador', route: 'libre', routeTag: 'Libre',
      body: 'Aliado (nivel 34): Gor el Cazador te cobra 1.840 cats, con 14% de descuento por amistad.',
      showPrice: true, priceText: '1.840 cats', priceTag: '-14%', priceFree: false,
      showPurse: true, purseText: '5.000 cats', affordable: true,
      showChance: false, chanceText: '',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', cancel: 'CANCELAR'
    }
  },
  {
    id: 'allyTemporary',
    title: 'Ally comes for a few days (80+)',
    note: 'Balanced, an allied Honorable of level 85 and the leader is 80 (within <code>allyTierCloseLevelGap</code> 15): Good and Neutral allies of 80+ join as temporary members (<code>Reason_AllyTemporary</code>). Free, no roll; the days are drawn once per day from the NPC\'s seed in <code>allyTierTempMinDays..MaxDays</code> (3..10). ACEPTAR recruits and hands the days to <code>temp_member.h</code>.',
    // facts: verdict willJoin, Currency_None, Reason_AllyTemporary, allyTiered, allyLevel 85, temporary, temporaryDays 6
    vm: {
      title: 'Reclutamiento', name: 'Beep el Sabio', route: 'libre', routeTag: 'Libre',
      body: 'Aliado (nivel 85): Beep el Sabio te acompanara 6 dias como miembro temporal.',
      showPrice: true, priceText: 'Gratis', priceFree: true,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', cancel: 'CANCELAR'
    }
  },
  {
    id: 'allyDuel',
    title: 'Ally 80+ can be challenged to a duel (TotalSparring)',
    note: 'The same Beep with TotalSparring installed and no lost-duel cooldown running (<code>duel_challenge_rules.h</code> <code>CanOfferDuel</code>): the tier\'s own answer stays (ACEPTAR as a temporary member) and <b>RETAR A DUELO</b> joins the row, a sumi plate with an amber border like the other extra answers. DUELOS is TotalSparring\'s head-to-head record of the selected squad member against Beep (wins-losses, then draws when there are any; read once as the window opens, <code>TS_ApiV1::GetDuelStats</code>). RETAR A DUELO closes this window and opens TotalSparring\'s own Dueling window with the selected fighter; nobody selected reads "Selecciona a alguien de tu grupo para hacerlo". SI LE GANAS is what the next win does (<code>duel_join_rules.h</code> <code>DuelNextWinOdds</code>): in Balanced a win is a join chance, <code>min(100, duelJoinBaseChance + duelJoinChancePerWin * (wins - 1))</code> with the next win counted, so 2 wins so far read 25 + 20 * 2 = 65%. The row is drawn only when the record was read. A win that rolls the join recruits for good ("Me ganaste. Me uno a ti"); a win that does not says "Buena pelea. Retame otra vez" and hides the button for a seeded 1-3 days (level 80-99) or 1-5 days (100+); a loss or a draw says "Vuelve cuando seas mas fuerte" and hides the button for <code>duelChallengeCooldownHours</code>. Without TotalSparring the window is the state above.',
    // facts: verdict willJoin, Reason_AllyTemporary, allyTiered, allyLevel 85, temporaryDays 6; duel offered, record 2-1-0; Balanced, base 25, per win 20
    vm: {
      title: 'Reclutamiento', name: 'Beep el Sabio', route: 'libre', routeTag: 'Libre',
      body: 'Aliado (nivel 85): Beep el Sabio te acompanara 6 dias como miembro temporal.',
      showPrice: true, priceText: 'Gratis', priceFree: true,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      showDuelRecord: true, duelRecordText: '2-1',
      showDuelJoin: true, duelJoinText: '65% de que se una',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', duel: 'RETAR A DUELO', cancel: 'CANCELAR'
    }
  },
  {
    id: 'allyDuelCruel',
    title: 'Cruel ally 80+ refuses, the duel is the way in',
    note: 'A ColdCruel ally of level 90 refuses at 80+ (<code>Reason_AllyCruel</code>): the reason in red and CERRAR as before, plus RETAR A DUELO. Draws show as a third number (0-3-1). No win yet, so the next one would be the first: SI LE GANAS reads the base chance, 25%.',
    // facts: verdict refused, Currency_Refused, Reason_AllyCruel, allyTiered, allyLevel 90; duel offered, record 0-3-1; Balanced, base 25
    vm: {
      title: 'Reclutamiento', name: 'Grath', route: 'libre', routeTag: 'Libre',
      body: 'Grath no se une todavia.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      showDuelRecord: true, duelRecordText: '0-3-1',
      showDuelJoin: true, duelJoinText: '25% de que se una',
      reason: 'No camina al lado de nadie: su crueldad no se pone a tus ordenes', reasonRed: true,
      accept: '', extra: '', duel: 'RETAR A DUELO', cancel: 'CERRAR'
    }
  },
  {
    id: 'cheatDuel',
    title: 'Cheat: an 80+ NPC can be challenged too, and a win always recruits',
    note: 'The Cheat preset has no ally tiers, so the duel is gated on the level alone: any person (not an animal) at <code>allyTierEliteFromLevel</code> (80) or above, read with <code>SafeNpcLevel</code> like TotalSparring\'s card, with TotalSparring present and no cooldown (<code>CanOfferDuel</code>, <code>cheatPreset</code>). The Cheat answer stays as it is (here: not in a cage yet) and RETAR A DUELO joins the row. SI LE GANAS reads "se une": in Cheat a win always recruits, there is no roll and no win cooldown.',
    // facts: Preset_Cheat, cheatNeedsCage, not in my cage -> Reason_CheatNeedsCage; level 84 (SafeNpcLevel); duel offered, record 1-2
    vm: {
      title: 'Reclutamiento', name: 'Garo', route: 'libre', routeTag: 'Libre',
      body: 'Garo no se une todavia.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      showDuelRecord: true, duelRecordText: '1-2',
      showDuelJoin: true, duelJoinText: 'se une',
      reason: 'Metelo en una de tus jaulas y hablara enseguida', reasonRed: true,
      accept: '', extra: '', duel: 'RETAR A DUELO', cancel: 'CERRAR'
    }
  },
  {
    id: 'allyTooWeak',
    title: 'Ally refuses: leader too weak',
    note: 'The same Beep with a leader of level 60: 25 levels apart, past <code>allyTierCloseLevelGap</code> (<code>Reason_AllyLeaderTooWeak</code>). An ordinary refusal: the reason in red, CERRAR. The cruel at 80+ (<code>Reason_AllyCruel</code>) and the greedy at 100+ (<code>Reason_AllyGreedy</code>) read the same with their own line.',
    // facts: verdict refused, Currency_Refused, Reason_AllyLeaderTooWeak, allyTiered, allyLevel 85
    vm: {
      title: 'Reclutamiento', name: 'Beep el Sabio', route: 'libre', routeTag: 'Libre',
      body: 'Beep el Sabio no se une todavia.',
      showPrice: false, priceText: '', priceFree: false,
      showPurse: false, purseText: '', affordable: true,
      showChance: false, chanceText: '',
      reason: 'Eres demasiado debil para que te acompane', reasonRed: true,
      accept: '', extra: '', cancel: 'CERRAR'
    }
  },
  {
    id: 'allyFree',
    title: 'Good ally under 20 joins free',
    note: 'Balanced, an allied WarmKind of level 12: the one tier where friendship is enough. Today\'s ally roll still decides (<code>Reason_AllyJoins</code> on a pass, <code>Reason_AllyMaybe</code> on a miss), so the odds show.',
    // facts: verdict willJoin, Currency_None, Reason_AllyJoins, rolled 45%, allyTiered, allyLevel 12
    vm: {
      title: 'Reclutamiento', name: 'Ruka la Amable', route: 'libre', routeTag: 'Libre',
      body: 'Ruka la Amable es tu aliado (nivel 12) y se une gratis por amistad.',
      showPrice: true, priceText: 'Gratis', priceFree: true,
      showPurse: false, purseText: '', affordable: true,
      showChance: true, chanceText: '45%',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', cancel: 'CANCELAR'
    }
  },
  {
    id: 'yield',
    title: 'Joins free (field-yield)',
    note: 'Sometido: beaten enough times in the open and the roll passed (<code>Reason_FieldYieldJoins</code>). Its own tag (<code>readout.route.fieldYield</code>), not Rendicion, which is the "rather than die" surrender. Free, and the odds that were rolled.',
    // facts: verdict willJoin, Currency_Submission, rolled 41%
    vm: {
      title: 'Reclutamiento', name: 'Soldado Holy Nation', route: 'sometido', routeTag: 'Sometido',
      body: 'Soldado Holy Nation ya no tiene voluntad de pelear y desea unirse a tu grupo.',
      showPrice: true, priceText: 'Gratis', priceFree: true,
      showPurse: false, purseText: '', affordable: true,
      showChance: true, chanceText: '41%',
      reason: '', reasonRed: false,
      accept: 'ACEPTAR', extra: '', cancel: 'CANCELAR'
    }
  }
];

// The haggle states carry their inputs (hg); their haggle fields are what BuildOfferView's
// haggle step returns for them (js/haggle.js applyHaggle, the JS twin), so the slider can
// be dragged and the view rebuilt as offer_popup.cpp does.
POPUP_STATES.forEach(function (s) {
  s.vm.id = s.id;
  if (s.hg) {
    s.vm.hg = s.hg;
    applyHaggle(s.vm);
  }
});
