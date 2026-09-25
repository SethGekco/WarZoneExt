# WarZoneExt

Shared spatial memory for the Yuri's Revenge AI DLLs.

It remembers **where things happen** on each map — accumulated across every game
ever played — and answers queries about it. Kill zones, miner deaths, contested
structures, traffic, pathfinding errors, per-spawn attack lanes: each is a named
**zone**, so new kinds of memory are a new name rather than a redesign.

It is one primitive and **never acts**: no teams, no targets, no triggers, no
production. Recorders write, consumers decide.

Consumed (optionally, at runtime) by **AITriggerTypeExt**, **DoctrineExt** and
**DossierExt** — so one feature doesn't require three feature-heavy DLLs.

Why it matters for AI learning: per-player habits gain one sample per finished
match, while zones gain samples per *event*, from *every* house, and are
player-independent — so a brand-new opponent on a known map inherits its whole
geography at once. **WarZoneExt learns the map; DossierExt learns the person.**

- [DESIGN.md](DESIGN.md) — what it is, zone list, storage, API, phases
- [CHANGELOG.md](CHANGELOG.md) — **read before changing anything**; contracts and
  the same-commit convention
- [CLAUDE.md](CLAUDE.md) — working notes and the cross-project workflow

## Status

**Phase 1** — recording `Kill`, `MinerDeath` and `Lane.<spawn>.Out/In` from the
kill seat, on top of the Phase 0 spine (map identity + per-map load/save, verified
in-game). Still exports nothing: don't bind an API until `WZ_Version()` exists
(Phase 3).

## Build

CI builds `DevBuild` via MSBuild (`.github/workflows/build.yml`), after
range-checking our hooks against the YR-Hook-Encyclopedia registry. Submodules
are YRpp + Phobos (utility headers only — this is not a Phobos fork), pinned to
the same commits as DoctrineExt and DossierExt.
