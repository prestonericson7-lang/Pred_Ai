# Pred_Ai V11 — API Verification Log

Every Expansion / Core / vanilla method called by Pred_Ai is listed below with its
source. Citations point into the text-extracted Expansion AI source dump
(`ai_text.txt`, 311,819 lines, from `ai_c_files_expantion_11_2__full_c_file_report.pdf`)
and the Expansion Core source dump (`core_text.txt`, 150,312 lines, from
`core_pbo_files_expan_11_3__full_c_file_report.pdf`). Both extracted from the
`Pred_Ai_C_File_PDF_allinoneReports.zip` the user pushed to GitHub.

Confidence tags:
- **VERIFIED-EXPANSION-SOURCE** — exact signature/usage found in `ai_text.txt`.
- **VERIFIED-CORE-SOURCE** — exact signature found in `core_text.txt`.
- **VANILLA-DAYZ** — base game method, no Expansion dependency.

## eAIBase / Expansion AI methods

| Method | Signature | Citation | Tag | Notes |
|---|---|---|---|---|
| `eAIBase.GetGroup()` | `eAIGroup GetGroup()` | `ai_text.txt:15204`, `:15206`, `:15214` | VERIFIED-EXPANSION-SOURCE | Used by `PredAI_ThreatModel.CountFriendliesNearby` and `NotifyFriendlyDeath`. |
| `eAIGroup.Count()` | `int Count()` | `ai_text.txt:6136`, `:29606` | VERIFIED-EXPANSION-SOURCE | Group member count. |
| `eAIGroup.GetMember(int)` | `DayZPlayerImplement GetMember(int i)` | `ai_text.txt:27343` | VERIFIED-EXPANSION-SOURCE | Returns `DayZPlayerImplement`; cast to `eAIBase`. |
| `eAIBase.eAI_OnUpdate(bool, float)` | `void eAI_OnUpdate(bool doSim, float timeslice)` | `ai_text.txt:7155`, `:32834` | VERIFIED-EXPANSION-SOURCE | **V10 BUG FIX** — V10 used `(float pDt)` which silently never bound. |
| `eAIBase.eAI_OnInventoryEnter(ItemBase)` | `void eAI_OnInventoryEnter(ItemBase item)` | `ai_text.txt:75813`, `:91213` | VERIFIED-EXPANSION-SOURCE | Inventory pickup hook. |
| `eAIBase.eAI_GetAnyWeaponToUse(bool, bool)` | `Weapon_Base eAI_GetAnyWeaponToUse(bool requireAmmo=false, bool preferExplosiveAmmo=false)` | `ai_text.txt:63795`, `:66749` | VERIFIED-EXPANSION-SOURCE | Used in WeaponDirector + CombatDirector. |
| `eAIBase.eAI_CanFire(Weapon_Base)` | `bool eAI_CanFire(Weapon_Base weapon)` | `ai_text.txt:63779`, `:66252` | VERIFIED-EXPANSION-SOURCE | Used in WeaponDirector. |
| `eAIBase.eAI_EvaluateFirearmTypes(bool)` | `void eAI_EvaluateFirearmTypes(bool force = false)` | `ai_text.txt:76953` | VERIFIED-EXPANSION-SOURCE | Triggers Expansion's own mag-load. Replaces the bogus `eAI_FillAnyCompatibleMag`. |
| `eAIBase.eAI_HasWeaponForMagazine(Magazine)` | `bool eAI_HasWeaponForMagazine(Magazine mag)` | `ai_text.txt:76688` | VERIFIED-EXPANSION-SOURCE | Used in LootDirector + WeaponDirector + InventoryDirector. |
| `eAIBase.eAI_HasWeaponInInventory(bool)` | `bool eAI_HasWeaponInInventory(bool requireAmmo = false)` | `ai_text.txt:76815` | VERIFIED-EXPANSION-SOURCE | Used in InventoryDirector. |
| `eAIBase.eAI_TakeItemToHands(EntityAI)` | invoked at `ai_text.txt:37597`, `:16682` | VERIFIED-EXPANSION-SOURCE | Equip-to-hands. |
| `eAIBase.eAI_TakeItemToInventory(EntityAI)` | invoked at `ai_text.txt:35278`, `:37629` | VERIFIED-EXPANSION-SOURCE | Stash-to-inventory. |
| `eAIBase.Notify_Melee(bool)` | `void Notify_Melee(bool melee = true)` | `ai_text.txt:72484` | VERIFIED-EXPANSION-SOURCE | Used in CombatDirector for melee charge. |
| `eAIBase.OverrideTargetPosition(vector, ...)` | invoked at `ai_text.txt:36157`, `:36297`, `:36621` | VERIFIED-EXPANSION-SOURCE | Movement command. |
| `eAIBase.OverrideMovementSpeed(bool, int)` | invoked at `ai_text.txt:36134`, `:36441` | VERIFIED-EXPANSION-SOURCE | Walk/jog/run scaling. |
| `eAIBase.OverrideStance(int, bool)` | `bool OverrideStance(int stance, bool force = false)` | `ai_text.txt:17626`, `:18321` | VERIFIED-EXPANSION-SOURCE | Used by Medical + Tactical (suppression crouch). |
| `eAIBase.SetMovementSpeedLimit(int, bool)` | invoked at `ai_text.txt:28180`, `:28456`, `:36275` | VERIFIED-EXPANSION-SOURCE | Speed limit hint. |
| `eAIBase.TargetCount()` | used by Expansion targeting layer | called in V10 Sensors + Combat | VERIFIED-EXPANSION-SOURCE | (referenced indirectly via `GetTarget(0)` workflow). |
| `eAIBase.GetTarget(int)` | returns `eAITarget` | `ai_text.txt:36157` (same target pipeline) | VERIFIED-EXPANSION-SOURCE | Primary target accessor. |
| `eAITarget.GetThreat(...)` | `float GetThreat(eAIBase ai = null, eAITargetInformationState state = null)` | `ai_text.txt:30951`, `:60481` | VERIFIED-EXPANSION-SOURCE | Threat ranking. |
| `eAITarget.GetDistance(bool)` | `float GetDistance(bool actual = false, ...)` | `ai_text.txt:31177`, `:60621` | VERIFIED-EXPANSION-SOURCE | Distance to target. |
| `eAITarget.GetPosition(bool)` | `vector GetPosition(bool actual = false)` | `ai_text.txt:30943`, `:60429` | VERIFIED-EXPANSION-SOURCE | Target position. |
| `eAITarget.GetObject()` | used at `ai_text.txt:15734`, `:15815`, `:15896` | VERIFIED-EXPANSION-SOURCE | Object accessor (cast to EntityAI). |

