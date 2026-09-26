# Changelog — WarZoneExt

> **Read this before you change anything.**
>
> WarZoneExt is a **shared dependency**. Three projects consume it —
> AITriggerTypeExt, DoctrineExt and DossierExt — and each is developed in its
> own session, by a different agent, often on the same day. Nobody sees the
> other sessions' context. This file is the only thing they all share.
>
> **Therefore: every change gets an entry here, in the same commit.** Not a
> summary afterwards, not "later" — an entry, with the reason. A changelog entry
> is cheaper than three agents re-deriving the same decision, and far cheaper
> than two of them silently breaking each other's assumptions.
>
> **Agents: this convention applies to you.** Add your entry. If you consume
> WarZoneExt from another project, also note the dependency there so that
> project's future sessions know it exists.

## How to write an entry

Newest first. One block per change:

```
## [unreleased] — YYYY-MM-DD — <who/which project drove it>
### Added / Changed / Fixed / Removed / API
- What changed, and WHY (the why is the part that saves the next agent).
- Anything a consumer must do differently. Say "no consumer action" if none.
```

Rules that keep three chats from colliding:

1. **The exported API is a contract.** Adding a function is safe. Changing or
   removing one is a breaking change: bump `WZ_Version()`, put **BREAKING** in
   the entry, and list which consumers need updating.
2. **Zone names are a contract too.** A consumer queries `"Kill"` by string.
   Renaming a zone breaks them silently — treat it as breaking.
3. **The on-disk format is a contract.** If a stored file's meaning changes,
   say whether old files stay readable. Never silently reinterpret old data.
4. **Record what you verified in-game**, not just what you wrote. "Deployed,
   log confirms Kill zone accumulating" is worth more than "implemented zones".
5. **If you had to correct an earlier claim, say so.** A wrong explanation left
   standing gets built on by the next agent.

---

## [unreleased] — 2026-09-25 — AITriggerTypeExt session (ground/naval reachability, Rex)

### Added
- **`Terrain.GroundZone` / `Terrain.WaterZone` zones** (NEW names — additive; new
  file `Connectivity.cpp`, doesn't touch Terrain.cpp or anything else Phase 2
  added). Same "measured once, useful on game 1, never recomputed" shape as
  `Terrain.Open/Cliff/Water/Choke`, but answers a different question: not "how
  open is this bucket" but "can you actually get from bucket A to bucket B".
  One representative cell per bucket samples `MapClass::GetMovementZoneType`
  (`0x56D230`, `MovementZone::Normal` / `::Water`) — YR's own precomputed
  per-cell connectivity grid, not a live pathfind. Two buckets are reachable by
  that movement type iff their stored value is **equal**.
