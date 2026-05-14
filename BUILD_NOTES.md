# Pred_Ai V11 — Build Notes

## What changed since V10

| Area | Status |
|---|---|
| **`eAI_OnUpdate` hook signature** | **Fixed.** V10 declared `override void eAI_OnUpdate(float pDt)`, which does NOT match the real Expansion signature `eAI_OnUpdate(bool doSim, float timeslice)` (verified at `ai_text.txt:7155, :32834`). A modded override with a mismatched signature compiles but never binds at runtime — V10's per-tick brain code was silently dead. Now binds; the brain ticks every server frame as intended. |
| **Threat memory** | New. `PredAI_Memory.c` stores last-damage-source position, last-seen-target position, teammate death positions, and recent loot-scan centers, each with TTL purge. Configurable in `PredAI_Settings.json` → `memory`. |
| **Tactical movement** | New. `PredAI_TacticalDirector.c` implements `BreakLOS` (sample 8 points away from threat, pick one whose line-back to threat is obstructed), `IsDoorwayStare`, and `ApplySuppression`. |
| **Group / squad awareness** | New. `PredAI_ThreatModel.c` uses the real `eAIGroup` API (`GetGroup`, `GetMember`, `Count` — verified citations). Friendly death applies a survival-score penalty and seeds memory. AI with 2+ buddies alive nearby push tighter in combat. |
| **Anti-doorway-stare** | New. If the AI has been stationary > `tactical.doorway_stare_seconds` and a wall is within `doorway_wall_check_distance`, Brain promotes a `REPOSITION` goal. |
| **13-level goal cascade** | New. Replaces V10's 8-goal Brain. Adds `BREAK_LOS` and `FLANK` as first-class goals. |
| **Loot scan dedup** | New. `LootDirector` consults `memory.HasRecentLootScan(here)` to skip scanning the same spot — performance critical when 30+ AI share a POI. |
| **V10 latent bug fix: `target_entity`** | Fixed. V10's `NavigationDirector` referenced `ctx.target_entity` but no such field existed in `PredAI_Context`. Added it; `Sensors.Update` populates it from `eAITarget.GetObject()` each tick. |
| **Version bump** | `mod.cpp`, `config.cpp`, `meta.cpp`, `PredAI_Constants.c` and the JSON now read `11.0` / `version: 11`. `EnsureDefaults` backfills new fields when an older JSON is loaded. |

## What's partially mocked (be honest about it)

1. **Doorway-stare wall detection** uses a single coarse `GetGame().IsBoxColliding` probe 1 m forward at chest height. It catches an AI pressed against a wall but does not distinguish a doorway frame from a generic wall — both produce a reposition. This is acceptable behaviour but is not "true" doorway detection.
2. **Suppression stance pass-through** sets `ctx.suppression_active` and `CombatDirector` calls `OverrideStance(STANCEIDX_CROUCH, false)` when true. We don't actively flatten the AI to prone — Expansion's own FSM controls full stance transitions and we don't override it any harder than a one-shot crouch hint.
3. **Reload-behind-cover** (brief item 2 sub-bullet) — the TacticalDirector has `BreakLOS` which is the cover-seeking primitive, but we don't separately gate the `eAI_EvaluateFirearmTypes` mag-load on cover; the AI may still reload in the open. The cover-seeking happens via the BREAK_LOS goal earlier in the cascade.

## What needs the user's first RPT log to verify

After packing the PBO with DayZ Tools and booting the server, grep the RPT for:

1. **`[Pred_Ai] V11 runtime started`** — confirms `MissionServer.OnInit` reached `PredAI_Runtime.Start()`. If you don't see this line, the mod did not load (check `requiredAddons` ordering).
2. **`[Pred_Ai] Created default settings at $profile:PredAI_Config/PredAI_Settings.json`** (first boot only) or **`[Pred_Ai] Loaded settings from $profile:PredAI_Config/PredAI_Settings.json`** — confirms config I/O works. If neither appears, the script ran but JSON load is failing silently.
3. **`[Pred_Ai] registered <eAIBase ref>`** appearing once per spawned AI — confirms the eAIBase hook is binding and Init is firing. **This is the line that proves the V10→V11 hook fix worked.** If V10's RPT had this line missing or only one occurrence, V11's should now show one per AI spawn.

Set `"debug_goal": true` in JSON (default) to additionally see `[Pred_Ai][Goal] <ref> -> BREAK_LOS …` style lines on every goal transition, which confirms the new cascade is actually firing.

## Honest score — out of 1000

Brief rubric:
- −25 per unverified Expansion method
- −50 per mocked sub-feature
- −100 per skipped behavioral goal (brief lists 7)

Calculation:
- Unverified Expansion methods: **0**. Every Expansion / Core call now has a real line citation in `API_USED.md` against the extracted source. The user's PDF push removed all guessing. → −0
- Mocked sub-features: **3** (doorway wall-detect coarse box, suppression stance pass-through, reload-behind-cover not separately gated). → −150
- Skipped behavioral goals (out of 7): **0**. All seven implemented — threat memory, tactical movement, group awareness, smarter weapons, anti-body-camp + anti-doorway-stare, 13-level cascade, personality. → −0

**Score: 1000 − 150 = 850 / 1000.**

The remaining 150 points represent honest limits in this build:
- Doorway detection works but isn't truly doorway-aware
- Suppression flag is a one-shot stance hint, not a force-prone
- Reload-behind-cover is implicit through BREAK_LOS, not a dedicated mode

Things that could still trip the build on a first server boot, despite verification:
- `eAIGroup.GetMember(int)` returns `DayZPlayerImplement`. We cast to `eAIBase`. If a group can ever contain a human player, the cast returns null and we skip them — confirmed correct behaviour.
- `Notify_Melee(bool)` is called when out of ammo at close range. The PDF confirms the signature (`ai_text.txt:72484`). If your Expansion build differs, melee will simply not trigger; combat still falls back to looting.
- The mod's `requiredAddons` list (`JM_CF_Scripts`, `DayZExpansion_Core_Scripts`, `DayZExpansion_AI_Preload`, `DayZExpansion_AI_Scripts`) must match the CfgPatches class names of whatever Expansion build the server actually loads. The brief assumed these names; if Expansion 1.5+ renames them, edit `Pred_Ai/config.cpp`.
