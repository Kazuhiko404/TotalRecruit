// Draws one OfferView as the recruit popup. Presentation only: no rule lives here.
// C++ twin: LayoutOffer in src/offer_popup.cpp (widgets from src/ui_style.cpp), fed by
// BuildOfferView / BuildMercyOfferView in src/offer_view.cpp (this object).
//
// Every rectangle is computed here with the same arithmetic as LayoutOffer, in client
// coordinates; the two word-wrapped texts (body and reason) are measured first, as the
// C++ asks MyGUI for getTextSize() before sizing the window.

var PP_INFO_LABELS = { price: 'Precio', purse: 'Tienes', chance: 'Probabilidad' };

// Height of `text` wrapped at `width` in the class `cls` (the C++ MeasureWrapped).
function ppMeasure(text, width, cls) {
  var probe = document.createElement('div');
  probe.className = 'pp';
  probe.style.cssText = 'position:absolute;visibility:hidden;left:-9999px;top:0';
  probe.innerHTML = '<div class="cl"><div class="w ' + cls + '" style="width:' + width + 'px">' +
    esc(text) + '</div></div>';
  document.body.appendChild(probe);
  var h = Math.ceil(probe.querySelector('.w').getBoundingClientRect().height);
  document.body.removeChild(probe);
  return h;
}

function ppBox(cls, x, y, w, h, extra) {
  return '<div class="' + cls + '" style="left:' + x + 'px;top:' + y + 'px;width:' + w +
    'px;height:' + h + 'px;' + (extra || '') + '"';
}

function ppFill(x, y, w, h, colour) {
  return ppBox('f', x, y, w, h, 'background:' + colour) + '></div>';
}

function ppFrame(x, y, w, h, colour) {
  return ppFill(x, y, w, 1, colour) + ppFill(x, y + h - 1, w, 1, colour) +
         ppFill(x, y, 1, h, colour) + ppFill(x + w - 1, y, 1, h, colour);
}

// The route tag colour, as readout_ui.cpp colours the card's tag.
function ppTagColour(route) {
  return { cautiverio: 'var(--amber)', rendicion: 'var(--soft-danger)', sometido: 'var(--soft-danger)',
           rescate: 'var(--ready)',
           animal: '#9db8e6', piedad: 'var(--brass)', libre: 'var(--text)' }[route] || 'var(--text)';
}

// Flat button: optional 1px edge, plate, caption. style = 'accept' | 'extra' | 'cancel'.
function ppButton(label, style, x, y, w, h) {
  var s = {
    accept: { edge: null, plate: 'var(--amber)', ink: 'var(--lacquer)' },
    extra:  { edge: 'var(--amber)', plate: 'var(--sumi)', ink: 'var(--amber)' },
    cancel: { edge: 'var(--golddim)', plate: 'var(--line)', ink: 'var(--text)' }
  }[style];
  var html = '';
  if (s.edge) {
    html += ppFill(x, y, w, h, s.edge) + ppFill(x + 1, y + 1, w - 2, h - 2, s.plate);
  } else {
    html += ppFill(x, y, w, h, s.plate);
  }
  return html + ppBox('btn', x, y, w, h, 'color:' + s.ink) + '>' + esc(label) + '</div>';
}

