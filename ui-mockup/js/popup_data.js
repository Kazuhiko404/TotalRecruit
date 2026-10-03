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
  aloneW: 150        // kAloneButtonW: Cancel / Close when it is the only button
};

var POPUP_STATES = [
  {
    id: 'cats',
    title: 'Joins for cats',
    note: 'Cautiverio, the roll passed and the price is due: <code>Reason_CatsJoin</code>. Price in amber, the purse it is checked against, the odds the engine rolled. Money only moves on ACEPTAR (CompleteOffer re-checks and charges).',
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
    id: 'short',
    title: "Can't afford",
    note: 'Libre, a mercenary who wants 9.570 and the purse holds 3.200: <code>Reason_CatsAsk</code>. Same price block, the purse in red, the red line says by how much, and ACEPTAR is hidden: there is nothing to accept. Only CANCELAR.',
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
    note: 'Cautiverio, hungry and you carry food: <code>Reason_Starving</code> with the feed remedy. The reason in red and the button that acts on it; DARLE COMIDA feeds and reopens the popup with the new verdict (its Cancel then reads AHORA NO). GOLPEARLO takes this slot when the remedy is the beating.',
    // facts: verdict refused, Currency_Care, wantsFood, player has food
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
