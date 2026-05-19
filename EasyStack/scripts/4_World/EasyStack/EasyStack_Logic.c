// EasyStack - core logic. Server authority only.
//
// Flow: ItemBase.OnInventoryEnter -> OnItemEnteredInventory (just schedules)
//       -> ProcessDeferred (re-validates) -> ProcessItem (does the work).
//
// Work is deferred one short tick so the inventory operation that triggered
// it has fully settled before we touch quantities or delete anything.
class EasyStack_Logic
{
    private static bool s_Busy = false;

    static void OnItemEnteredInventory(ItemBase item, Man player)
    {
        if (!EasyStack_Settings.ENABLED)
            return;
        if (!GetGame() || !GetGame().IsServer())
            return;
        if (!item)
            return;

        PlayerBase pb = PlayerBase.Cast(player);
        if (!pb)
            return;

        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(
            EasyStack_Logic.ProcessDeferred, 100, false, item, pb);
    }

    static void ProcessDeferred(ItemBase item, PlayerBase player)
    {
        // Re-validate everything: the world may have changed during the tick.
        if (!item || !player)
            return;
        if (!player.IsAlive())
            return;
        if (item.GetHierarchyRootPlayer() != player)
            return; // item is no longer in this player's inventory

        // Re-entrancy guard. Our own SetQty/Delete must never recurse into
        // a fresh processing pass.
        if (s_Busy)
            return;

        s_Busy = true;
        ProcessItem(item, player);
        s_Busy = false;
    }

    static void ProcessItem(ItemBase item, PlayerBase player)
    {
        if (EasyStack_Settings.ENABLE_AUTO_QUANTITY_STACK && EasyStack_Util.IsStackable(item))
        {
            DoQuantityStack(item, player);
            return;
        }

        if (EasyStack_Settings.ENABLE_DURABILITY_COMBINE && EasyStack_Util.IsDurabilityCombinable(item))
        {
            DoDurabilityCombine(item, player);
            return;
        }
    }

    // -------------------------------------------------------------------
    // Quantity stacking. Pours the picked-up item into existing same-type
    // stacks (fullest first). Leftover simply stays in the picked-up item
    // as its own stack - no extra partial item is ever created.
    // -------------------------------------------------------------------
    static void DoQuantityStack(ItemBase item, PlayerBase player)
    {
        GameInventory inv = player.GetInventory();
        if (!inv)
            return;

        string type = item.GetType();

        array<EntityAI> all = new array<EntityAI>;
        inv.EnumerateInventory(InventoryTraversalType.PREORDER, all);

        array<ItemBase> targets = new array<ItemBase>;
        foreach (EntityAI e : all)
        {
            ItemBase it = ItemBase.Cast(e);
            if (!it || it == item)
                continue;
            if (it.GetType() != type)
                continue;
            if (!EasyStack_Util.SameStackGroup(item, it))
                continue;
            if (EasyStack_Util.GetQty(it) >= EasyStack_Util.GetQtyMax(it))
                continue; // already full
            targets.Insert(it);
        }

        if (targets.Count() == 0)
            return;

        EasyStack_Util.SortByQtyDesc(targets);

        foreach (ItemBase tgt : targets)
        {
            int srcQ = EasyStack_Util.GetQty(item);
            if (srcQ <= 0)
                break;

            int tgtQ = EasyStack_Util.GetQty(tgt);
            int space = EasyStack_Util.GetQtyMax(tgt) - tgtQ;
            if (space <= 0)
                continue;

            int move = Math.Min(space, srcQ);
            if (move <= 0)
                continue;

            EasyStack_Util.SetQty(tgt, tgtQ + move);
            EasyStack_Util.SetQty(item, srcQ - move);

            EasyStack_Log.Msg("Stacked " + move.ToString() + "x " + type
                + " -> " + (tgtQ + move).ToString() + "/"
                + EasyStack_Util.GetQtyMax(tgt).ToString());
        }

        // Only delete the source if it is truly drained AND holds nothing.
        if (EasyStack_Util.GetQty(item) <= 0 && EasyStack_Util.IsCompletelyEmpty(item))
        {
            GetGame().ObjectDelete(item);
            EasyStack_Log.Msg("Removed emptied " + type + " stack");
        }
    }

    // -------------------------------------------------------------------
    // Durability combine. Only same-type, same-tier, BOTH-empty weapons or
    // clothing. Promotes exactly one tier (badly damaged + badly damaged ->
    // damaged, damaged + damaged -> worn, worn + worn -> pristine). Because
    // both items are provably empty, deleting one loses nothing.
    // -------------------------------------------------------------------
    static void DoDurabilityCombine(ItemBase item, PlayerBase player)
    {
        int lvl = item.GetHealthLevel();
        // Only Worn(1), Damaged(2), Badly damaged(3) can be promoted.
        // Pristine(0) is already best; Ruined(4) is never combined.
        if (lvl < 1 || lvl > 3)
            return;

        if (!EasyStack_Util.IsCompletelyEmpty(item))
            return; // safety first: never risk contents

        GameInventory inv = player.GetInventory();
        if (!inv)
            return;

        string type = item.GetType();

        array<EntityAI> all = new array<EntityAI>;
        inv.EnumerateInventory(InventoryTraversalType.PREORDER, all);

        ItemBase partner = null;
        foreach (EntityAI e : all)
        {
            ItemBase it = ItemBase.Cast(e);
            if (!it || it == item)
                continue;
            if (it.GetType() != type)
                continue;
            if (it.GetHealthLevel() != lvl)
                continue; // must be the SAME damage tier
            if (!EasyStack_Util.IsCompletelyEmpty(it))
                continue;
            partner = it;
            break;
        }

        if (!partner)
            return;

        int newLvl = lvl - 1; // exactly one tier better
        float norm = EasyStack_Util.HealthLevelMidpoint01(newLvl);

        // Keep the item already in the inventory, upgrade it, drop the
        // freshly-picked duplicate. Both are empty -> zero item loss.
        partner.SetHealth01("", "Health", norm);
        GetGame().ObjectDelete(item);

        EasyStack_Log.Msg("Combined two " + type + " at tier " + lvl.ToString()
            + " -> tier " + newLvl.ToString());
    }
}
