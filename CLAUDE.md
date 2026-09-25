# WarZoneExt — notes for agents

Read `DESIGN.md` for what this is. Two rules specific to this repo:

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
- "Changed hands" is `Owner != InitialOwner`. **Not** `HasBeenCaptured` — that
  also trips on garrison-type interactions, and `Capturable` is set on ordinary
  buildings too, so neither flag means what its name suggests.
- Kill seat: `0x702D40` RegisterDestruction entry, `ECX` = victim,
  `[ESP+4]` = killer. Same-address hooks chain legally; DoctrineExt and
  DossierExt already sit there read-only.
- Harvester flag for miner zones: `UnitTypeClass::Harvester`.
- Run the hook overlap check against the YR-Hook-Encyclopedia registry in CI.
- Log a **breakdown** of any classification, never just a total. Every
  misdiagnosis in DossierExt was caught (or caused) by this.
- Verify a flag's real meaning in rulesmd before building on it.
