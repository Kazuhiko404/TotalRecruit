# ui-mockup (TotalRecruit readout and recruit popup)

Interactive HTML mockup the in-game readout is built from (source of truth, 1:1 with the game).
Open `readout.html` straight from disk in Chrome: no server, no build. Same conventions as
`TotalSparring/ui-mockup`: palette tokens in `css/base.css` (= `ui_palette.h`), component CSS whose
px values are the C++ constants, mock data in `js/data.js`, a pure renderer in `js/readout.js`.

| File | What it holds | C++ it mirrors |
|---|---|---|
| `readout.html` | page: the six states (expanded / compact / side by side), the placement scene (selected NPC / right-click menu open / menu on your prisoner with Golpear and Noquear, one column and two columns / press Hablar / head notice and selecting the one who called / proactive offer: a person and an animal walk up, a bad roll calms), the per-route table, the Mod Hub controls, the recruit popup gallery (section 5) | -- |
| `css/base.css` | palette tokens | `ui_style.h` / `ui_style.cpp` colours |
| `css/readout.css` | the card: widths, padding, bar heights, segment colours | `kCardWidth`, `kCardWidthCompact`, `kPad`, `kBarH` ... |
| `js/data.js` | one view-model per state (the facts that produce it are in a comment above each); `PLACEMENT` timings and gaps; `HUB` rows | `readout_model.cpp` output (`ReadoutView`); `readout.cpp` / `readout_ui.cpp` constants; `mod_hub_integration.cpp` |
| `js/readout.js` | draws a view-model as the card; `placeCard` (over the head, sideways beside the offer); `offerRect` (the popup centred); `menuAnchorUnion` (the native menu's one or two columns as one rectangle) | `screen_projection.cpp` (`PlaceCard`, called by `readout_ui.cpp` `PlaceUnguarded`); `offer_popup.cpp` (`ChoosePosition`); `menu_anchor.cpp` (`UnionMenuRect`) |
| `css/popup.css` | the recruit popup: fonts and colours of each slot, flat buttons | `offer_popup.cpp` (widgets from `ui_style.cpp`) |
| `js/popup_data.js` | `POPUP_LAYOUT` (every px of the popup) and one `OfferView` per state, the facts above each | `offer_popup.cpp` constants; `offer_view.cpp` output (`OfferView`) |
| `js/popup.js` | `layoutOffer`: every rectangle of the popup, text measured first; the haggle section and the slider art | `offer_popup.cpp` (`LayoutOffer`, `MeasureWrapped`); `flat_slider.cpp` |
| `js/haggle.js` | JS copy of the haggle rules (bounds, steps, odds), the slider's pixel math and the haggle step of the view, so the slider can be dragged | `haggle_rules.cpp`; `flat_slider.h`; `offer_view.cpp` (`ApplyHaggle`) |

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
   A left or right press anywhere that is not a MyGUI widget nor the NPC's body puts it away (Kenshi keeps the
   selection on a right-click to walk); it comes back on a left click on that NPC, a new selection or Hablar, and a
   Hablar card beside an open offer ignores outside clicks (`card_dismiss_rules.cpp`, user report 2026-10-03).
3. **It shows the engine's numbers, never its own.** Chance = `Verdict::chancePercent`; price = `Verdict::cats`;
   stress = `StressFromState`, split into the four terms it is made of (hunger, wounds, exhaustion, beatings) which
   add up to the bar -- the fourth, Golpes, is the beating stress Golpear and Noquear leave in the player's cage
   (`beating_stress_rules.h`, fading over `submissionBeatingStressFadeHours`), its legend on a row of its own because
   three legends already fill the card's width; defeats against `defeatsToBreak`; animal bar = fullness against `TamingFullnessNeeded`.
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
   (tienes 12.000)", "Desafiante: hambre o golpes lo ablandan", "Hambre 56% / necesita 80%", "Rencor: no perdona fuera de la jaula".
   The expanded card adds a muted second line only for a challengeable ally's duel record, and a "Lo cuidaste
   +N%" row under the chance for a Balanced prisoner you cared for (v1-presets-close P4). MyGUI text
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
13. **Buttons.** ACEPTAR = amber plate, dark text; the remedy / IGNORAR / PERDONAR / GOLPEAR / NOQUEAR = amber
    edge, amber text; CANCELAR / CERRAR / RECHAZAR = line plate, golddim edge. Visible buttons share the row in the
    order ACEPTAR, extra, GOLPEAR, NOQUEAR, Cancel; Cancel alone sits on the right at 150 px. More than three
    (`kButtonsPerRow`): the others share the first row and Cancel wraps alone to the right of a second row, 8 px
    under it (DARLE COMIDA would not hold at a quarter of the row).
