class PredAI_WeaponDirector
{
    //! Reads complete inventory state into context.
    //! Also scans for best_bandage while enumerating.
    static void ReadInventoryState(PredAI_Context ctx)
    {
        if (!ctx || !ctx.ai)
            return;

        eAIBase ai = ctx.ai;

        //! eAI_GetAnyWeaponToUse(bool needsAmmo, bool preferExplosive)
        //! true/false  = wants a ready (loaded) weapon, no explosive bias
        ctx.ready_weapon = ai.eAI_GetAnyWeaponToUse(true, false);
        ctx.best_weapon  = ctx.ready_weapon;

        if (!ctx.best_weapon)
            ctx.best_weapon = ai.eAI_GetAnyWeaponToUse(false, false);

        ctx.has_firearm       = ctx.best_weapon != null;
        ctx.has_loaded_weapon = ctx.ready_weapon != null;
        ctx.has_weapon_ready  = ctx.has_loaded_weapon;
        ctx.has_unloaded_weapon = ctx.has_firearm && !ctx.has_loaded_weapon;

        //! Check if best weapon can fire right now
        if (ctx.ready_weapon)
            ctx.has_ammo = ai.eAI_CanFire(ctx.ready_weapon);
        else if (ctx.best_weapon)
            ctx.has_ammo = false;

        //! Is the best weapon already in hands?
        EntityAI inHands = ai.GetItemInHands();
        if (inHands && ctx.best_weapon && inHands == ctx.best_weapon)
            ctx.weapon_in_hands = true;

        //! Enumerate full inventory for mags, ammo piles, and bandages
        array<EntityAI> inv = new array<EntityAI>;
        ai.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, inv);

        foreach (EntityAI ent : inv)
        {
            if (!ent)
                continue;

            //! Check for bandage items (Expansion Core method)
            ItemBase asItem;
            if (Class.CastTo(asItem, ent))
            {
                if (asItem.Expansion_CanBeUsedToBandage())
                {
                    ctx.has_bandage = true;
                    if (!ctx.best_bandage)
                        ctx.best_bandage = asItem;
                }
            }

            //! Check for magazines / ammo piles
            Magazine mag;
            if (Class.CastTo(mag, ent))
            {
                int ammo = mag.GetAmmoCount();
                if (mag.IsAmmoPile())
                {
                    if (ammo > 0)
                    {
                        ctx.has_ammo_pile = true;
                        if (!ctx.best_ammo_pile)
                            ctx.best_ammo_pile = mag;
                    }
                    continue;
                }

                if (ammo > 0)
                {
                    ctx.has_loaded_mag = true;
                    if (!ctx.best_mag)
                        ctx.best_mag = mag;
                }
                else
                {
                    ctx.has_empty_mag = true;
                    if (!ctx.best_mag && ctx.best_weapon && ai.eAI_HasWeaponForMagazine(mag))
                        ctx.best_mag = mag;
                }
            }
        }
    }

    static bool NeedsWeaponSetup(PredAI_Context ctx)
    {
        if (!ctx || !ctx.ai)
            return false;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg.weapon.enabled)
            return false;

        if (!ctx.has_firearm)
            return true;

        if (ctx.has_firearm && !ctx.has_loaded_weapon)
            return true;

        if (ctx.has_loaded_weapon && cfg.weapon.prefer_weapon_in_hands && !ctx.weapon_in_hands &&
            (ctx.has_target || ctx.stationary_seconds > 2.0))
            return true;

        return false;
    }

    static void Execute(PredAI_Context ctx, float dt)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg.weapon.enabled)
            return;

        if (ctx.since_weapon_setup < cfg.weapon.setup_cooldown)
            return;

        ctx.since_weapon_setup = 0.0;

        eAIBase ai = ctx.ai;

        //! Tell Expansion AI to re-evaluate what firearms are usable
        ai.eAI_EvaluateFirearmTypes(true);

        //! Try to get a ready (loaded) weapon first
        Weapon_Base ready = ai.eAI_GetAnyWeaponToUse(true, false);
        if (ready)
        {
            ctx.ready_weapon      = ready;
            ctx.best_weapon       = ready;
            ctx.has_loaded_weapon = true;
            ctx.has_weapon_ready  = true;
            TryPutWeaponInHands(ctx, ready);
            return;
        }

        //! No loaded weapon - try any weapon
        Weapon_Base anyWeapon = ai.eAI_GetAnyWeaponToUse(false, false);
        if (anyWeapon)
        {
            ctx.best_weapon = anyWeapon;

            //! If we have an ammo pile, try to fill any compatible mag via Expansion evaluate
            //! NOTE: eAI_FillAnyCompatibleMag does not exist in Expansion AI public API.
            //! We instead call EvaluateFirearmTypes which triggers Expansion's own mag-loading.
            if (ctx.has_ammo_pile || ctx.has_loaded_mag)
            {
                ai.eAI_EvaluateFirearmTypes(true);

                Weapon_Base nowReady = ai.eAI_GetAnyWeaponToUse(true, false);
                if (nowReady)
                {
                    TryPutWeaponInHands(ctx, nowReady);
                    ctx.AddScore(18, "weapon-now-ready-after-evaluate");
                    return;
                }
            }

            TryPutWeaponInHands(ctx, anyWeapon);
            ctx.AddScore(-2, "weapon-found-needs-ammo");

            //! Still no ammo - go loot
            if (!ctx.has_loaded_weapon)
                PredAI_LootDirector.Execute(ctx, dt);

            return;
        }

        //! No weapon at all - go loot
        PredAI_LootDirector.Execute(ctx, dt);
    }

    static bool TryPutWeaponInHands(PredAI_Context ctx, Weapon_Base weapon)
    {
        if (!ctx || !ctx.ai || !weapon)
            return false;

        eAIBase ai = ctx.ai;
        EntityAI hands = ai.GetItemInHands();
        if (hands == weapon)
        {
            ctx.weapon_in_hands = true;
            return true;
        }

        //! If holding a bandage and still bleeding, don't drop it
        ItemBase handsItem;
        if (Class.CastTo(handsItem, hands))
        {
            if (handsItem.Expansion_CanBeUsedToBandage() && ctx.should_bandage)
                return false;

            ai.eAI_TakeItemToInventory(handsItem);
        }

        if (ai.eAI_TakeItemToHands(weapon))
        {
            ctx.weapon_in_hands = true;
            ctx.AddScore(PredAI_Config.Get().scoring.weapon_ready, "weapon-to-hands");
            return true;
        }

        ctx.AddScore(-4, "weapon-to-hands-failed");
        return false;
    }
}
