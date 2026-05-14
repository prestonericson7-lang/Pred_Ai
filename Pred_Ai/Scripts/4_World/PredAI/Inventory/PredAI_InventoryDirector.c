class PredAI_InventoryDirector
{
    static int ScoreItem(eAIBase ai, ItemBase item)
    {
        if (!ai || !item)
            return 0;

        PredAISettings cfg = PredAI_Config.Get();
        if (item.IsDamageDestroyed() || item.IsSetForDeletion())
            return cfg.loot.ruined_penalty;

        if (item.Expansion_CanBeUsedToBandage())
            return cfg.loot.medical_weight;

        Weapon_Base weapon;
        if (Class.CastTo(weapon, item))
        {
            Magazine maybeMag;
            if (weapon.Expansion_HasAmmo(maybeMag))
                return cfg.loot.weapon_weight + cfg.weapon.ready_weapon_score_bonus;

            if (!ai.eAI_HasWeaponInInventory(false))
                return cfg.loot.weapon_weight + cfg.loot.weapon_need_bonus;

            return cfg.loot.weapon_weight;
        }

        Magazine mag;
        if (Class.CastTo(mag, item))
        {
            int ammoCount = mag.GetAmmoCount();
            if (mag.IsAmmoPile())
            {
                if (ammoCount > 0)
                    return cfg.loot.ammo_weight + cfg.loot.compatible_ammo_bonus;

                return 0;
            }

            if (ammoCount > 0)
            {
                if (ai.eAI_HasWeaponForMagazine(mag))
                    return cfg.loot.ammo_weight + cfg.loot.loaded_mag_bonus;

                return cfg.loot.ammo_weight;
            }

            if (ai.eAI_HasWeaponForMagazine(mag))
                return cfg.loot.empty_mag_weight;

            return cfg.weapon.empty_mag_only_penalty;
        }

        string type = item.GetType();
        type.ToLower();

        if (type.Contains("ammobox"))
            return 35;

        if (type.Contains("ammo"))
            return cfg.loot.ammo_weight;

        if (type.Contains("mag"))
            return cfg.loot.empty_mag_weight;

        if (item.Expansion_IsMeleeWeapon())
            return 55;

        if (item.IsInherited(Edible_Base))
            return cfg.loot.food_weight;

        return 1;
    }

    static bool IsWeaponItem(ItemBase item)
    {
        Weapon_Base weapon;
        return Class.CastTo(weapon, item);
    }

    static bool IsLoadedMagazineOrAmmoPile(ItemBase item)
    {
        Magazine mag;
        if (!Class.CastTo(mag, item))
            return false;

        return mag.GetAmmoCount() > 0;
    }

    static bool IsEmptyMagazine(ItemBase item)
    {
        Magazine mag;
        if (!Class.CastTo(mag, item))
            return false;

        return !mag.IsAmmoPile() && mag.GetAmmoCount() <= 0;
    }
}