14. **Short purse = no ACEPTAR.** A cats price above the purse turns TIENES red, adds "No te alcanzan los cats
    (te faltan N)" and hides ACEPTAR. Any refusal hides ACEPTAR and shows its reason in red.
15. **Text** is the `popup` block of `messages.json`; the route tag reuses `readout.route.*`. Field yield has its own
    tag, **Sometido** (`readout.route.fieldYield`), on the card and the popup alike: Rendicion is the "rather than
    die" surrender.

16. **Haggling** (`haggleEnabled`; user request 2026-10-03). Only a cats price gets it (free-world bribe, the
    captive cats route); rescue, mercy, field yield and surrender never do. Between the top block and the reason:
    OFERTA | the flat slider (TotalSparring's, ported as `flat_slider.cpp`) | the offer in amber; the min and max under
    the slider's ends; one line under that: "Probabilidad de aceptar: N%" (green from 70, amber 30-69, red under 30),
    or the shortfall for the offer, or "Se ofendio: hoy no regatea". The slider runs from `haggleMinFraction` of the
    ask to the ask in steps of 10 and starts at the ask. The purse is checked against the offer: short, TIENES is red
    and the amber button is hidden. Below the ask it reads OFRECER (one attempt on the day's seed), at the ask ACEPTAR.
    An offended NPC shows the raised price and the offended line with no slider. The section has a fixed height, so a
    drag never resizes the window: only texts, colours and the button row change. With the slider, the purse
    shortfall goes in the slider's line, never on the reason plate.

### Cage commands (section 2, "Menu on your prisoner")

17. **Golpear and Noquear beside Hablar** (user request 2026-10-03). For a prisoner awake in the player's own cage
    with `allowBeating` on, the block beside the context menu is `[Hablar, Golpear, Noquear]`, top to bottom, one
    `PlaceButtonBlock` call with the shared 160 x 44 style and 2 px between buttons; anyone else keeps `[Hablar]`.
    Golpear is one direct, capped blow from the selected character (`fix/cage-beat-direct-hit`: its own attack damage
    as blunt; the prisoner stays caged and is never killed); Noquear is Kenshi's own order (`STEALTH_KNOCKOUT`) given to
    the selected character, once: when the prisoner is down the character is let go (prisoner-module-fixes F1).
    Noquear shows the game's own odds ("Noquear 72%", `getPlayerTaskProbability`), plain "Noquear" when there are none.
    Labels are title case like Hablar (`cage.button.*` in `messages.json`). Each button arms its own press-and-release
    gesture.
    **Golpear and Noquear rest** (tab "Menu on your prisoner, Golpear and Noquear resting"; R2.3,
    prisoner-module-fixes F6, cage-beating-stress S1). After each blow on a prisoner, Golpear rests for
    `submissionBeatingCooldownHours` game hours (Mod Hub "Sumision: horas entre golpizas"); after a knockout order
    that put the prisoner down, Noquear rests for `submissionKnockoutCooldownHours` ("Sumision: horas entre noqueos").
    Both 1 h by default, per prisoner, session-only like the defeat episodes (`CageCooldownLedger`). The buttons stay
    in the block and carry the hours left, rounded up: "Golpear (1 h)" (`RestLabel`), "Noquear (1 h)" in place of the
    odds (`KnockoutCaption`). A click only tells the wait ("Podrás volver a golpearlo en 1 h", `cage.beatResting`;
    "Podrás volver a noquearlo en 1 h", `cage.knockoutResting`). Every click re-reads the facts the block was built
    on: a prisoner freed, knocked out or out of the cage since says "Ya no puedes hacerle eso" (`cage.notAllowed`).
    Neither command's knockout counts as a defeat any more: each blow and each knockout adds beating stress (the
    card's Golpes).
    **Two native columns** (tab "Menu on your prisoner, two columns"; prisoner-module-fixes F4). A character inside a
    building opens two columns side by side: the character (`ContextMenu::menuGUI`) and the building (`menuGUI2`, e.g.
    "Jaula de Prisionero": ABRIR CON LLAVE, DESMANTELAR, PONER EN LIBERTAD). The block is placed beside the rectangle
    that holds both visible columns (`menu_anchor.cpp` `UnionMenuRect` = `menuAnchorUnion` in `js/readout.js`), so it
    never covers the second one; the button width stays the first column's (`MakeBlockStyle`). With one column the
    rectangle is that column and nothing moves.
18. **The same commands in the popup** (user report 2026-10-03: a caged, defiant Holy Warden got only CERRAR). Any
    refusal from a prisoner the block above would offer Golpear and Noquear for (one predicate,
    `CageCommandsAllowed`) carries GOLPEAR and NOQUEAR (the game's odds, `KnockoutLabel`) beside the remedy and
    CERRAR; same orders, same selected actor, the same "Selecciona a alguien" line; the window closes. A yes keeps
    ACEPTAR / CANCELAR only, and the haggle slider owns its row, so neither carries them. Beating off: no buttons,
    and a defiant prisoner reads "Sigue desafiante: dejalo con hambre" (`popup.reason.defiantNoBeat`). The old
    GOLPEARLO remedy is gone; GOLPEAR and NOQUEAR rest exactly as in the block above ("GOLPEAR (1 h)", "NOQUEAR
    (1 h)", state "Caged and defiant, GOLPEAR and NOQUEAR resting"). Someone who cannot leave (caged, shackled, carried) never gets IGNORAR
    (mercy) or PERDONAR (submission). States "Caged and defiant", "Caged and hungry", "Mercy plea from someone who
    cannot leave".

### Submission event (section 5, "Submitted NPC asks to join")

19. **A beaten NPC asks; the window is the same popup** (user request 2026-10-03: "it should walk up to me
    automatically and ask to join ... Accept, Forgive, Reject"). Nothing new is drawn: `BuildSubmissionOfferView`
    fills the recruit popup with the Sometido tag, the kneeling line ("se arrodilla y te pide unirse a tu grupo"),
    PRECIO Gratis and three buttons in the usual order and styles -- ACEPTAR (amber plate), PERDONAR (amber edge,
    the extra slot), RECHAZAR (line plate, the cancel slot). No chance row: the roll was already made when the NPC
    set off. The X is not RECHAZAR: closing without an answer leaves the NPC calm in place for the forgive window.
    The popup opens only when the NPC has arrived and no other window is up; while one is, the NPC stands and waits.

### Head notice (section 2, "Someone wants to talk" / "Select the one who called")

20. **Who wants to talk is shown over their head** (user request 2026-10-03: "the game tells you, but if you don't
    read the name you don't know who wants to talk"). Whoever calls out -- a ready captive (`hail.cpp`, "Quiere hablar
    contigo") or a beaten NPC walking up (`submission_director.cpp`, "Se acerca para hablar contigo") -- gets a compact
    card over the head: the readout card's lacquer plate and frame, a 3 px amber edge on the left, the name in brass
    over a gold rule and the line in amber (`.hn` in `css/readout.css` = the constants of `head_notice.cpp`; width =
    the wider text, 120..264 px). Same anchor and `PlaceCard` as the readout card, `ToolTip` layer, no mouse focus.
    It stays `headNoticeSeconds` (default 90, real seconds frozen on pause) when it asks the player to act (these
    two, and a helper's offer), or `headNoticeInfoSeconds` (default 25) for a one-line informational notice (a
    temporary member's or a helper's lines), and goes by itself; selecting that NPC
    dismisses it for good and the normal card takes the head, so the two never overlap. Selecting someone else or
    clicking elsewhere never dismisses it; if the normal card about another NPC would cover it, the card wins and the
    notice hides that frame. Up to 8 at once, one per NPC. Hidden off screen and under a right-click menu; dropped when
    the NPC dies or joins, when the submission window opens or the event ends, and at a load.

### Balanced allies by level (section 5, the "Ally ..." states)

21. **The tier says what the ally wants** (user request 2026-10-03: allies by level, with friendship discounts).
    Nothing new is drawn: the same popup, with the body naming the tier (`popup.ally.*`). A paying ally reads
    "Aliado (nivel 34): Gor te cobra 1.840 cats, con 14% de descuento por amistad." and PRECIO carries the
    discount beside the price, two spaces after it, in the same amber ("1.840 cats  -14%",
    `popup.ally.discountTag`); the slider, the purse check and the offended markup work on the discounted
    price because the discount is already in it. A Good ally under 20 reads "es tu aliado (nivel 12) y se une
    gratis por amistad" with the odds of today's roll. An 80+ Good or Neutral ally close in level reads "te
    acompanara 6 dias como miembro temporal", Gratis. The tier refusals (leader too weak, cruel, greedy) are
    ordinary refusals with their reason in red. The card shows "Aliado: 1.840 cats con descuento"
    (`readout.line.ally.paidJoin`) instead of the plain price. States "Ally pays", "Ally comes for a few days",
    "Ally refuses: leader too weak", "Good ally under 20 joins free".

### Temporary members (section 2, "Temporary member (squad notices)")

22. **A temporary member says what is going on, over their head** (ally-tiers-temp-helpers T2). The same
    head notice (`.hn`), raised as a squad notice (`RaiseSquadHeadNotice`): it stays while they are in the squad,
    where an ordinary notice goes the moment its NPC joins, and selecting them never dismisses it. Lines
    (`tempMember.notice.*`): "Te acompanare 6 dias" on joining, "Me quedan 4 dias" once per game day after the
    join day ("Me queda 1 dia" on the last; `daily` / `dailyOne`, skipped the day "Me ire pronto" goes up,
    `tempMemberDailyNotice`), "No me trates asi" on a strike (hunger: "Tengo
    hambre... no me trates asi"), "Si sigues asi me voy" on the strike before the last, "Me ire pronto" before the
    time is up, and after they left the squad "Me voy. Fue un honor" or "Me largo" (an ordinary notice by then).
    All of these are informational: they stay `headNoticeInfoSeconds` (default 25 s), not `headNoticeSeconds`.
    Selecting a temporary member shows their card with one line, "Temporal: le quedan 4 dias"
    (`readout.line.temp.daysLeft`), and nothing else. Section 2, tab "Temporary member (squad notices)".

### Free helper (section 5, "Free helper offers to escort you"; section 2, "Free helper ...")

23. **A stronger ally offers to escort the player, for free, without taking orders** (ally-tiers-temp-helpers
    T3, user request 2026-10-03: "higher-level allied NPCs with good personalities offer to help you for some
    days, but you can't control them"). Nothing new is drawn. Walking up (the submission walk-up), the ally gets
    the head notice "Quiere ofrecerte ayuda" (`headNotice.helperOffer`); on arrival the same popup opens with the
    tag **Ayuda** (`popup.route.helper`), the body "Beep el Sabio te ofrece acompanarte 4 dias, gratis. No podras
    darle ordenes." (`popup.body.helper`), PRECIO Gratis and two buttons -- ACEPTAR (amber plate) and RECHAZAR
    (line plate, the cancel slot). No chance row. The X is no answer. During the escort the helper raises ordinary
    notices (`helper.notice.*`): "Te cubro las espaldas" on accepting, "Me ire pronto" before the end, "Hasta
    aqui llego. Suerte" or "No vuelvo a ayudarte" on leaving, "Sigo contigo" when found again after a load.
    The offer stays `headNoticeSeconds` (default 90 s); the escort's lines are informational and stay
    `headNoticeInfoSeconds` (default 25 s).
    Selecting a helper shows their card with one line, "Ayudante: le quedan 3 dias"
    (`readout.line.helper.daysLeft`), and nothing else.

### Duel challenge (section 5, "Ally 80+ can be challenged ..." and "Cruel ally 80+ ..."; section 1, "Ally 80+ you can challenge")

25. **An 80+ ally can be challenged to a TotalSparring duel and joins for good on a win** (ts-duel-bridge,
    user request 2026-10-03: "challenge him to a duel so he joins you; TotalSparring already has the duel logic").
    TotalRecruit draws one new button and one new info row, nothing else; the fight itself is TotalSparring's own
    Dueling window. When `CanOfferDuel` holds (duel challenge on, TotalSparring loaded and ready, a Balanced tiered
    ally at `allyTierEliteFromLevel` or above, no lost-duel cooldown), the popup keeps the tier's own answer
    (ACEPTAR as a temporary member, the greedy's price, or the cruel / too-weak refusal) and adds **RETAR A DUELO**
    to the row, after the extra slot: a sumi plate with an amber border, like the other extra answers. The info
    box gets a fourth row, **DUELOS**, with TotalSparring's head-to-head record of the selected squad member
    against this NPC in amber ("2-1"; draws as a third number, "0-3-1"), only when TotalSparring could read it.
    The card of the same NPC shows "Duelos: 2-1" as its second line (`readout.line.duel.record`). Decision: the
    record is a row of the info box, not a sentence in the body, because it is a number the player compares
    like the price and the odds; the button sits beside the tier's answer instead of replacing it, so the duel
    is an extra way in and never takes the temporary stay or the paid deal away. Without TotalSparring nothing
    changes. The outcome is told with head notices (`duel.notice.*`): "Me ganaste. Me uno a ti" over the new
    member (a squad notice), "Vuelve cuando seas mas fuerte" over the winner.

26. **A won duel is a join chance in Balanced; Cheat offers the duel too and a win always recruits**
    (duel-join-chance, user design 2026-10-04). Balanced: `min(100, duelJoinBaseChance + duelJoinChancePerWin *
    (wins - 1))`, wins = TotalSparring's head-to-head wins against this NPC with the new one counted (25, 45, 65,
    85, 100 by default); the roll is seeded by the NPC and the win count, so a reload never rerolls it. The popup
    gets a fifth info row, **SI LE GANAS**, with what the next win does ("65% de que se una"; Cheat "se une"), drawn
    only when the record was read (Cheat: always). The card's record line carries it: "Duelos: 2-1, si ganas 65% se
    une" (`readout.line.duel.recordChance`; with draws "Duelos: 0-3-1, si ganas 25%", `recordDrawsChance`; Cheat
    "Duelos: 1-2, si ganas se une", `recordSure` / `recordDrawsSure`). Decision: the chance rides on the record line
    instead of a third card line, because the card has two lines and the chance is meaningless without the record.
    A win that does not roll the join raises an informational notice "Buena pelea. Retame otra vez"
    (`duel.notice.stays`) and hides the button for a seeded `duelWinCooldownMinHours` .. `MaxHoursUnder100`
    (level 80-99, 1-3 days) or .. `MaxHours` (100+, 1-5 days). Cheat (state "Cheat: an 80+ NPC ...") gates the
    duel on the level alone (`SafeNpcLevel` >= `allyTierEliteFromLevel`, a person, not an animal).

27. **A free animal can be challenged, beaten into yielding, and in Cheat healed into joining**
    (animal-duel-submission, user request 2026-10-04). A free animal (no owner: `BelongsToSomeone` outside the
    player's cage; `exclude.animals` still wins) of `duelAnimalsFromLevel` (40) or above gets RETAR A DUELO in both
    presets (`duelAnimalsEnabled`), with the same record line and SI LE GANAS row as a person (states "Wild animal
    40+ ..."). A won duel is the same chance in Balanced and a sure join in Cheat; the outcome is a narrator line
    (`duel.animal.*`) with no head notice, because an animal never speaks. In Balanced two defeats in the open open
    the field-yield roll (temperament Normal, no loyalty penalty) and the card shows DERROTAS n/2 next to the
    stomach bar (states "Wild animal beaten ..."); a failed roll keeps food as the other way in. In Cheat a meal, a
    cure or one knockout by the squad is the gesture ("Comio de tu mano / Lo curaste / Lo venciste: te sigue").

28. **In Cheat any cage counts** (cheat-any-cage, user request 2026-10-05: "for cheat mode drop the limit of only
    recruiting from your cell"). With `cheatAnyCage` on (Mod Hub "Cheat: cualquier jaula", last `HUB` row) a
    prisoner in another faction's cage, shackles or carried off (`SafeInPrison`) gets the same one-minute wait and
    joins, on the cage route. The cage commands (Golpear / Noquear) stay on the player's own cages. Balanced is
    unchanged. Update (duel-any-source U5, user report): "metelo en una de tus jaulas" read wrong with any cage, so
    with `cheatAnyCage` on the refusal is `Reason_CheatNeedsAnyCage` (`cheat.needsAnyCage`: "metelo en una jaula y
    hablara enseguida", ...; state "Cheat: any person can be challenged ..." shows it); off, `cheat.needsCage` as
    before. The card line stays "Metelo en una jaula".

29. **Cheat offers the duel to a person of any level, and a TotalSparring duel counts too** (duel-any-source, user
    report 2026-10-05: "a los honorables no me deja ... retarlos a un duelo"). In Cheat a person gets RETAR A DUELO
    from `duelCheatPeopleFromLevel` (Mod Hub "Duelo Cheat: nivel minimo de personas", 1) instead of Balanced's 80
    (state "Cheat: any person can be challenged ..." now shows a level 33 NPC). With `duelCountExternalBouts` on
    (Mod Hub "Duelo: contar duelos de TotalSparring") a duel the player opens from TotalSparring's own window
    against someone outside the squad who could be challenged counts like RETAR A DUELO: a win runs the duel join
    (and the proactive offer), a loss the cooldown. No new card or popup state: the mockup gains the two `HUB`
    rows (appended at the end) and the updated "Duelo: retar NPC 80+" description.

30. **A duel can require several wins before it recruits** (duel-any-source U4). `duelMinWinsCheat` /
    `duelMinWinsBalanced` (Mod Hub "Duelo Cheat / Balanced: victorias minimas", 1 by default = today) hold off the
    join until that many head-to-head wins: an earlier win rolls nothing and starts no cooldown, and the card's
    record line counts the wins still missing (state "Ally you can challenge, under the minimum duel wins":
    "Duelos: 1-0, faltan 3 victorias"). From the minimum on, Cheat joins and Balanced rolls the usual curve,
    counted from the first win. Decision: the popup SI LE GANAS row reads "0% de que se una" while wins are
    missing (no new popup text key).

### Native windows (sections 2 and 5: the card and the head notices)

24. **The card and the notices step aside for Kenshi's own dialogue** (user report 2026-10-03: "if I talk to the
    NPC and a dialogue comes up, the tooltip doesn't hide and covers the dialogue"). Both overlays sit on the
    `ToolTip` layer, above the native dialogue window, over a head in the middle of the screen. While a conversation
    (`ForgottenGUI::inDialogue` or the `DialogueWindow` visible) or a modal message box is open, the readout card --
    pinned by Hablar too -- and every head notice are hidden, not dismissed, and come back on the first frame after
    it closes (`overlay_visibility_rules.h`, one rule for both). Nothing new is drawn, so the mockup has no new
    state. Trade and inventory windows are not covered: the engine only says "some inventory is open", which is
    also true for the player's own bag.

## Checking a change

Mockup first, then C++. Measure in Chrome (`getBoundingClientRect`) and copy the px into the constants.
