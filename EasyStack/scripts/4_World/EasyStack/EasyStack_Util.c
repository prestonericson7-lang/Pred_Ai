// EasyStack - low level, side-effect-free helpers.
//
// Every "is it safe?" check in here is written to FAIL CLOSED: if anything
// is uncertain it returns the answer that prevents a merge/delete, so the
// worst case is "items did not combine", never "items were lost".
class EasyStack_Util
{
    // ----- quantity abstraction (ammo piles use ammo count, everything
    //       else uses the generic item quantity) ---------------------------

    static bool IsAmmoPile(ItemBase it)
    {
        if (!it) return false;
        if (!Magazine.Cast(it)) return false;
        // Real weapon magazines derive from Magazine_Base; loose ammo piles
        // derive from Ammunition_Base. Only piles stack.
        return it.IsKindOf("Ammunition_Base");
    }

    static int GetQty(ItemBase it)
    {
        if (!it) return 0;
        Magazine mag;
        if (Class.CastTo(mag, it))
            return mag.GetAmmoCount();
        return (int)Math.Round(it.GetQuantity());
    }

    static int GetQtyMax(ItemBase it)
    {
        if (!it) return 0;
        Magazine mag;
        if (Class.CastTo(mag, it))
            return mag.GetAmmoMax();
        return (int)Math.Round(it.GetQuantityMax());
    }

    static void SetQty(ItemBase it, int v)
    {
        if (!it) return;
        if (v < 0) v = 0;
        Magazine mag;
        if (Class.CastTo(mag, it))
        {
            mag.ServerSetAmmoCount(v);
            return;
        }
        // destroy_config = false: never let SetQuantity auto-delete the item.
        // EasyStack deletes a drained source itself, and only after it has
        // verified the source holds nothing.
        it.SetQuantity(v, false);
    }

    // ----- "is this a quantity stackable?" -------------------------------

    static bool IsStackable(ItemBase it)
    {
        if (!it) return false;
        if (IsAmmoPile(it)) return true;
        if (Magazine.Cast(it)) return false; // a real magazine, do not merge
        if (Weapon_Base.Cast(it)) return false;
        if (Clothing.Cast(it)) return false;
        return it.GetQuantityMax() > 1;
    }

    // Two same-type stackables may only merge if they belong to the same
    // logical group. The only vanilla gotcha is liquid containers: never
    // mix two different liquids.
    static bool SameStackGroup(ItemBase a, ItemBase b)
    {
        if (!a || !b) return false;
        if (a.GetType() != b.GetType()) return false;
        if (a.IsLiquidContainer() && b.IsLiquidContainer())
        {
            if (a.GetLiquidType() != b.GetLiquidType()) return false;
        }
        return true;
    }

    // ----- "is this an empty weapon/clothing we may durability-combine?" --

    static bool IsDurabilityCombinable(ItemBase it)
    {
        if (!it) return false;
        if (IsStackable(it)) return false;
        if (Weapon_Base.Cast(it)) return true;
        if (Clothing.Cast(it)) return true;
        return false;
    }

    // TRUE only when the entity is provably holding nothing: no attachments,
    // no cargo, and (for weapons) no magazine, empty chamber(s) and empty
    // internal magazine(s). Any doubt -> FALSE -> no combine -> no loss.
    static bool IsCompletelyEmpty(EntityAI e)
    {
        if (!e) return false;

        GameInventory inv = e.GetInventory();
        if (inv)
        {
            if (inv.AttachmentCount() > 0)
                return false;

            CargoBase cargo = inv.GetCargo();
            if (cargo && cargo.GetItemCount() > 0)
                return false;
        }

        Weapon_Base wpn;
        if (Class.CastTo(wpn, e))
        {
            int muzzles = wpn.GetMuzzleCount();
            for (int m = 0; m < muzzles; m++)
            {
                if (!wpn.IsChamberEmpty(m))
                    return false;
                if (wpn.GetMagazine(m))
                    return false;
                if (wpn.GetInternalMagazineCartridgeCount(m) > 0)
                    return false;
            }
        }

        return true;
    }

    // ----- damage tier helpers -------------------------------------------
    //
    // DayZ health levels: 0 Pristine, 1 Worn, 2 Damaged, 3 Badly damaged,
    // 4 Ruined. Combining promotes ONE tier (level - 1). We place the
    // result at the midpoint of the new tier using the standard vanilla
    // health-level fractions so it lands solidly inside that tier.

    static float HealthLevelMidpoint01(int level)
    {
        switch (level)
        {
            case 0: return 0.85; // Pristine        (0.70 .. 1.00]
            case 1: return 0.60; // Worn            (0.50 .. 0.70]
            case 2: return 0.40; // Damaged         (0.30 .. 0.50]
            case 3: return 0.15; // Badly damaged   (0.00 .. 0.30]
        }
        return 0.0;              // Ruined
    }

    // ----- sorting --------------------------------------------------------

    // Descending by current quantity so the FULLEST existing stack gets
    // topped off first (matches the "fill bottle B to full first" intent).
    static void SortByQtyDesc(array<ItemBase> a)
    {
        if (!a) return;
        int n = a.Count();
        for (int i = 0; i < n - 1; i++)
        {
            for (int j = 0; j < n - 1 - i; j++)
            {
                if (GetQty(a[j]) < GetQty(a[j + 1]))
                {
                    ItemBase tmp = a[j];
                    a[j] = a[j + 1];
                    a[j + 1] = tmp;
                }
            }
        }
    }
}
