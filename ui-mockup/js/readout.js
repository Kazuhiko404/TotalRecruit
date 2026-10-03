// Draws one view-model as the readout card. Presentation only: no rule lives here.
// C++ twin: src/readout_ui.cpp (widgets) fed by src/readout_model.cpp (this object).

function esc(s) {
  return String(s).replace(/[&<>"]/g, function (c) {
    return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c];
  });
}

function barRow(label, valueHtml, trackHtml) {
  return '<div class="row"><span>' + label + '</span>' + valueHtml + '</div>' + trackHtml;
}

function stressBlock(vm, compact) {
  var s = vm.stress;
  var track = '<div class="track">' +
    '<div class="seg hunger" style="width:' + s.hunger + '%"></div>' +
    '<div class="seg wounds" style="width:' + s.wounds + '%"></div>' +
    '<div class="seg tired" style="width:' + s.tired + '%"></div></div>';
  var html = barRow('Estres', '<b>' + vm.stressPct + '%</b>', track);
  if (!compact) {
    html += '<div class="legend">' +
      '<span><i style="background:var(--hunger)"></i>Hambre ' + s.hunger + '</span>' +
      '<span><i style="background:var(--wounds)"></i>Heridas ' + s.wounds + '</span>' +
      '<span><i style="background:var(--tired)"></i>Cansancio ' + s.tired + '</span></div>';
  }
  return html;
}

function chanceBlock(vm) {
  var cls = vm.chancePct >= 66 ? 'good' : (vm.chancePct < 25 ? 'low' : '');
  var track = '<div class="track"><div class="fill ' + cls + '" style="width:' + vm.chancePct + '%"></div></div>';
  return barRow('Chance', '<b class="hot">' + vm.chancePct + '%</b>', track);
}

function foodBlock(vm) {
  var cls = vm.foodPct >= vm.foodNeededPct ? 'good' : '';
  var track = '<div class="track"><div class="fill ' + cls + '" style="width:' + vm.foodPct + '%"></div>' +
    '<div class="notch" style="left:' + vm.foodNeededPct + '%"></div></div>';
  return barRow('Barriga', '<b>' + vm.foodPct + '%</b>', track);
}

function defeatsRow(vm) {
  var pips = '';
  for (var i = 0; i < vm.defeatsNeeded; i++) {
    pips += '<span class="pip' + (i < vm.defeats ? ' on' : '') + '"></span>';
  }
  return '<div class="row"><span>Derrotas</span><span><span class="pips">' + pips + '</span><b>' +
    vm.defeats + '/' + vm.defeatsNeeded + '</b></span></div>';
}

// The head notice over an NPC who called out (src/head_notice.cpp LayoutUnguarded): the name,
// a gold rule, and the line from messages.json headNotice.*.
function headNoticeHTML(name, line) {
  return '<div class="hn"><div class="nm">' + esc(name) + '</div><div class="ln">' + esc(line) + '</div></div>';
}

function readoutHTML(vm, compact, noArrow) {
  var h = '<div class="rd' + (compact ? ' compact' : '') + (noArrow ? ' noarrow' : '') + '">';
  h += '<div class="hd"><span class="nm">' + esc(vm.name) + '</span>' +
       '<span class="tag r-' + vm.route + '">' + esc(vm.routeLabel) + '</span></div>';
  if (vm.showStress) h += stressBlock(vm, compact);
  if (vm.showFood) h += foodBlock(vm);
  if (vm.showChance) h += chanceBlock(vm);
  if (vm.showDefeats && !compact) h += defeatsRow(vm);
  h += '<div class="why ' + vm.why.tone + '" title="' + esc(vm.why.text) + '">' + esc(vm.why.text) + '</div>';
  if (vm.alt && !compact) h += '<div class="why alt">' + esc(vm.alt.text) + '</div>';
  return h + '</div>';
}

function stateById(id) {
  for (var i = 0; i < STATES.length; i++) if (STATES[i].id === id) return STATES[i];
  return STATES[0];
}

// Where the card's top-left goes. C++ twin: PlaceCard in src/screen_projection.cpp (called by
// readout_ui.cpp PlaceUnguarded). Centred over the anchor (the projected point over the head),
// clamped to the view sideways; under the head when there is no room above. When it would cover
// `avoid` (the offer window) it steps SIDEWAYS to the window's nearer side at the same height,
// else the far side; with no room on either side it is not shown (placement 'none'). Never
// moved up or down to dodge the window, never pinned to the top of the screen.
function placeCard(anchor, size, view, avoid) {
  var g = PLACEMENT.arrowGap, m = PLACEMENT.screenMargin;
  var left = anchor.x - Math.floor(size.w / 2);
  var top = anchor.y - size.h - g;
  if (left + size.w > view.w - m) left = view.w - m - size.w;
  if (left < m) left = m;
  if (top < m) top = anchor.y + PLACEMENT.belowGap;
  if (top + size.h > view.h - m) top = view.h - m - size.h;
  if (!avoid || !(left < avoid.left + avoid.width && avoid.left < left + size.w &&
                  top < avoid.top + avoid.height && avoid.top < top + size.h)) {
    return { left: left, top: top, placement: 'overHead' };
  }
  var right = avoid.left + avoid.width + g, leftSide = avoid.left - g - size.w;
  var rightFits = right + size.w <= view.w - m, leftFits = leftSide >= m;
  var nearerIsRight = 2 * anchor.x >= 2 * avoid.left + avoid.width;
  if (nearerIsRight ? rightFits : leftFits) {
    return { left: nearerIsRight ? right : leftSide, top: top, placement: 'beside' };
  }
  if (nearerIsRight ? leftFits : rightFits) {
    return { left: nearerIsRight ? leftSide : right, top: top, placement: 'beside' };
  }
  return { left: left, top: top, placement: 'none' };
}

// The recruit popup as src/offer_popup.cpp centres it (ChoosePosition), given its
// measured size (popup.js draws it).
function offerRect(view, size) {
  return { left: Math.floor((view.w - size.w) / 2), top: Math.floor((view.h - size.h) / 2),
           width: size.w, height: size.h };
}