- **Weight convention here differs from every other zone in the store — read
  this before querying it.** The stored int is `(engine zone id + 1)`, not a
  count or a percentage. `0` means *not recorded* (no zone exists for that
  movement type at that bucket, or the scan hasn't run). The `+1` exists because
  a real zone id can legitimately be `0`, which would otherwise be
  indistinguishable from "unrecorded" through `WZ_ZoneWeight`'s existing
  0-means-unknown convention. **Compare two reads for equality only** — never
  sum, rank, or threshold the value; the magnitude itself carries no meaning.
- No new config: reuses `[WarZone.Terrain] Scan`. Own skip-check keyed on its
  own zone names (`HasZone("Terrain.GroundZone")`), deliberately independent of
  `Terrain::EnsureScanned`'s `Open`/`Cliff` check — so a map record saved
  *before* this feature existed (which already has `Terrain.Open` on disk)
  doesn't short-circuit this scan and leave connectivity permanently
  unrecorded on that map. The `Terrain.` prefix gets it the existing
  `Zones::IsStatic` exemption (no TopN pruning, no decay) for free.

### API
- No new exports. Reads through the existing generic `WZ_ZoneWeight(zone, x, y)`
  — that's the whole point of the by-name zone design. No consumer action
  needed to gain access once WarZoneExt is present.

### Consumers
- **Origin**: the AITriggerTypeExt session that requested this already ships
  `RequiresGroundPathToEnemy`/`RequiresNavalPathToEnemy` conditions computed the
  *same way* directly against the engine — cheap enough that a single boolean
  trigger gate doesn't need WarZoneExt. **AITriggerTypeExt keeps that direct
  call; no rework, no action needed.**
- **Added ahead of its consumer, on purpose** (Rex: "add it even if it's as a
  reference"). Intended first real consumer is **DoctrineExt**, for
  multi-path / nearest-reachable-enemy reasoning during its route-steering
  work (the "route saturated → steer around it" item on its roadmap). DossierExt:
  no action, nothing it depends on changed.
- Whoever binds this first: check `WZ_Version` per the existing pattern, and
  treat `0` as "not recorded" per the weight convention above — do **not**
  assume the returned int is literally the engine's zone id.

### Verified
- Not yet. CI pending on this commit. In-game verification needs a debug log
  showing `connectivity scanned: N bucket(s) -> ...` at map load, and ideally a
  multi-landmass map to confirm `distinctGround > 1` actually flags disconnected
  land. Will update this entry once tested.

## [unreleased] — 2026-09-25 — DossierExt session (terrain verify, Rex)

### Verified in-game (first real data)
- **Phase 0/1/2 all confirmed working.** `Powder_Keg.ini` grew 149B -> 19KB.
  Load half of the round-trip finally proven: `map 'Powder_Keg' loaded: games=2
  244x244 spawns=2, 14 zone(s), 1704 bucket(s)`. Terrain caching confirmed:
  "terrain already on record for this map, skipping scan".
- Every event zone accumulating: `Kill` (hottest 13,16=141), `MinerDeath`,
  `MinerTravel` (24,12=199), `Traffic`, `StructureLoss`, `FirstContact`,
  `Lane.0/1.In/Out`, plus the other sessions' `AirDeath`, `Terrain.GroundZone`,
  `Terrain.WaterZone`.
- **Nice cross-check that the lane logic is right:** `Lane.0.Out` and
  `Lane.1.In` agree exactly (13,16=123), as do `Lane.1.Out` and `Lane.0.In`
  (9,20=17) — spawn 0's kills ARE spawn 1's losses, so the direction split is
  wired correctly.

### Fixed / Added
- **`Terrain.Cliff` came back empty — RESOLVED, and it was correct.** Rex
  confirmed the tested map (Powder Keg) simply has no cliffs, so zero buckets is
  the right answer, not a detection failure. Recording the correction here per
  rule 5: the earlier wording in this entry implied a possible bug, and there
  isn't one.
- **Caveat for whoever relies on cliffs:** absence is now confirmed correct, but
  *presence* has never been positively verified — no tested map has had cliffs
  yet. Before trusting `Terrain.Cliff` for anything load-bearing, check it on a
  map with real cliffs and confirm the buckets land where they should.
- Added a **LandType census** to the scan log anyway
  (`LandType census: Clear=... Water=... Rock=...`): it makes "this map has no
  X" self-evident from the log instead of something a future agent has to ask
  about, and it cost nothing.
- New `[WarZone.Terrain] Rescan=yes` to force a re-scan when the SCANNER changes
  (stored terrain is otherwise permanent and would mask any fix). **No consumer
  action; no zone or API change.**

## [unreleased] — 2026-09-25 — DoctrineExt session (AA air-defense, Rex)

### API — FIRST EXPORTED SURFACE (`WZ_Version()` now exists)
- `WZ_Version()` → int (currently **1**). The bind gate: a consumer does
  `GetModuleHandleA("WarZoneExt.dll")` + `GetProcAddress`, and if it's absent or
  returns less than the consumer needs, the consumer must not rely on the rest.
  This is the API the changelog earmarked for "Phase 3", brought forward because
  DoctrineExt's air-defense needed a persistent zone to read. **Additive — no
  action for existing consumers; binding is optional by construction.**
- `WZ_ZoneWeight(const char* zone, int cellX, int cellY)` → int, the weight stored
  for a zone at a map cell (0 if unknown / not recorded / disabled). Generic **by
  zone name** as the design intends (one query; names are data). x86 `__cdecl`,
  `extern "C"`, exported undecorated as `WZ_Version` / `WZ_ZoneWeight`.

### Added
- **`AirDeath` zone** (NEW name — additive, nothing repurposed): where aircraft
  get their kills. Recorded by a SECOND read-only hook at the kill seat `0x702D40`
  (`DEFINE_HOOK_AGAIN`; Recorder keeps its own hook — same-address chaining is
  legal, Syringe runs both). Every air-caused death, every house, so it becomes
  map-level "air is contested *here*" memory a brand-new opponent inherits on a
  known map. First consumer: DoctrineExt air-defense (AA P2), blending this
  persistent prior with its own in-game per-house air death-zones.

### Consumers
- **DoctrineExt** will bind `WZ_Version` + `WZ_ZoneWeight` **optionally**
  (`GetModuleHandle`/`GetProcAddress`), so it runs unchanged when WarZoneExt isn't
  installed and lights up when it is. No action for AITriggerTypeExt / DossierExt —
  the export table and every existing zone are untouched.

### Verified
- Builds: pending CI on this commit. **Not yet verified in-game** — needs a match
  with aircraft to confirm the `AirDeath` zone accumulates and `WZ_ZoneWeight`
  reads it back. Will update here once tested. (Recording sits on the same proven
  kill seat as the `Kill` zone, which is already confirmed accumulating.)
- NOTE: this landed on top of Phase 2 (terrain + Traffic/MinerTravel/StructureLoss/
  FirstContact), which a parallel session committed moments earlier. To stay out of
  its way this change touches only `WarZoneExt.cpp` + this file — no edits to the
  shared Recorder/Zones/Engine/Config, so nothing it added is affected.

## [unreleased] — 2026-09-24 — DossierExt session (Rex)

### Added
- Project created. Design in `DESIGN.md`.
- **Renamed from the original KillZoneExt proposal to WarZoneExt** at Rex's
  request, so the DLL can hold more than kill data: high-traffic zones,
  pathfinding-error zones, miner-travel zones and whatever comes later. The
  generalisation is why `WZ_ZoneWeight()` takes a zone **name** instead of there
  being one function per zone type.

### Why this DLL exists
- Split out of DossierExt because all three AI brains want spatial memory, and
  Rex does not want users installing three feature-heavy DLLs to get one
  primitive.
- It is also the answer to "how do we make the AI learn faster": per-player
  habits gain **one sample per finished match**, while zones gain samples **per
  event**, from **every house**, and are **player-independent** so a brand-new
  opponent on a known map inherits the map's geography immediately.

### API
- Nothing exported yet — no consumer action. Phase 3 introduces the API; until
  `WZ_Version()` exists, consumers must not attempt to bind.

### Workflow (how Rex wants this repo handled)
- He treats WarZoneExt as **invisible infrastructure** and does not want to
  manage it as a separate project. A request for a zone feature arrives in a
  *consumer* session; that agent does **both halves itself**: extend whatever is
  needed here (additively), then wire it into the consumer as an optional
  runtime dependency. No task-relaying back to Rex. See `CLAUDE.md` §0.
- Corollary: **add, don't repurpose.** New requirement = new zone name. Never
  change the meaning of an existing zone, exported function, or stored field —
  that is what lets three sessions extend this in parallel safely.

### Added — Phase 0 (scaffold, map identity, load/save round-trip)
- Scaffold from the DossierExt/AcademyExt template: YRpp + Phobos submodules
  pinned to the same commits the sibling projects use (3ba94954 / 47475624), CI
  building DevBuild after range-checking hooks against the YR-Hook-Encyclopedia
  registry.
- **4 hooks, all shared seats already proven by the siblings** — no new address
  risk: `0x7CD810` ExeRun (static patches), `0x52F639` CmdLineParse (flush log),
  `0x685659` Scenario_ClearClasses (per-scenario reset + re-parse),
  `0x4F8440` HouseClass::Update (frame pulse; WarZoneExt has no per-house logic,
  it self-gates on frame). Overlap + bounds checks clean.
- `[WarZone.General]`: Enabled, DebugTicks, ZoneDir, Bucket, TopN, DecayShift,
  ForgetBelowGames, CheckpointInterval, UseHistoryInMultiplayer.
- **Zone store** (`Zones.h/.cpp`): the generic registry — `Add(zone, x, y, w)` is
  the single write path, so pruning and decay live in one place and every future
  zone type inherits them. Load/save `<ZoneDir>/<MapStem>.ini` with `[Meta]` +
  `[Zone.<Name>]`; zone names may contain dots (`Lane.0.Out`) since the parser
  splits only on the first one. Prunes to `TopN` on save; applies `DecayShift`
  and `ForgetBelowGames` on load.
- **Map identity** (`MapId.h/.cpp`): carries over the trap that cost DossierExt a
  test cycle — `ScenarioClass::FileName` is *always* `spawnmap.ini` in
  skirmish/CnCNet, so the real name comes from spawn.ini `[Settings] UIMapName`
  (leading `[8] ` tag stripped, sanitised dot-free). Getting this wrong collapses
  every map into one record.
- Writes the record at match **start**, not just at the end, so a broken IO path
  shows up immediately rather than after a full game.
- Logs the MP verdict once per match: recording is synced-safe and always on, but
  stored history differs per client, so it is off-limits for sim-affecting use
  with 2+ humans unless `UseHistoryInMultiplayer=yes`.

### Added — Phase 1 (recording: Kill, MinerDeath, Lane)
- `Recorder.cpp`, one hook at the proven kill seat `0x702D40`
  (RegisterDestruction; ECX=victim, `[ESP+4]`=killer — DoctrineExt and DossierExt
  already sit there, same-address hooks chain, and we only read so order is
  irrelevant). **5 hooks total, overlap clean.**
- Zones now written (**new names — additive, nothing repurposed**):
  - `Kill` — every death, every house. Recording AI-vs-AI too is most of why
    this layer converges in a few games instead of dozens.
  - `MinerDeath` — deaths of `UnitTypeClass::Harvester` units, i.e. vulnerable
    harvest spots. Under-reports rather than guessing: an unflagged miner-like
    unit simply isn't counted.
  - `Lane.<spawn>.Out` / `Lane.<spawn>.In` — derived from the same event, so
    "this map fights here" becomes "a player starting *there* fights *here*" for
    free. Only counted when the two houses are actually hostile, so friendly
    fire and civilian casualties can't paint a false attack route.
- Zone summary now logs at every checkpoint under `DebugTicks`, not only at a
  clean game end — matches here routinely stop without a verdict, and a summary
  nobody sees can't be graded.

### Added — Phase 2 (terrain analysis + four more zones)
- **`Terrain.cpp` — static map analysis, a different KIND of zone.** Event zones
  need games to accumulate; terrain is a *fact about the map*, so it is computed
  once the first time a map is seen and is useful on **game 1**, before any play
  history exists. Never recomputed (cliffs don't move). Weight = percent, so the
  stored numbers describe **spacing** directly:
  - `Terrain.Open` — `>= OpenPercent` passable: room to manoeuvre
  - `Terrain.Cliff` — `>= CliffPercent` rock/wall: a flank to anchor on
  - `Terrain.Water` — `>= CliffPercent` water
  - `Terrain.Choke` — passable but narrow (`ChokeMin..ChokeMax%`) **and** joining
    ≥2 open buckets, so it's a real corridor rather than a ragged map edge
  Cost: one linear pass over cells, then one over buckets; neighbour lookups are
  per-bucket map probes, so nothing is quadratic in cells.
- **`Terrain.*` is exempt from `TopN` pruning, `DecayShift` and
  `ForgetBelowGames`.** A map has far more than TopN cliff buckets and pruning
  them would silently amputate its geography. Evidence zones still prune/decay.
- **`Traffic.cpp` — sampled, not hooked**, because movement is a continuous state
  and a hook on it would fire absurdly often. Every `TrafficInterval` frames,
  bucket anything with `Destination != nullptr`:
  - `Traffic` — the road network as players actually use it, not as the terrain
    implies
  - `MinerTravel` — the ore commute, i.e. where interdiction actually hurts
- Two more zones free off the existing kill seat:
  - `StructureLoss` — where *buildings* die: ground someone committed to and
    then lost. Distinct from `Kill` because losing a structure means something
    different from losing a unit.
  - `FirstContact` — the first hostile death of a match *only*. One sample per
    game, so it converges into a genuine opening-engagement map instead of being
    drowned by a long battle.
- New config: `[WarZone.General] TrafficInterval`; `[WarZone.Terrain] Scan`,
  `OpenPercent`, `CliffPercent`, `ChokeMinPercent`, `ChokeMaxPercent`.
- All names are new — additive, nothing repurposed. 5 hooks unchanged.

### Notes
- Phase 1 and 2 are both **untested in-game** as of this entry — recording-only
  and exporting nothing, so they cannot affect gameplay, but treat their output
  as unverified until a log confirms it.
- Phase 0 deliberately recorded nothing, proving the spine (identify map → read →
  play → write) before data fed it. **Verified in-game 2026-09-24**: the
  spawnmap trap fix resolved `UIMapName='Powder_Keg'` correctly first try, and
  `WarZones/Powder_Keg.ini` was written with `[Meta]` only. The *load* half of
  the round-trip is still unverified — it needs a second match on the same map.
- Config echo happens from the per-scenario parse, **not** from ExeRun: logging
  at `0x7CD810` is discarded because the log isn't open yet.
