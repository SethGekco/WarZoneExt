# WarZoneExt

**What it does:** remembers *where things happen* on each map, forever, and lets
the AI DLLs ask about it. One primitive: spatial memory. It never acts — no
teams, no triggers, no production. Observers record, consumers decide.

Shared by **AITriggerTypeExt**, **DoctrineExt** and **DossierExt** so none of
them has to carry this itself, and users don't install three heavy DLLs to get
one feature.

## Zones

A **zone** is one named grid of accumulated weight per map. Types are a list, not
a hardcoded set — new ones drop in without redesign:

| Zone | Records | Useful for |
|---|---|---|
| `Kill` | where anything dies | lethal ground, ambush spots, avoid-routes |
| `MinerDeath` | harvester deaths | vulnerable miner spots; raid or defend |
| `Contest` | structures changing hands | engineer/spy rush targets |
| `Traffic` | heavy unit movement | travel lanes, chokepoints |
| `PathError` | pathfinding failures/stalls | bad terrain the AI should not route through |
| `MinerTravel` | harvester routes | where to cut an economy |
| `Lane.<spawn>.Out` / `.In` | per-spawn attack direction | "a player starting there fights here" |

## Why it exists (the learning-rate argument)

Per-player habits learn **one sample per finished match** — dozens of games
before a tell means anything. Zones learn **per event** (hundreds per match),
from **every house** (AI-vs-AI teaches the map too), and are
**player-independent** so knowledge is shared: a brand-new opponent on a known
map inherits the whole geography at once.

So: **WarZoneExt learns the map, DossierExt learns the person.** Map meta
converges fast, personal habits slowly — together the AI is useful on day one.
It also gives DossierExt a baseline, which is what separates "rex always pushes
west" from "everyone pushes west here". Only the difference is a real tell.

## Storage

`<ZoneDir>/<MapStem>.ini`, one file per map, INI so it stays greppable.

```
[Meta]              Games= Width= Height= Spawns= LastSeen=
[Zone.Kill]         bx,by=weight
[Zone.MinerDeath]   bx,by=weight
[Zone.Contest]      bx,by=games
[Zone.Lane.0.Out]   bx,by=weight
```

Size control, all modder-facing: `Bucket=` (cells per bucket, coarse by
default), `TopN=` pruning per zone on save, `DecayShift=` to fade stale meta, and
`tools/clean_maps.py --min-games N` to delete barely-played maps.

## Access

Persistence is the file; live queries are a small exported C API, resolved by
consumers with `GetProcAddress` so it is an **optional** dependency — the other
DLLs work unchanged without it and light up when present:

```c
int WZ_Version(void);
int WZ_ZoneWeight(const char* zone, int cellX, int cellY);
int WZ_Hotspot(const char* zone, int rank, int* outX, int* outY);
int WZ_MapGames(void);              // how much evidence this map has
int WZ_HistoryUsable(void);         // 0 in MP: see below
```

## Multiplayer

Recording reads synced sim events, so every client builds identical in-game
grids — safe, always on. The *persisted* file is local history and differs per
machine, so anything sim-affecting that reads it is single-human only by default
(`UseHistoryInMultiplayer=no`); `WZ_HistoryUsable()` reports it and consumers
must respect it.

## Phases

0. Scaffold + map identity (spawn.ini `UIMapName`; `ScenarioClass::FileName` is
   always `spawnmap.ini`) + empty load/save round-trip. Log-only.
1. `Kill`, `MinerDeath`, `Lane.*` from the proven seat `0x702D40`
   (RegisterDestruction, ECX=victim, [ESP+4]=killer). Prune + decay.
2. `Contest` (via `Owner != InitialOwner`, verified in DossierExt) + fingerprint.
3. Exported API + `clean_maps.py`.
4. `Traffic`, `PathError`, `MinerTravel`.
5. Consumers: DossierExt baseline, then DoctrineExt and AITriggerTypeExt.

## Open

- Should DossierExt keep personal spatial grids, or store only the *delta* vs
  map meta? (Leaning: both — the delta is the tell.)
- Decay default: fade on load, or only when the modder asks?