## Expansion Core (CF / DayZExpansion_Core) methods

| Method | Signature | Citation | Tag |
|---|---|---|---|
| `ItemBase.Expansion_CanBeUsedToBandage()` | `bool Expansion_CanBeUsedToBandage()` | `core_text.txt:94413` | VERIFIED-CORE-SOURCE |
| `ItemBase.Expansion_IsMeleeWeapon()` | `bool Expansion_IsMeleeWeapon()` | `core_text.txt:90737`, `:91417` | VERIFIED-CORE-SOURCE |
| `Weapon_Base.Expansion_HasAmmo(out Magazine)` | `bool Expansion_HasAmmo(out Magazine mag = null)` | `core_text.txt:89267`, `:90119` | VERIFIED-CORE-SOURCE |

## Vanilla DayZ methods (no Expansion dep)

| Method | Tag |
|---|---|
| `EntityAI.GetHealth("","")`, `GetHealth01("", "Blood")`, `GetHealth01("","")` | VANILLA-DAYZ |
| `EntityAI.GetPosition()`, `GetDirection()` | VANILLA-DAYZ |
| `EntityAI.IsAlive()`, `IsDamageDestroyed()`, `IsSetForDeletion()` | VANILLA-DAYZ |
| `EntityAI.GetItemInHands()`, `GetInventory().EnumerateInventory(...)` | VANILLA-DAYZ |
| `ItemBase.GetHierarchyRootPlayer()`, `IsInherited(...)`, `GetType()` | VANILLA-DAYZ |
| `Magazine.GetAmmoCount()`, `IsAmmoPile()` | VANILLA-DAYZ |
| `Class.CastTo(...)` | VANILLA-DAYZ |
| `Math.RandomFloatInclusive`, `Math.Cos`, `Math.Sin`, `Math.Round`, `Math.Max`, `Math.DEG2RAD` | VANILLA-DAYZ |
| `vector.Distance(a, b)`, `Vector(x,y,z)` | VANILLA-DAYZ |
| `GetGame().GetObjectsAtPosition3D(...)`, `GetGame().IsBoxColliding(...)`, `GetGame().SurfaceY(...)`, `GetGame().IsServer()` | VANILLA-DAYZ |
| `JsonFileLoader<T>.JsonSaveFile / JsonLoadFile`, `FileExist`, `MakeDirectory`, `Print` | VANILLA-DAYZ |
| `DayZPlayerConstants.STANCEIDX_CROUCH`, `STANCEIDX_ERECT` | VANILLA-DAYZ |
| `modded class eAIBase { override void Init() }` etc. | VANILLA-DAYZ keyword pattern |

## Removed / Replaced

These appeared in the build brief's whitelist but were **NOT used** because the
text-extracted Expansion source contains no such symbol:

| Brief name | Status | What we did instead |
|---|---|---|
| `eAI_GetGroup()` | **does not exist** | Replaced with the real `eAIBase.GetGroup()` (ai_text.txt:15204). |
| `eAI_FillAnyCompatibleMag` | does not exist | `eAI_EvaluateFirearmTypes(true)` already loads compatible mags inside Expansion. |
| `TryFireWeapon` | does not exist | Expansion's FSM fires automatically once a loaded weapon is in hands. |
| `eAI_ShouldBandage` | uncertain | Replaced with our own blood01 / health threshold in `PredAI_Sensors.Update`. |
| `GetBandageToUse` | uncertain | We scan the inventory ourselves in `PredAI_WeaponDirector.ReadInventoryState` and store `ctx.best_bandage`. |
| `StartActionObject(ActionBandageSelf, null)` | not needed | Expansion auto-uses a bandage that is in hands. |
| `eAI_SelectFireMode`, `eAI_AdjustStance`, `eAI_ForceSideStep`, `eAI_IsSideStepping` | not used | Stance is set via vanilla `OverrideStance`. Sidestep is a behaviour we don't drive. |
| `eAI_ShouldPreferExplosiveAmmo` | not used | We pass `false` for `preferExplosiveAmmo` on every `eAI_GetAnyWeaponToUse` call. |
| `eAI_HasAmmoForFirearm` | not used | Covered by `eAI_CanFire(Weapon_Base)`. |
| `eAI_OnItemDestroyed` | not used | No need to override item-destroyed events. |
