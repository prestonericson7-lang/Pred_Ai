// EasyStack - the single hook into the game. When any item enters someone's
// inventory we hand it to EasyStack_Logic, which decides (server-side only)
// whether to auto-stack or durability-combine it.
modded class ItemBase
{
    override void OnInventoryEnter(Man player)
    {
        super.OnInventoryEnter(player);
        EasyStack_Logic.OnItemEnteredInventory(this, player);
    }
}
