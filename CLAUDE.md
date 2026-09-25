# WarZoneExt — notes for agents

Read `DESIGN.md` for what this is.

## 0. Rex treats this repo as invisible infrastructure

He does not want to manage WarZoneExt as a separate project. When he asks for a
zone feature **in a consumer project** (AITriggerTypeExt, DoctrineExt,
DossierExt), the agent in that session does **both halves itself, in one go**:

1. **Extend WarZoneExt** so it can supply what's needed — *additively*, without
   changing behaviour another consumer already relies on.
2. **Wire it into the consumer** as an *optional* runtime dependency
   (`GetModuleHandle` + `GetProcAddress`), so that DLL still works unchanged
   when WarZoneExt isn't installed and lights up when it is.

Do not ask him to coordinate the other projects, and do not hand him a task to
relay. The changelog is the coordination mechanism — see §1.

**Add, don't repurpose.** A new requirement means a **new zone name**, never a
changed meaning for an existing zone, exported function, or stored field. That
single habit is what lets three sessions extend this repo in parallel without
breaking each other.

## 1. Update CHANGELOG.md in the same commit as your change

This is a **shared dependency** with three consumers (AITriggerTypeExt,
DoctrineExt, DossierExt), each developed in a separate session that cannot see
the others' context. `CHANGELOG.md` is the only channel between them. Its header
explains the format; follow it, including the *why*.

Breaking changes — the exported API, zone **names** (consumers query them as
strings), or the on-disk format — must say **BREAKING**, bump `WZ_Version()`, and
name the consumers that need updating.

## 2. Stay a primitive

WarZoneExt **records and answers**. It does not build teams, pick targets, enable
triggers, or produce anything. If a change would make it act on its own data,
it belongs in the consumer instead. That boundary is why three projects can
depend on it without fighting.

## Working notes carried over from DossierExt

- Map identity: `ScenarioClass::FileName` is **always `spawnmap.ini`** in
  skirmish/CnCNet. Use spawn.ini `[Settings] UIMapName` (strip a leading `[8] `
  tag, sanitise, keep it dot-free). Getting this wrong collapses every map into
  one record.
- "Changed hands" is `Owner != InitialOwner` — exact, and what DossierExt uses.
  `HasBeenCaptured` is **not** a safe substitute: an AI house showed 14 of them
  in one game and the cause was never established, so treat it as meaning more
  than "was captured" until someone proves otherwise. `Capturable` is worse
  still — vanilla sets it on ordinary buildings (GAPILE/GAREFN/GAPOWR), so it
  identifies nothing. Neither flag means what its name suggests.
- Kill seat: `0x702D40` RegisterDestruction entry, `ECX` = victim,
  `[ESP+4]` = killer. Same-address hooks chain legally; DoctrineExt and
  DossierExt already sit there read-only.
- Harvester flag for miner zones: `UnitTypeClass::Harvester`.
- Run the hook overlap check against the YR-Hook-Encyclopedia registry in CI.
- Log a **breakdown** of any classification, never just a total. Every
  misdiagnosis in DossierExt was caught (or caused) by this.
- Verify a flag's real meaning in rulesmd before building on it.
