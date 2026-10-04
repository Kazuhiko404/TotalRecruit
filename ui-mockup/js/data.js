// Mock view-models. Each object is exactly what src/readout_model.cpp returns for the facts in
// the comment above it (the C++ tests pin that mapping); the renderer in readout.js only draws.
// Text values are the messages.json "readout" block, in the mod's player language (no accents,
// like the rest of messages.json).

var STATES = [
  {
    id: 'cage',
    title: 'Caged bandit',
    note: 'Cautiverio, easy wait on. Stress comes from an empty stomach and untreated wounds; the chance is JoinChance(...) and the price is FinalPriceInCats(...) -- the same numbers Evaluate uses. A cats-currency prisoner is the one case with two lines.',
    // facts: route Cage, level 14 vs player 20, hunger raw 1.2/3, worstDamage .4, rested .7,
    // defeats 1/2, held 3h of 6h, price 9570 (purse 12000)
    vm: {
      name: 'Kareem el Tuerto', route: 'cautiverio', routeLabel: 'Cautiverio',
      showStress: true, stressPct: 53, stress: { hunger: 30, wounds: 14, tired: 9 },
      showChance: true, chancePct: 58, ready: false,
      showDefeats: true, defeats: 1, defeatsNeeded: 2,
      showFood: false,
      why:  { text: 'Precio 9.570 cats (tienes 12.000)', tone: '' },
      alt:  { text: 'O espera: faltan 3h en la jaula' }
    }
  },
  {
    id: 'yield',
    title: 'Field-yield candidate',
    note: 'Sometido (its own tag, not Rendicion): beaten enough times in the open. The card asks as a bystander with no roll key, so it cannot know whether the roll passes: instead of a "not yet" that the click may contradict, the line states the odds (<code>kLineRollOpen</code>). With the key and a failed roll the line is "Aun no cede: vencelo otra vez" in the open, and "dejalo sin comer" only for someone in your own cage.',
    // facts: route FieldYield, defeats 2/2, stress .35, chance 41, peek (no roll key), not in your cage
    vm: {
      name: 'Soldado Holy Nation', route: 'sometido', routeLabel: 'Sometido',
      showStress: true, stressPct: 35, stress: { hunger: 5, wounds: 25, tired: 5 },
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
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0 },
      showChance: false, chancePct: 0, ready: false,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: true, foodPct: 56, foodNeededPct: 80,
      why:  { text: 'Hambre 56% / necesita 80%', tone: '' },
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
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0 },
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
    note: 'Libre, ally, Balanced: the level tier decides (<code>ally_tier_rules.h</code>). A Normal of level 34 pays a friend\'s price, no roll, so no chance bar: the line is the discounted price (<code>readout.line.ally.paidJoin</code>; short of cats it is <code>paidAsk</code> with the purse). A Good ally under 20 still rolls and shows the odds; 80+ reads "Aliado: te acompana unos dias" or the tier refusal. Vanilla+ keeps the single roll for every ally: chance bar and "Quiza te siga; si no, hazte mas fuerte".',
    // facts: Route FreeWorld, Balanced, relation +40, level 34 vs 20, Normal -> Paid 20%; cats 1840, willJoin
    vm: {
      name: 'Mercader Ostrel', route: 'libre', routeLabel: 'Libre',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0 },
      showChance: false, chancePct: 0, ready: false,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      why:  { text: 'Aliado: 1.840 cats con descuento', tone: 'ok' },
      alt:  null
    }
  },
  {
    id: 'rescue',
    title: 'Saved life (will join)',
    note: 'Rescate with a certain outcome (no roll left): the chance row is replaced by the result. Same card, the line goes green.',
    // facts: Route EasyHealthy / Surrendered / ... willJoin without a roll
    vm: {
      name: 'Dalia', route: 'rescate', routeLabel: 'Rescate',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0 },
      showChance: false, chancePct: 0, ready: true,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      why:  { text: 'Te debe la vida: se une', tone: 'ok' },
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
// after headNoticeSeconds, when that NPC is selected, when they die or join, or at a load.
// temp_member.cpp raises squad notices (RaiseSquadHeadNotice) over a temporary member: kept while
// they are in the squad, never dismissed by selecting them.
var HEAD_NOTICE = {
  seconds: 300,          // headNoticeSeconds default (config.h), clamp 10..3600
  cap: 8,                // kHeadNoticeCap: one per NPC, the oldest goes when full
  wantsToTalk: 'Quiere hablar contigo',          // messages.json headNotice.wantsToTalk
  approaching: 'Se acerca para hablar contigo'   // messages.json headNotice.approaching
};

// What a temporary member (80+ ally, src/temp_member.h) says over their head: messages.json
// tempMember.notice.*. {days} is filled in.
var TEMP_MEMBER = {
  join: 'Te acompanare 6 dias',                       // on joining
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

// Mod Hub rows, word for word as src/mod_hub_integration.cpp registers them, with the
// src/config.h default.
var HUB = [
  ['Lectura: activa', 'readoutEnabled', 'ON',
   'Muestra estres, chance y que falta del NPC seleccionado o al apretar Hablar'],
  ['Lectura: compacta', 'readoutCompact', 'OFF',
   'Version mas chica: sin el desglose del estres ni las derrotas'],
  ['Lectura: al seleccionar', 'readoutOnSelect', 'ON',
   'La lectura sigue al NPC que seleccionas con clic izquierdo. Apagado, solo aparece al apretar Hablar'],
  ['Aviso: segundos sobre la cabeza', 'headNoticeSeconds', '300',
   'Cuanto queda el cartel \'Quiere hablar contigo\' sobre quien te llama (seleccionarlo lo quita)']
];
