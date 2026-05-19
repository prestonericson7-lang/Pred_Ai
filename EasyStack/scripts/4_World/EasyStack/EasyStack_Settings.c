// EasyStack - tunable behaviour. Kept as script constants so there is no
// external file to load (one less thing that can fail or be lost).
class EasyStack_Settings
{
    // Master switch. If false, EasyStack does nothing at all.
    static const bool ENABLED = true;

    // Auto-merge quantity items on pickup (ammo piles, rags, nails, food
    // with a quantity, liquid containers, etc.). Fills the fullest existing
    // stack first; any leftover stays in the picked-up item.
    static const bool ENABLE_AUTO_QUANTITY_STACK = true;

    // Auto-combine two of the SAME empty weapon/clothing of the SAME damage
    // tier into one item that is one tier better. Only ever runs when BOTH
    // copies are completely empty, so nothing can be lost.
    static const bool ENABLE_DURABILITY_COMBINE = true;

    // Print EasyStack activity to the server script log.
    static const bool LOG = true;
}