// Returns { html, width, height } for the client area; popupHTML wraps it in the frame.
function layoutOffer(vm) {
  var L = POPUP_LAYOUT, W = L.clientW, m = L.margin, P = L.portrait;
  var x0 = m + P + 2 + L.colGap, colW = W - x0 - m;
  var back = '', front = '';   // fills first (paint order), then texts and buttons

  // Portrait framed in line.
  back += ppFrame(m, m, P + 2, P + 2, 'var(--line)');
  front += ppBox('portrait', m + 1, m + 1, P, P) + '></div>';

  // Name in brass, the route as a framed tag, a golddim rule under them.
  var y = m;
  var tagY = y + Math.floor((L.nameH - L.tagH) / 2), tagC = ppTagColour(vm.route);
  front += ppBox('t nm', x0, y, colW - L.tagW - 6, L.nameH) + '>' + esc(vm.name) + '</div>';
  back += ppFrame(x0 + colW - L.tagW, tagY, L.tagW, L.tagH, tagC);
  front += ppBox('t tag', x0 + colW - L.tagW, tagY, L.tagW, L.tagH, 'color:' + tagC) + '>' +
           esc(vm.routeTag) + '</div>';
  y += L.nameH + L.ruleGap;
  back += ppFill(x0, y, colW, 1, 'var(--golddim)');
  y += 1 + L.bodyGap;

  // The body line, wrapped and measured.
  var bodyH = ppMeasure(vm.body, colW, 'body');
  front += ppBox('w body', x0, y, colW, bodyH) + '>' + esc(vm.body) + '</div>';
  y += bodyH;

  // Info box: line frame around a bgdark plate, muted labels, coloured values.
  var rows = [];
  if (vm.showPrice) rows.push([PP_INFO_LABELS.price, vm.priceText, vm.priceFree ? 'var(--ready)' : 'var(--amber)']);
  if (vm.showPurse) rows.push([PP_INFO_LABELS.purse, vm.purseText, vm.affordable ? 'var(--text)' : 'var(--soft-danger)']);
  if (vm.showChance) rows.push([PP_INFO_LABELS.chance, vm.chanceText, 'var(--text)']);
  if (rows.length) {
    y += L.infoGap;
    var infoH = rows.length * L.infoRowH + 2 * L.infoPad;
    back += ppFill(x0, y, colW, infoH, 'var(--line)') + ppFill(x0 + 1, y + 1, colW - 2, infoH - 2, 'var(--bgdark)');
    rows.forEach(function (r, i) {
      var ry = y + L.infoPad + i * L.infoRowH;
      front += ppBox('t lbl', x0 + 10, ry, L.infoLabelW, L.infoRowH) + '>' + esc(r[0]) + '</div>';
      front += ppBox('t val', x0 + 10 + L.infoLabelW, ry, colW - 20 - L.infoLabelW, L.infoRowH,
                     'color:' + r[2]) + '>' + esc(r[1]) + '</div>';
    });
    y += infoH;
  }

  y = Math.max(y, m + P + 2);

  // The reason: a bgdark plate across the window, a red bar on its left when it is a no.
  if (vm.reason) {
    y += L.sectionGap;
    var textX = m + L.reasonBar + L.reasonPadX, textW = W - 2 * m - L.reasonBar - 2 * L.reasonPadX;
    var textH = ppMeasure(vm.reason, textW, 'why');
    var plateH = textH + 2 * L.reasonPadY;
    back += ppFill(m, y, W - 2 * m, plateH, 'var(--bgdark)');
    if (vm.reasonRed) back += ppFill(m, y, L.reasonBar, plateH, 'var(--danger)');
    front += ppBox('w why' + (vm.reasonRed ? '' : ' plain'), textX, y + L.reasonPadY, textW, textH) + '>' +
             esc(vm.reason) + '</div>';
    y += plateH;
  }

  // Buttons: Accept, Extra, Cancel share the row; Cancel alone sits on the right.
  y += L.sectionGap;
  var buttons = [];
  if (vm.accept) buttons.push([vm.accept, 'accept']);
  if (vm.extra) buttons.push([vm.extra, 'extra']);
  buttons.push([vm.cancel, 'cancel']);
  if (buttons.length === 1) {
    front += ppButton(vm.cancel, 'cancel', W - m - L.aloneW, y, L.aloneW, L.buttonH);
  } else {
    var bw = Math.floor((W - 2 * m - (buttons.length - 1) * L.buttonGap) / buttons.length);
    buttons.forEach(function (b, i) {
      front += ppButton(b[0], b[1], m + i * (bw + L.buttonGap), y, bw, L.buttonH);
    });
  }
  var H = y + L.buttonH + m;

  // The lacquer plate is the first child (created and textured first in the C++).
  var html = ppFill(0, 0, W, H, 'var(--lacquer)') + back + front;
  return { html: html, width: W, height: H };
}

function popupHTML(vm) {
  var c = layoutOffer(vm);
  return '<div class="pp"><div class="ttl"><span>' + esc(vm.title) + '</span><span class="x">X</span></div>' +
    '<div class="cl" style="height:' + c.height + 'px">' + c.html + '</div></div>';
}
