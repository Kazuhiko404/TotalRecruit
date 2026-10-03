# ui-mockup (TotalRecruit readout)

Interactive HTML mockup the in-game readout is built from (source of truth, 1:1 with the game).
Open `readout.html` straight from disk in Chrome: no server, no build. Same conventions as
`TotalSparring/ui-mockup`: palette tokens in `css/base.css` (= `ui_palette.h`), component CSS whose
px values are the C++ constants, mock data in `js/data.js`, a pure renderer in `js/readout.js`.

| File | What it holds | C++ it mirrors |
|---|---|---|
| `readout.html` | page: the six states (expanded / compact / side by side), the placement scene (press Hablar / hover opt-in), the per-route table, the Mod Hub controls | -- |
| `css/base.css` | palette tokens | `readout_ui.cpp` colours |
| `css/readout.css` | the card: widths, padding, bar heights, segment colours | `kCardWidth`, `kCardWidthCompact`, `kPad`, `kBarH` ... |
| `js/data.js` | one view-model per state (the facts that produce it are in a comment above each); `PLACEMENT` timings and gaps; `HUB` rows | `readout_model.cpp` output (`ReadoutView`); `readout.cpp` / `readout_ui.cpp` constants; `mod_hub_integration.cpp` |
| `js/readout.js` | draws a view-model as the card; `placeCard` (beside the offer); the offer window | `readout_ui.cpp` (`MoveUnguarded`, `StepAside`); `offer_popup.cpp` |

## Design decisions

1. **One card, anchored above the NPC.** The NPC's world position is projected to the screen and the card sits
   centred over their head, clamped inside the view; if the projection fails it falls back to the cursor. Not
   anchored to the cursor hint (the vanilla speech bubble is drawn by the engine and has no widget to anchor to)
   and not to the context menu (the Hablar button already lives beside it).
2. **Shown when you press Hablar, beside the offer; hover is an opt-in in Mod Hub.** The Hablar click pins the
   card: it is worked out once, at the click, and then only follows the NPC. When it would cover the offer
   window that click opened it steps beside it (right, else left, else above / below). It goes away when the
   offer closes, after 8 s when no offer opened, when the NPC dies or leaves the screen, when another Hablar
   replaces it, and at a load. Nothing is shown on hover or selection by default: polling the cursor and
   re-evaluating four times a second cost frame time with many NPCs on screen, so `readoutOnHover` is off
   and opt-in (hidden while the offer is open and when the mouse is over any window).
3. **It shows the engine's numbers, never its own.** Chance = `Verdict::chancePercent`; price = `Verdict::cats`;
   stress = `StressFromState`, split into the three terms it is made of (hunger, wounds, exhaustion) which add up
   to the bar; defeats against `defeatsToBreak`; animal bar = fullness against `TamingFullnessNeeded`.
4. **The card never reveals whether the roll passes.** The roll is stable and seeded from the character's id; showing
   its outcome would make the percent meaningless, and reading the id would mint one for a bystander. The readout is
   computed with no roll key, so it shows the odds and what is needed if luck does not carry them.
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

## Checking a change

Mockup first, then C++. Measure in Chrome (`getBoundingClientRect`) and copy the px into the constants.
