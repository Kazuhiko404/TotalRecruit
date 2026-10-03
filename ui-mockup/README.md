# ui-mockup (TotalRecruit readout and recruit popup)

Interactive HTML mockup the in-game readout is built from (source of truth, 1:1 with the game).
Open `readout.html` straight from disk in Chrome: no server, no build. Same conventions as
`TotalSparring/ui-mockup`: palette tokens in `css/base.css` (= `ui_palette.h`), component CSS whose
px values are the C++ constants, mock data in `js/data.js`, a pure renderer in `js/readout.js`.

| File | What it holds | C++ it mirrors |
|---|---|---|
| `readout.html` | page: the six states (expanded / compact / side by side), the placement scene (selected NPC / right-click menu open / press Hablar), the per-route table, the Mod Hub controls, the recruit popup gallery (section 5) | -- |
| `css/base.css` | palette tokens | `ui_style.h` / `ui_style.cpp` colours |
| `css/readout.css` | the card: widths, padding, bar heights, segment colours | `kCardWidth`, `kCardWidthCompact`, `kPad`, `kBarH` ... |
| `js/data.js` | one view-model per state (the facts that produce it are in a comment above each); `PLACEMENT` timings and gaps; `HUB` rows | `readout_model.cpp` output (`ReadoutView`); `readout.cpp` / `readout_ui.cpp` constants; `mod_hub_integration.cpp` |
| `js/readout.js` | draws a view-model as the card; `placeCard` (over the head, sideways beside the offer); `offerRect` (the popup centred) | `screen_projection.cpp` (`PlaceCard`, called by `readout_ui.cpp` `PlaceUnguarded`); `offer_popup.cpp` (`ChoosePosition`) |
| `css/popup.css` | the recruit popup: fonts and colours of each slot, flat buttons | `offer_popup.cpp` (widgets from `ui_style.cpp`) |
| `js/popup_data.js` | `POPUP_LAYOUT` (every px of the popup) and one `OfferView` per state, the facts above each | `offer_popup.cpp` constants; `offer_view.cpp` output (`OfferView`) |
| `js/popup.js` | `layoutOffer`: every rectangle of the popup, text measured first | `offer_popup.cpp` (`LayoutOffer`, `MeasureWrapped`) |

## Design decisions

1. **One card, anchored above the NPC.** The point 20 world units over the NPC's feet (`getPosition`, game space)
   has the scene's relative origin taken out (`SceneManager::getRelativeOrigin`, as the engine's own world-to-screen
   does) and is projected through the player camera (`screen_projection.cpp` `ProjectToScreen`); the card sits
   centred over that point, 6 px higher, clamped inside the view sideways, under the head when the top of the screen
   leaves no room. If the projection fails (behind the camera, off the screen) the card hides; there is no cursor
   fallback. Without the origin step the game-space point went through the render-space camera and landed on the
   horizon: the card "flying at the top of the screen" (2026-10-03). Not anchored to the cursor hint (the vanilla
   speech bubble is drawn by the engine and has no widget to anchor to) and not to the context menu (the Hablar
   button already lives beside it).
