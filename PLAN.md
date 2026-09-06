# TotalRecruit — Implementation Plan

A non-cheat universal recruitment mod for Kenshi, built as a native RE_Kenshi
C++ plugin. Target: **Kenshi 1.0.65 x64 (Steam)**.

Every engine API cited here was verified against the KenshiLib headers in
`../KenshiLib_Examples_deps/kenshilib/Include/kenshi/`. Nothing is assumed.

---

## 1. Why this mod exists

Every popular recruitment mod (Recruitable Prisoners, TameBeasties, Recruit
Anyone) works by attaching an FCS dialogue package to each NPC. That package
only exists on NPCs that shipped with the mod, so **any character added by
another mod cannot be recruited** — hence the endless patch treadmill.

A C++ plugin does not ask *"does this NPC have the dialogue installed?"*. It
asks *"is this a Character?"*. Universal coverage by construction, including
mods that do not exist yet.

That is the only reason to build this.

---

## 2. Design decisions (settled)

| Question | Decision | Reason |
|---|---|---|
| Dice or meter? | **Neither** — stateless evaluation at click time | No save-scumming, no parallel save file, no progress lost on load |
| Central gesture | **Two routes**: rescue and captivity | Rescue is the one thing no existing mod has |
| Coexist with Recruitable Prisoners? | **Coexist, don't negotiate** | Right-click overlay; anchor beside the menu, not below |
| Animals | Same engine, different numbers, **humanoids ship first** | `isAnimal()` splits the table, not the system |
| Faction relations | **Modifier only, Vanilla+ preset only** | Faction is the one thing the player cannot change; as a gate it makes half the population unrecruitable — exactly what this mod exists to fix |
| Confirmation prompt | **Native FCS dialogue**, not a custom modal | The engine's own dialogue window has branches, choices and voice |

### The three routes

**A — Rescue.** You found them bleeding out and chose to save them. No cage, no
price, no negotiation. This is the only route in Kenshi where the player is not
exercising power, and it fits the fiction better than anything else: whoever
pulled you out of the dirt is your people. Animals live almost entirely here.

**B — Captivity.** They are in your cage. The price varies by who they are —
a bandit wants cats, a starving wanderer wants food, a slave wants freeing, a
soldier wants to be beaten first.

**C — Surrender.** Optional, last. See §7 for why it is deferred.

### The three presets

