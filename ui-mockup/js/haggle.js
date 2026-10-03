// The haggle slider's rules, copied from the C++ so the mockup's slider can be dragged.
// No drawing here (popup.js draws); no rule here that the C++ does not have.
//   haggle_rules.cpp   HaggleSliderBounds, ClampOffer, HaggleStepCount, OfferAtStep,
//                      StepOfOffer, HaggleLeverage, HagglePersonalityShift, AcceptOdds
//   flat_slider.h      ValueFromX, ThumbLeftFor (pixel <-> slider position)
//   offer_view.cpp     the haggle step of BuildOfferView (ApplyHaggle): which line, which
//                      colour, which button
// Tests that pin the C++ twins: tests/rules_tests/haggle_rules_tests.cpp,
// flat_slider_tests.cpp, offer_view_tests.cpp.

// config.json "prices" defaults (config.h Prices).
var HAGGLE_PRICES = {
  haggleEnabled: true,
  haggleMinFraction: 0.5,
  haggleDiscountWeight: 2.0,
  haggleLeverageWeight: 0.5,
  haggleOffenceMarkup: 0.2
};

// The flat slider's art (TotalSparring betting.css): 10 px track, 22 px square thumb.
var SLIDER_ART = { trackH: 10, trackBorder: 1, thumb: 22, thumbBorder: 2 };

var HAGGLE_STEP = 10;

function hgRoundUpToStep(v, step) {
  if (step <= 0 || v <= 0) return v < 0 ? 0 : v;
  return Math.floor((v + step - 1) / step) * step;
}

function hgBounds(ask, p) {
  var b = { min: 0, max: 0, step: HAGGLE_STEP };
  if (ask <= 0) return b;
  var floorCats = Math.ceil(ask * p.haggleMinFraction - 1e-3);
  b.max = ask;
  b.min = Math.min(hgRoundUpToStep(floorCats, HAGGLE_STEP), ask);
  if (b.min < 0) b.min = 0;
  return b;
}

function hgStepCount(b) {
  if (b.max <= b.min || b.step <= 0) return 0;
  return Math.floor((b.max - b.min + b.step - 1) / b.step);
}

function hgOfferAtStep(b, i) {
  var n = hgStepCount(b);
  if (i <= 0) return b.min;
  if (i >= n) return b.max;
  return Math.min(b.min + i * b.step, b.max);
}

function hgClampOffer(offer, b) {
  if (offer >= b.max) return b.max;
  if (offer <= b.min) return b.min;
  var snapped = b.step > 0 ? Math.floor((offer + Math.floor(b.step / 2)) / b.step) * b.step : offer;
  if (b.max - offer < Math.abs(snapped - offer)) return b.max;
  return Math.max(b.min, Math.min(b.max, snapped));
}

function hgStepOfOffer(b, offer) {
  var n = hgStepCount(b);
  var o = hgClampOffer(offer, b);
  if (o >= b.max) return n;
  return Math.min(n, Math.floor((o - b.min + Math.floor(b.step / 2)) / b.step));
}

// Fearful +15; Brave, ColdCruel, Maniacal -10; Honorable -15.
function hgPersonalityShift(temperament) {
  return { fearful: 15, brave: -10, coldcruel: -10, maniacal: -10, honorable: -15 }[temperament] || 0;
}

function hgLeverage(speakerBest, npcLevel, p) {
  var raw = (speakerBest - npcLevel) * p.haggleLeverageWeight;
  return Math.max(-20, Math.min(20, Math.floor(raw + 0.5)));
}

function hgAcceptOdds(ask, offer, leverage, temperament, p) {
  if (ask <= 0 || offer >= ask) return 100;
  var discount = (ask - offer) * 100 / ask;
  var odds = Math.floor(100 - discount * p.haggleDiscountWeight + 0.5) + leverage + hgPersonalityShift(temperament);
  return Math.max(0, Math.min(95, odds));
}

// ready >= 70, amber 30..69, danger < 30 (OddsToneOf).
function hgTone(pct) {
  return pct >= 70 ? 'ready' : (pct >= 30 ? 'amber' : 'danger');
}

// flat_slider.h: the thumb's centre follows the mouse; travel = width - thumb.
function hgValueFromX(localX, width, thumb, maxValue) {
  if (maxValue <= 0) return 0;
  var travel = width - thumb;
  if (travel <= 0) return 0;
  var left = Math.max(0, Math.min(travel, localX - Math.floor(thumb / 2)));
  var v = Math.floor((left * maxValue * 2 + travel) / (travel * 2));
  return Math.max(0, Math.min(maxValue, v));
}

function hgThumbLeft(value, width, thumb, maxValue) {
  var travel = width - thumb;
  if (travel <= 0 || maxValue <= 0) return 0;
  var v = Math.max(0, Math.min(maxValue, value));
  return Math.max(0, Math.min(travel, Math.floor(v * travel / maxValue)));
}

function hgCats(n) {
  return String(n).replace(/\B(?=(\d{3})+(?!\d))/g, '.');
}

// The haggle step of BuildOfferView: vm.hg holds the inputs (ask, purse, leverage,
// temperament, locked, offer); the view fields it fills are the ones layoutOffer reads.
// vm.hg.offer is the slider value (0 = start at the ask).
var HAGGLE_TEXTS = {
  label: 'Oferta',
  odds: 'Probabilidad de aceptar: {pct}%',
  offer: 'OFRECER',
  offended: 'Se ofendio: hoy no regatea',
  short: 'No te alcanzan para esa oferta (te faltan {cats})'
};

function applyHaggle(vm) {
  var h = vm.hg;
  if (!h) return vm;
  var p = HAGGLE_PRICES;
  vm.showSlider = false;
  vm.haggleLine = '';
  if (!p.haggleEnabled) return vm;
  if (h.locked) {
    vm.haggleLine = HAGGLE_TEXTS.offended;
    vm.haggleTone = 'danger';
    return vm;
  }
  var b = hgBounds(h.ask, p);
  if (b.min >= b.max) return vm;
  var offer = h.offer > 0 ? hgClampOffer(h.offer, b) : b.max;
  h.offer = offer;
  vm.showSlider = true;
  vm.sliderSteps = hgStepCount(b);
  vm.sliderStep = hgStepOfOffer(b, offer);
  vm.offerText = hgCats(offer) + ' cats';
  vm.minText = hgCats(b.min);
  vm.maxText = hgCats(b.max);
  vm.oddsPercent = hgAcceptOdds(h.ask, offer, h.leverage, h.temperament, p);
  vm.cancel = 'CANCELAR';
  vm.reason = '';
  vm.reasonRed = false;
  if (h.purse < offer) {
    vm.affordable = false;
    vm.haggleLine = HAGGLE_TEXTS.short.replace('{cats}', hgCats(offer - h.purse));
    vm.haggleTone = 'danger';
    vm.accept = '';
    return vm;
  }
  vm.affordable = true;
  vm.haggleLine = HAGGLE_TEXTS.odds.replace('{pct}', vm.oddsPercent);
  vm.haggleTone = hgTone(vm.oddsPercent);
  vm.accept = offer < h.ask ? HAGGLE_TEXTS.offer : 'ACEPTAR';
  return vm;
}

// Moves a state's slider to `step` and recomputes its view.
function setHaggleStep(vm, step) {
  var b = hgBounds(vm.hg.ask, HAGGLE_PRICES);
  vm.hg.offer = hgOfferAtStep(b, step);
  return applyHaggle(vm);
}
