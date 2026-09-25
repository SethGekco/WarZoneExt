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

### Not yet built
- Phase 0 scaffold (template, submodules, CI) still to do. No DLL exists yet.