- **Cheat total** — everything recruitable, click and done (today's behaviour)
- **Balanced** — the routes with their prices
- **Vanilla+** — same, plus faction relation as a modifier

One engine, three INI tables. Presets are numbers, not code paths.

---

## 3. What the engine gives us — all verified

### 3.1 Per-NPC memory — the engine already remembers

This is the keystone. It removes any need for plugin-side persistence.

```cpp
enum CharacterPerceptionTags_LongTerm {   // Character.h:87-101
    LT_MY_LIFESAVER,            // route A
    LT_MY_CAPTOR,               // route B
    LT_FREED_ME,                // freed slave
    LT_DEFEATED_MY_SQUAD_ONCE,  // submission (Fenix-tier)
    LT_KILLED_MY_FRIEND,        // the honest "no"
    LT_SQUAD_LOST_TO_ME_ONCE, LT_MY_INTRUDER, LT_STOLE_FROM_ME,
    LT_FRIENDLY_AQUAINTANCE, LT_I_SCREWED_THIS_GUY
};

void rememberCharacter(Character* who, CharacterPerceptionTags_LongTerm);  // :548-551
bool getCharacterMemoryTag(Character* who, CharacterPerceptionTags_LongTerm); // :553
bool  haveMetBefore(Character* h);        // :553
float lastSeenInHoursAgo(Character* c);   // :552
```

These persist in the save file. The four relationships this mod is about are
already engine state.

Bonus — the engine already has opinions (Character.h:400-403):
`shouldIHelpThisGuy()`, `ILoveThisGuyBecauseOfStuffThatHappened()`,
`IHateThisGuyBecauseOfStuffThatHappened()`.

### 3.2 Personality and archetype — free, and mod-proof

```cpp
Character::getStateBroadcast()            // Character.h:225
  -> PersonalityTags myPersonality;       // StateBroadcastData.h:62
  -> CharacterTypeEnum NPCType;           // StateBroadcastData.h:63
  -> CharacterTypeEnum looksLike_clothingArchetype;  // :80
  -> SlaveStateEnum _slaveState;          // :33
  -> Faction* isSlaveOf;                  // :65
  -> bool isEscapedPrisoner, isKidnapped; // :66-67
```

**Personalities** (StateBroadcastData.h:11-29): `PT_HONORABLE`, `PT_TRAITOROUS`,
`PT_SMART`, `PT_DUMB`, `PT_BRAVE`, `PT_FEARFUL`, `PT_WARM_KIND`,
`PT_COLD_CRUEL`, `PT_NORMAL`, `PT_MANIACAL`. (Four unnamed RE gaps at ordinals
3, 4, 7, 8.) These match the vanilla FCS personality tags.

**Archetypes** (Enums.h:254-267): `OT_BANDIT`, `OT_SLAVE`, `OT_MILITARY`,
`OT_TRADER`, `OT_CIVILIAN`, `OT_ADVENTURER`, `OT_LAW_ENFORCEMENT`, `OT_SLAVER`,
`OT_DIPLOMAT`.

The engine assigns these, so **NPCs from mods that do not exist yet still have
them**. This is what kills the patch treadmill a second time.

Design note worth using: `NPCType` is what the NPC **is**;
`looksLike_clothingArchetype` is what it **appears to be**. They diverge on
purpose (disguises, stolen armour). A bandit dressed as Holy Nation is a whole
story, for free.

Finer temperament, if needed (CharStats.h:167-171): `warriorSpirit`,
`aggression`, `goodness`, `evilness` — note `goodness` and `evilness` are
**separate axes**, not one scale.

### 3.3 Dying vs unconscious — route A's trigger

```cpp
bool MedicalSystem::isProbablyDying();              // MedicalSystem.h:262  <- THE predicate
bool Character::isLiterallyUnconciousNotPretending();  // Character.h:468  <- MANDATORY
```

`PS_PLAYING_DEAD` exists (ProneState, Character.h:53-60), so NPCs fake death.
Without the second check the player farms recruits off pretenders.

Supporting reads: `isInBloodlossTrauma()` (:270), `pointOfNoReturn()` (:213),
`pointOfNoReturn_Hunger01()` (:214), `isHungerKO()` (:215), `isReallyHungry()`
(:259), `getOverallHealthRating()` (:264), `float blood` (0x70), `float hunger`
(0x60). Raw flags are contiguous: `crippled` 0x160, `unconcious` 0x161,
`sub50KO` 0x162, `bloodlossTrauma` 0x163, `dead` 0x164.

**Do not use `CollapseStage` for this.** It is a per-bodypart ragdoll severity
classifier (`COLLAPSE_NONE` / `COLLAPSE_BUT_NO_RAGDOLL` / `COLLAPSE_KO`); its
top value is KO, not death.

### 3.4 Healing attribution — "only if the player saved them"

```cpp
bool applyFirstAid (float skill, Item* eq, float dt, Character* who); // MedicalSystem.h:250
bool applyDoctoring(float skill, Item* eq, float dt, Character* who); // :251
bool applyRigging  (float skill, Item* eq, float dt);                 // :252  <- NO `who`
```

`who` is the healer. Robot jury-rigging carries **no** attribution through
`applyRigging`; for skeletons use `CharStats::xpFirstAid(Character* patient,
float, StatsEnumerated)` (CharStats.h:213), which carries both sides.

Cleanest signal of all — paired events (Enums.h:736-740):
`EV_HEALING_OTHER_START`, `EV_BEING_HEALED_START`, `EV_HEALING_OTHER_FINISHED`,
`EV_BEING_HEALED_FINISHED`, `EV_FIRSTAID_KIT_EMPTY`.

`selfHealing` (HealthPartStatus 0x30) is a per-part **capability** flag, not a
per-tick record of who healed. Attribution must come from the call sites above.

Note: `FIRST_AIDED_US` is a faction-relations event (FactionRelations.h:49) —
the engine already treats healing someone as a diplomatic act.

### 3.5 Captivity — "is this my cage?"

The brainstorm doc listed this as the open technical unknown. It is two hops:

```cpp
character->inSomething == IN_PRISON            // Character.h:129-134, :595
character->inWhat.getBuilding()->getFaction()  // util/hand.h, RootObjectBase.h:52
```

`setPrisonMode(bool on, UseableStuff* h)` (Character.h:597) confirms the engine
stores the cage as that second argument.

Slavery already has the explicit link the doc thought was missing:
`slaveOwner` (0x328, :600), `getMySlaveOwner()` (:605), `isMySlave()` (:608),
`isMyFactionsSlave()` (:609), plus `Ownerships::slaves` — a `std::set<hand>` on
the Platoon with its `Faction*` beside it (Platoon.h:49-93).

Cages need no name matching: `BD_PRISON` / `BD_SLAVE_STORAGE` are engine
building designations (Platoon.h:40-43), and `BF_CAGE` is the furniture
function.

**Prior art to build on:** `Shutori_Re_Kenshi_Source/.../PrisonerUtil.cpp`
(1359 lines) already implements cage discovery via `BF_CAGE`, occupancy via
`inSomething == IN_PRISON`, and player-ownership filtering.

Unlocking, from `KenshiExtensionPlugin/kep-mod-tool/DevToolsPanel.cpp:398-421`:
```cpp
building->getDoorLock()->locked = false;
character->getChainedModeShackles()->lock->locked = false;
character->dialogue->sendEvent(target, EV_UNLOCK_MY_CAGE_OR_SHACKLES);
```
That third line is what makes the game react properly instead of leaving the
NPC in a strange state.

### 3.6 Dialogue — the plugin/FCS split

**This is the architecture that removes the need for a custom modal.**

Firing an FCS-authored dialogue (reference implementation:
`KenshiExtensionPlugin/kep-mod-tool/PlatoonTool.cpp:266-323`):

```cpp
GameData* dlg = ou->gamedata.getData("<fcs-string-id>", DIALOGUE);
if (DialogDataManager::getData(dlg) == NULL) return;   // sanity check
DialogChoiceList list;
list.add(dlg, NULL);
DialogLineData* line = npc->dialogue->_chooseDialog(&list, playerChar, false);
npc->dialogue->startPlayerConversation(playerChar, line);   // opens the choice UI
```

Reading back what the player chose — hook `Dialogue::_doActions`:

```cpp
void Dialogue::_doActions(DialogLineData* dialogLine);  // Dialogue.h:404, RVA 0x67FDE0
```

It fires for every executed line, including the player's chosen reply. Inside:
`dialogLine->getStringID()` (the FCS ID), `getGameData()->objectReferences`
(custom fields the mod author defines in FCS), `thisptr->me` (the NPC),
`thisptr->getConversationTarget()` (the player).

Template to copy: `KenshiLib_Examples/Dialogue/Dialogue.cpp:429-450`.

**Universal coverage is preserved** because the plugin attaches the package at
runtime rather than relying on it being pre-installed:
```cpp
void Dialogue::addDialoguePackage(GameData* _con);   // Dialogue.h:345
bool Dialogue::hasDialogueEvent(EventTriggerEnum e); // :365  <- check before firing
```
KEP does exactly this in `CharacterExtension.cpp:135-146`.

**Triggers we need already exist** (Enums.h:716-793):
`EV_PLAYER_TALK_TO_ME`, `EV_GET_UP_PEACE` (route A's "when they get up"),
`EV_BEING_HEALED_FINISHED`, `EV_PRISONER_FREE_TO_GO`,
`EV_BOUGHT_ME_FROM_SLAVERY`, `EV_UNLOCK_MY_CAGE_OR_SHACKLES`.

**Actions we need already exist** (Dialogue.h:31-93):
`DA_JOIN_SQUAD_FAST`, `DA_JOIN_SQUAD_WITH_EDIT`, `DA_TAKE_MONEY`,
`DA_FREE_TARGET_SLAVE`, `DA_REMOVE_SLAVE_STATUS`, `DA_REMEMBER_CHARACTER`
(writes the `LT_` tags from dialogue), `CHOOSE_RECRUITING`,
`CHOOSE_PRISONER_BAIL`, `CHOOSE_SLAVES_BUYING`.

**Conditions read live state** (Enums.h:796-879): `DC_IS_IMPRISONED`,
`DC_IMPRISONED_BY_TARGET`, `DC_PERSONALITY_TAG`, `DC_IS_SLAVE`,
`DC_IS_IN_LOCKED_CAGE`, `DC_STARVING`, `DC_IS_KO`, `DC_MET_TARGET_BEFORE`,
`DC_I_LOVE_THIS_GUY`, `DC_IS_RECRUITABLE`.

So the work splits cleanly:

| Side | Owns |
|---|---|
| **FCS (mod author)** | Text, branches, player choices, and the actions |
| **Plugin (C++)** | When to fire, which dialogue, for whom — reading `NPCType`, `myPersonality`, `isProbablyDying()` |

### 3.7 Classification, most specific first

1. `character->data->stringID` (GameData.h:92 via RootObjectBase.h:76) — exact FCS record
2. Faction + `Faction::ranks` (Faction.h:62) — **this is what makes a Paladin a Paladin**
3. `getStateBroadcast()->NPCType` — coarse bucket
4. `Faction::getFundamentalNPCType()` (Faction.h:65) — fallback when `OT_NONE`
5. `getRace()` → `RaceData::data->stringID`, `raceGroup` (RaceData.h:60)
6. `myPersonality` + CharStats scalars — flavour

Also useful: `isUnique()` (Character.h:202) for Fenix-tier rules,
`getRoughLevel()` (:404) for a crude power tier.

---

## 4. The price table (route B)

Not a difficulty system — **a pricing system where each NPC charges in a
different currency.** This is what unifies the three reference mods:
Recruitable Prisoners is the "condition" price, Recruit for Cats is the "money"
price, TameBeasties is the "food" price. One engine, three currencies.

| Who | Charges in | Read from |
|---|---|---|
| `OT_CIVILIAN`, starving | Food, healing | `hunger`, `getOverallHealthRating()` |
| `OT_BANDIT` / `OT_TRADER` | Cats | `takeMoney()` (Character.h:208-211) |
| `OT_SLAVE` | Freeing them | `_slaveState`, `LT_FREED_ME` |
| `OT_MILITARY` / `PT_HONORABLE` | Submission + time | `LT_DEFEATED_MY_SQUAD_ONCE` |
| `PT_FEARFUL` | Cheaper | modifier |
| `PT_BRAVE` / `PT_COLD_CRUEL` | Dearer | modifier |
| `isUnique()` (Fenix) | Hard submission | + per-name INI rule |
| Animal, wounded | Food + healing | fast — route A |
| Animal, healthy | Food + time | slower |

Every one is recruitable. None is impossible. What changes is the **currency**,
not the door.

`LT_KILLED_MY_FRIEND` is the one honest hard "no" — a reason the player
understands and could have avoided.

---

## 5. Implementation phases

### Phase 0 — Project base *(DONE)*
- [x] Copy `UnlimitedRecruitV2` → `TotalRecruit`
- [x] Rename namespace `urv2` → `totalrecruit`, macros `URV2_*` → `TR_*`
- [x] Rename widget names `URV2_*` → `TR_*` (**anti-collision: both mods can be installed together**)
- [x] `RE_Kenshi.json` → `TotalRecruit.dll`; valid 46-byte `.mod` header
- [x] Builds Release|x64 clean
- [x] Exports verified mangled: `?startPlugin@@YAXXZ` / `?stopPlugin@@YAXXZ`

Inherited intact: context-menu overlay (two hooks, `0xF8` offset, side
anchoring), SEH discipline, `CharacterFromTarget`, `Recruit()`.

### Phase 1 — Smoke test (do this before anything else)
A minimal build that logs, on right-click, for any NPC:
`NPCType`, `myPersonality`, `isProbablyDying()`, `isLiterallyUnconciousNotPretending()`,
`inSomething`, `isUnique()`, plus whether an FCS test dialogue fires.

**Why first:** everything above is verified against headers, but nothing is
verified *running*. This validates the pillars in ~30 minutes instead of after
building on top of them.

### Phase 2 — Rule engine
Replace the body of `IsRecruitable()` (it already returns a text reason, which
is the exact shape the feedback needs). Read via `getStateBroadcast()`.
Feedback through the status line + `ou->showPlayerAMessage(text, true)`.

### Phase 3 — Dialogue bridge
`addDialoguePackage` + `startPlayerConversation` + hook `_doActions`.
**Unblocks phases 4 and 5**, so it comes before them.

### Phase 4 — Route A (rescue)
`isProbablyDying()` + `isLiterallyUnconciousNotPretending()`; attribution via
`applyFirstAid`/`applyDoctoring`; write `LT_MY_LIFESAVER`; fire the dialogue on
`EV_GET_UP_PEACE` — **when they get up, not when healing ends** (user decision).

Because `LT_MY_LIFESAVER` persists, the debt survives if the player walks away;
the conversation can happen at the next meeting rather than being lost.

### Phase 5 — Route B (captivity)
Build on `PrisonerUtil.cpp`. Release the KEP way, including
`EV_UNLOCK_MY_CAGE_OR_SHACKLES`.

### Phase 6 — Settings panel
Native `DatapanelGUI` — **no KEP dependency needed**:
```cpp
ForgottenGUI::createDatapanel(top, left, w, h, scrolls, layer, window);  // gui/ForgottenGUI.h:69
setLineCheckbox(caption, bool* val, category);                  // gui/DatapanelGUI.h:87
setLineSliderEditable(key, cat, showName, min, max, float* v);  // :81
setLineDropBox(key, cat, int* valPtr, goBox, width);            // :88
setLineTextEditable(...); setLineTextButton(...);               // :83, :78
showTabs(bool); addTab(id, name, icon); clearPage(cat);         // :53-56, :41
```
Widgets write **through the pointer** — no read-back callback. Reference usage:
`KenshiExtensionPlugin/kep-mod-tool/DevToolsPanel.cpp:119-247` and `:651-667`.

Also replace the config: today's is a flat read-only JSON scanner. Use the
layered INI with the `__MISSING_KEY__` sentinel
(`RE_Kenshi_Plugins_Source/06_MultiXPTraining_Source`, `:122-127`, `:152-209`).

Use `InputHandler::addCommand` (`kep-core/src/config_manager.cpp:43-49`) for a
rebindable key in the vanilla Controls screen instead of a hardcoded keycode.

### Phase 7 — Animals
Same route A, different numbers. `isAnimal()` (Character.h:204) splits the table.

### Phase 8 — Route C (surrender), optional
See §7.

---

## 6. Known traps (from KENSHI-MODDING-NOTES.md and this research)

1. **Characters are state-locked during a conversation.** Call
   `dialogue->endDialogue(true)` first, mutate the world on the next frame.
   Confirmed by `KenshiLua/examples/.../gambling_crooked.lua`.
2. **Custom dialogue-condition enums must be in 1100–55000** to avoid clashing
   with other mods (`KenshiLib_Examples/Dialogue/Dialogue.cpp:16`).
3. **`lektor<T>` out-params must be freed manually** — `free(x.stuff)`.
4. **C2712**: every SEH guard lives in its own POD-only frame. MSVC counts every
   C++ object in the whole function, including MyGUI's implicit temporaries.
5. **Entry points must NOT be `extern "C"`** — RE_Kenshi resolves the mangled
   name. Verify with `dumpbin /EXPORTS`.
6. **Kenshi locks the DLL while running.** Verify the deployed file by size and
   md5 against the build output.
7. **Anchor the overlay beside the menu, not below** — LimbRemoval and
   Loot-Scoot-Execute already claim the space underneath.
8. **`pacakgesIHave` is misspelled in the engine** (Dialogue.h:343).
9. `lastTimeSaid = -99` resets a dialogue cooldown if one refuses to re-fire
   (`PlatoonTool.cpp:369-379`).

---

## 7. Route C (surrender) — implemented, and the concern was unfounded

`TARGET_IS_SURRENDERED` exists in the `StateType` enum (Enums.h:1141) but has no
named predicate in the SDK. Its enum neighbours are right there in `AI/AI.h` —
`targetIsUnarmed` (:380), `_targetLooksLikeASlave` (:381) — and the one in the
middle is simply absent.

This was deferred on the assumption that reaching it meant calling through the
static `AI::stateFuncs` at RVA `0x1E43450`, which the field notes warn against.
**That assumption was wrong.**

`AI::stateIsTrue` is an ordinary method that takes a `TaskStateData`
(AI/AI.h:152-153), and `TaskStateData` is a plain POD with an exported
constructor (Tasker.h:22-33). Both symbols are in `KenshiLib.lib`:

```
?_NV_stateIsTrue@AI@@QEAA_NAEBVTaskStateData@@AEBVhand@@AEBVVector3@Ogre@@@Z
??0TaskStateData@@QEAA@W4StateType@@_N1@Z
```

So the engine's own state machine answers the question through the linker, with
**no hardcoded address at all**:

```cpp
const TaskStateData state(TARGET_IS_SURRENDERED, true, false);
ai->_NV_stateIsTrue(state, character->getHandle(), character->getPosition());
```

`_NV_` rather than the virtual, because `GetRealAddress` cannot resolve virtuals
and the non-virtual thunk is what the import library exports.

A composed fallback covers the case where the engine cannot answer: the player
must have beaten them (`lastGuyWhoDefeatedMe` or `LT_DEFEATED_MY_SQUAD_ONCE`),
and they must be in no state to fight on (`isDown`, `isFleeing`, or dying).

**One more SDK duplicate found doing this:** `AI/AI.h` cannot be included
alongside `Character.h` — both define `CharacterMessage` (Character.h:110 vs
AI/AI.h:48). `surrender.cpp` declares the single method it needs locally
instead; `AI` is only forward-declared in Character.h, so the declaration
completes the same type and links normally.

---

## 8. Verification status

**Verified in a running game** (the premise holds):
- `NPCType` and `myPersonality` come back populated for NPCs from other mods --
  which is the entire reason this was built in code
- the cage-ownership derivation works (`isMyCage=yes` in the log)
- speech bubbles, the offer window, and recruiting all work end to end
- **Portraits**: working via `PortraitManager::setImageWidgetNPC(Character*, MyGUI::ImageBox*)`
- **Native notification sounds**: working via `ou->playNotification(NULL)`
- **Dynamic bribery & level check**: working for free NPCs

**Found only by playing** -- each of these was a bug the headers could not have
warned about, and all are written up in `../KENSHI-MODDING-NOTES.md` §11:
- `getOverallHealthRating()` returns 0 for anyone unconscious, so a death check
  on it disabled the whole mod
- `hunger` is not a 0..1 fraction (a fed NPC reads ~3)
- `inSomething == IN_PRISON` misses shackled and carried prisoners
- right-clicking a cage resolves to the cage, not the occupant
- declaring an engine `virtual` locally crashes the game
- `PortraitManager::setImageWidget/createPortraitImage` only draws player squad;
  `setImageWidgetNPC` is the required path for non-squad NPCs

**In progress / Needs refinement:**
- **Offer popup text responsiveness**: long description/dialogue texts clip vertically if window dimensions and text area are not dynamically sized according to line length. Needs dynamic height adjustment and scroll visibility fallback.
- **The talk cursor**: `ForgottenGUI::changeMouseCursor` hooks cleanly and never fires for the world cursor. Replaced by prisoners calling out to the player, which is better anyway.

**Never exercised:** the FCS dialogue bridge. It is written and compiles, but no
dialogue package has been authored to fire it, so `startPlayerConversation` with
a runtime-attached package remains unproven. The plugin's own offer window is
the path that actually runs.

---

## 9. New SDK bugs found while building this

Both are the same family as the known `ContextMenu` double definition: a type
defined in two headers, neither guarded, so whichever is included second is a
`C2011`. Worth adding to `KENSHI-MODDING-NOTES.md`.

**`BuildingDesignation`** — defined byte for byte identically in
`kenshi/Platoon.h:31` and `kenshi/Building/Building.h:14`.

Renaming the tag with `#define BuildingDesignation X` does **not** help: the
enumerators (`BD_NONE`, `BD_PRISON`, …) still collide as `C2365`.

*Workaround:* do not include `Building.h`. `Building` derives from `RootObject`
(Building.h:83) and both `getFaction()` and `getName()` live on
`RootObjectBase`, so the cage resolves through `hand::getRootObjectBase()`
rather than `hand::getBuilding()`.

**`CharacterMessage`** — defined in `kenshi/Character.h:110` and again in
`kenshi/AI/AI.h:48`, so `AI.h` cannot be included by anything that already has
`Character.h`.

*Workaround:* declare the one AI method you need locally. `AI` is only
forward-declared in `Character.h`, so a local declaration completes the same
type and links against `KenshiLib.lib` normally. See `surrender.cpp`.