2. **Shown while the NPC is selected; Hablar places it beside the offer.** (User request 2026-10-03: "que se
   enable y disable al dar click en el NPC, no hover".) A left click on an NPC that is not yours shows the card;
   deselecting hides it and selecting someone else switches it. Each frame only compares the selection with the
   last one; the card is worked out when it changes and every 2 s after that (`kSelectionRefreshMs`), so price
   and stress stay current without an evaluation per frame. Pressing Hablar pins it: it stays over the head while the
   offer window that click opened is up, and only when it would cover that window does it step SIDEWAYS to the
   window's nearer side at the head's height (else the far side; hidden when neither has room). Never moved up or
   down to dodge the window, never clamped to the top of the screen. When the offer closes (or after 8 s when none opened) it
   stays only if that NPC is still selected. It hides while a right-click menu is open (the Hablar, TotalSparring
   and TotalLoot buttons take the free slots on either side, so no side is left to step to) and while the NPC is
   off the screen, and goes when they die and at a load. Hover is gone: polling the cursor and re-evaluating four
   times a second was slow and covered the screen. `readoutOnSelect` (on) off leaves only the Hablar card.
3. **It shows the engine's numbers, never its own.** Chance = `Verdict::chancePercent`; price = `Verdict::cats`;
   stress = `StressFromState`, split into the three terms it is made of (hunger, wounds, exhaustion) which add up
   to the bar; defeats against `defeatsToBreak`; animal bar = fullness against `TamingFullnessNeeded`.
4. **The card never reveals whether the roll passes, and never claims it failed.** The roll is stable and seeded from
   the character's id; showing its outcome would make the percent meaningless, and reading the id would mint one for
   a bystander. The readout is computed with no roll key (`EvaluatePeek`), so a route that rolled cannot say yes and
   would read "not yet" even when the click joins them. Those lines (field yield, ally) become the odds instead:
   "41%: puede unirse si le hablas" (`readout.line.rollOpen`). With a key, a failed field yield says "Aun no cede:
   vencelo otra vez" in the open, and "dejalo sin comer" only for someone in the player's own cage.
5. **Only what applies.** Stress bar for Cautiverio / Rendicion (the routes whose roll reads stress); chance bar when
   the route rolled; stomach bar for animals; defeats pips when defeats matter. A refused NPC shows just the route
   tag and the reason, because showing a 0% bar would suggest a roll that does not exist.
6. **One main line, at most one more.** The main line is the verdict's reason with its number: "Precio 9.570 cats
   (tienes 12.000)", "Faltan 3h en la jaula", "Hambre 56% / necesita 80%", "Rencor: no perdona fuera de la jaula".
   The expanded card adds a muted second line only for a caged cats-prisoner whose easy wait still runs. MyGUI text
   boxes do not wrap, so every line is written to fit the card (about 36 characters at the body font).
7. **Compact** drops the legend, the defeats row and the second line, narrows the card (176 vs 264) and thins
   the bars (5 vs 8). The route tag, the bars and the main line stay.
8. **Skin-mod safe.** Fills are a shipped white PNG tinted with bright foreground tokens (the stock `WhiteSkin`
   is the fallback); a tint multiplies against the skin underneath, so nothing foreground is dimmed. The card lives
   on the `ToolTip` layer, which has `Pick=false`, and every widget has `setNeedMouseFocus(false)`: it cannot take a click.
9. **Player-facing text** is the `readout` block of `messages.json` (Spanish, no accents, like the rest of the file).

### Recruit popup (section 5)

10. **One window for every Hablar outcome.** (User request 2026-10-03: "en vez de mostrar una notificacion de cuanto
    cuesta, muestra un popup con la informacion y Aceptar o Cancelar".) Yes, a price, a price the purse cannot meet,
    a refusal, a refusal with a remedy, a mercy plea and a rescue all open it; nothing ends in a notification.
    `money.short` survives only as the last-second re-check when ACEPTAR is pressed.
11. **Kenshi frame, mockup content.** `Kenshi_WindowCX` on the `Info` layer keeps the title bar, the X (= Cancel)
    and dragging; the client area gets a lacquer plate and flat art over it (TotalSparring `duel_popup`). Fills are
    created and textured before anything drawn on top of them.
12. **Layout.** Framed 110 px portrait on the left; on the right the name in brass with the route tag, a brass
    rule, the body line (wrapped and measured, never guessed), then an info box with only the rows that apply:
    PRECIO (amber, or `Gratis` in green), TIENES (red when short), PROBABILIDAD (when the verdict rolled). Under
    that, full width, the reason on a dark plate with a red bar, then the buttons. The window height is the sum.
13. **Buttons.** ACEPTAR = amber plate, dark text; the remedy / IGNORAR = amber edge, amber text; CANCELAR /
    CERRAR / RECHAZAR = line plate, golddim edge. Visible buttons share the row in that order; Cancel alone sits
    on the right at 150 px.
14. **Short purse = no ACEPTAR.** A cats price above the purse turns TIENES red, adds "No te alcanzan los cats
    (te faltan N)" and hides ACEPTAR. Any refusal hides ACEPTAR and shows its reason in red.
15. **Text** is the `popup` block of `messages.json`; the route tag reuses `readout.route.*`. Field yield has its own
    tag, **Sometido** (`readout.route.fieldYield`), on the card and the popup alike: Rendicion is the "rather than
    die" surrender.

## Checking a change

Mockup first, then C++. Measure in Chrome (`getBoundingClientRect`) and copy the px into the constants.
