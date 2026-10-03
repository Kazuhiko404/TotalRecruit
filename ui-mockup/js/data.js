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
    note: 'Rendicion: beaten enough times in the open. Defeats reached the threshold, stress is whatever state they are in now, and the single line says what would change it.',
    // facts: route FieldYield, defeats 2/2, stress .35, chance 41, roll not passed
    vm: {
      name: 'Soldado Holy Nation', route: 'rendicion', routeLabel: 'Rendicion',
      showStress: true, stressPct: 35, stress: { hunger: 5, wounds: 25, tired: 5 },
      showChance: true, chancePct: 41, ready: false,
      showDefeats: true, defeats: 2, defeatsNeeded: 2,
      showFood: false,
      why:  { text: 'Aun no cede: dejalo sin comer', tone: '' },
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
    note: 'Libre, ally. Chance from standing and level gap; the line is what the NPC says when the roll does not carry them (no price in this branch: allies are asked, not bribed). (The readout never says whether the roll passed: the odds are shown, the result is still the click\'s.)',
    // facts: Route FreeWorld, relation +40, level 18 vs 20, honorable? no. chance 63
    vm: {
      name: 'Mercader Ostrel', route: 'libre', routeLabel: 'Libre',
      showStress: false, stressPct: 0, stress: { hunger: 0, wounds: 0, tired: 0 },
      showChance: true, chancePct: 63, ready: false,
      showDefeats: false, defeats: 0, defeatsNeeded: 0,
      showFood: false,
      why:  { text: 'Quiza te siga; si no, hazte mas fuerte', tone: '' },
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

// Where the card goes and when (src/readout.cpp, src/readout_ui.cpp, src/offer_popup.cpp).
// By default the card is pinned by the Hablar click: worked out once, shown beside the offer
// window that click opened, gone when that window closes or after pinTimeoutMs when none
// opened. Hover is the opt-in mode that re-evaluates every hoverIntervalMs.
var PLACEMENT = {
  arrowGap: 8,          // kArrowGap
  screenMargin: 4,      // kScreenMargin
  anchorLift: 70,       // kAnchorLiftPx (readout_target.cpp)
  pinTimeoutMs: 8000,   // kPinTimeoutMs
  hoverIntervalMs: 250, // kHoverIntervalMs
  offer: { width: 620, height: 260 } // kBaseWidth, kBaseHeightNoExtra
};

// Mod Hub rows, word for word as src/mod_hub_integration.cpp registers them, with the
// src/config.h default.
var HUB = [
  ['Lectura: activa', 'readoutEnabled', 'ON',
   'Muestra estres, chance y que falta del NPC al apretar Hablar'],
  ['Lectura: compacta', 'readoutCompact', 'OFF',
   'Version mas chica: sin el desglose del estres ni las derrotas'],
  ['Lectura: al pasar el mouse', 'readoutOnHover', 'OFF',
   'Opcional y cuesta rendimiento. Apagado, la lectura aparece al apretar Hablar, junto a la oferta']
];
