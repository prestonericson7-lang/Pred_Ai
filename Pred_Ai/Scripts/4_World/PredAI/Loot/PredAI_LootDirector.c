class PredAI_LootDirector
{
    static void Execute(PredAI_Context ctx, float dt)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg.loot.enabled)
            return;

        //! Don't loot during active gunfight unless desperate (bleeding or no weapon)
        if (ctx.has_target && !ctx.personality.WantsLootUnderPressure(ctx.target_distance) &&
            !ctx.should_bandage && ctx.has_loaded_weapon)
            return;

        if (ctx.since_loot_scan >= cfg.loot.scan_cooldown || !ctx.best_loot)
            ScanNearbyLoot(ctx, cfg);

        ItemBase item;
        if (!Class.CastTo(item, ctx.best_loot))
            return;

        if (!IsUsableGroundItem(item))
        {
            ctx.best_loot = null;
            return;
        }

        float dist = vector.Distance(ctx.ai.GetPosition(), item.GetPosition());
        if (dist > cfg.loot.pickup_distance)
        {
            ctx.ai.OverrideTargetPosition(item.GetPosition(), false, 1.0, true);
            ctx.AddScore(1, "moving-to-loot");
            return;
        }

        //! Bleeding with a bandage in reach - hard priority, take it to hands now
        if (item.Expansion_CanBeUsedToBandage() && ctx.should_bandage)
        {
            if (ctx.ai.eAI_TakeItemToHands(item))
            {
                ctx.AddScore(cfg.scoring.looting_useful + cfg.medical.no_bandage_loot_bias, "loot-bandage-to-hands");
                ctx.best_loot = null;
            }
            return;
        }

        //! Weapon on the ground - put in hands if unarmed or unloaded
        Weapon_Base weapon;
        if (Class.CastTo(weapon, item))
        {
            if (!ctx.has_firearm || !ctx.has_loaded_weapon)
            {
                if (ctx.ai.eAI_TakeItemToHands(weapon))
                {
                    ctx.AddScore(cfg.scoring.looting_useful + cfg.loot.weapon_need_bonus + cfg.loot.hands_weapon_bonus, "loot-weapon-to-hands");
                    ctx.best_loot = null;
                    ctx.ai.eAI_EvaluateFirearmTypes(true);
                    PredAI_WeaponDirector.Execute(ctx, dt);
                }
                return;
            }
        }

        //! Everything else (ammo, mags, food) goes to inventory
        if (ctx.ai.eAI_TakeItemToInventory(item))
        {
            int itemScore = ScoreGroundItem(ctx, item, cfg);
            ctx.AddScore(itemScore, "loot-picked-item");
            ctx.best_loot = null;
            ctx.ai.eAI_EvaluateFirearmTypes(true);
            PredAI_WeaponDirector.Execute(ctx, dt);
        }
    }

    static void ScanNearbyLoot(PredAI_Context ctx, PredAISettings cfg)
    {
        ctx.since_loot_scan = 0.0;
        ctx.best_loot = null;
        ctx.best_loot_score = -999999;

        vector here = ctx.ai.GetPosition();

        //! Skip the scan if we already swept this spot recently - keeps a 60-player
        //! server from grinding when 30 AI are huddled in one POI.
        if (ctx.memory && ctx.memory.HasRecentLootScan(here))
            return;

        if (ctx.memory)
            ctx.memory.RecordLootScan(here);

        array<Object> objects = new array<Object>;
        array<CargoBase> proxyCargos = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D(here, cfg.loot.scan_radius, objects, proxyCargos);

        int checked = 0;
        foreach (Object obj : objects)
        {
            if (checked >= cfg.loot.max_objects_per_scan)
                break;
            checked++;

            ItemBase item;
            if (!Class.CastTo(item, obj))
                continue;

            if (!IsUsableGroundItem(item))
                continue;

            int value = ScoreGroundItem(ctx, item, cfg);
            if (value <= 0)
                continue;

            float dist = vector.Distance(ctx.ai.GetPosition(), item.GetPosition());
            int score = value - Math.Round(dist);

            if (score > ctx.best_loot_score)
            {
                ctx.best_loot_score = score;
                ctx.best_loot = item;
            }
        }
    }

    static bool IsUsableGroundItem(ItemBase item)
    {
        if (!item)
            return false;

        if (item.IsDamageDestroyed() || item.IsSetForDeletion())
            return false;

        //! Don't pick up items already owned by a player
        if (item.GetHierarchyRootPlayer())
            return false;

        return true;
    }

    static int ScoreGroundItem(PredAI_Context ctx, ItemBase item, PredAISettings cfg)
    {
        if (!ctx || !ctx.ai || !item)
            return 0;

        if (item.IsDamageDestroyed() || item.IsSetForDeletion())
            return cfg.loot.ruined_penalty;

        //! Bandage - highest priority when bleeding, still good if no bandage at all
        if (item.Expansion_CanBeUsedToBandage())
        {
            if (ctx.should_bandage)
                return cfg.loot.medical_weight + cfg.medical.no_bandage_loot_bias + 200;

            if (!ctx.has_bandage)
                return cfg.loot.medical_weight;

            return 15;
        }

        //! Weapon - score by readiness
        Weapon_Base weapon;
        if (Class.CastTo(weapon, item))
        {
            //! eAI_CanFire tells us if the weapon on the ground could fire right now
            //! We can't check ammo-in-weapon without picking it up; use base score.
            if (!ctx.has_firearm)
                return cfg.loot.weapon_weight + cfg.loot.weapon_need_bonus + 80;

            if (!ctx.has_loaded_weapon)
                return cfg.loot.weapon_weight + cfg.loot.hands_weapon_bonus;

            return Math.Round(cfg.loot.weapon_weight * 0.5);
        }

        //! Magazine / ammo pile
        Magazine mag;
        if (Class.CastTo(mag, item))
        {
            int ammoCount = mag.GetAmmoCount();
            if (mag.IsAmmoPile())
            {
                if (ammoCount <= 0)
                    return 0;

                if (ctx.has_unloaded_weapon || ctx.has_empty_mag || ctx.has_firearm)
                    return cfg.loot.ammo_weight + cfg.loot.compatible_ammo_bonus + 80;

                return cfg.loot.ammo_weight;
            }

            if (ammoCount > 0)
            {
                if (ctx.ai.eAI_HasWeaponForMagazine(mag))
                    return cfg.loot.ammo_weight + cfg.loot.loaded_mag_bonus + 120;

                if (!ctx.has_loaded_weapon)
                    return cfg.loot.ammo_weight + 35;

                return cfg.loot.ammo_weight;
            }

            //! Empty mag - only valuable if AI has a gun for it
            if (ctx.ai.eAI_HasWeaponForMagazine(mag))
                return cfg.loot.empty_mag_weight + 70;

            if (ctx.has_ammo_pile && ctx.has_firearm)
                return cfg.loot.empty_mag_weight + 35;

            return cfg.weapon.empty_mag_only_penalty;
        }

        //! Melee weapon fallback
        if (item.Expansion_IsMeleeWeapon())
        {
            if (!ctx.has_firearm)
                return 70;

            return 20;
        }

        //! Food - minor value
        if (item.IsInherited(Edible_Base))
            return cfg.loot.food_weight;

        //! String-based fallback for ammo boxes etc.
        string type = item.GetType();
        type.ToLower();

        if (type.Contains("ammobox"))
            return cfg.loot.ammo_weight + 35;

        if (type.Contains("ammo"))
            return cfg.loot.ammo_weight;

        if (type.Contains("mag"))
            return cfg.loot.empty_mag_weight;

        return 1;
    }
}
