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
