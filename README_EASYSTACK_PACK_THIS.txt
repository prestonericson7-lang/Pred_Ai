EASYSTACK
=========

What it does
------------
1. Auto-stack on pickup
   When you pick up a quantity item (ammo piles, rags, nails, quantity
   food, liquid containers, etc.) it is poured into your existing stacks
   of the same item automatically. The FULLEST existing stack is topped
   off first. Any leftover simply stays in the picked-up item as its own
   stack - no extra partial item is ever spawned.
   (Bottle example: have a 70% bottle, pick up a 50% bottle -> the 70%
   becomes 100%, the picked-up one is left at 20%. Liquids of different
   types are never mixed.)

2. Ammo cap = 99
   Every loose ammo pile (all calibers) now stacks up to 99 rounds.
   Real weapon magazines are NOT changed - a 30-round mag still holds 30.

3. Safe durability combine
   Pick up a weapon or clothing item that is the SAME type and the SAME
   damage tier as one you already have, and they combine into ONE item
   that is exactly one tier better:
       badly damaged + badly damaged -> damaged
       damaged       + damaged       -> worn
       worn          + worn          -> pristine
   Different tiers do NOT combine. This ONLY happens when BOTH copies are
   completely empty (no attachments, no magazine, no chambered/internal
   ammo, no cargo). If either one is holding anything, EasyStack leaves
   both items exactly as they are. Nothing is ever moved or deleted out
   of an item, so attachments / mags / pocket contents can never be lost.

Notes / limitations
-------------------
- This is server-authoritative and changes nothing else in the game.
- Items that are not quantity-based in vanilla (e.g. an individual
  bandage, which vanilla treats as a single discrete item) are not made
  newly stackable - doing so would require rewriting their configs and
  risks breaking crafting, so it is intentionally not done. Anything
  vanilla already treats as a quantity/stack (ammo, rags, nails, etc.)
  is handled.
- Behaviour can be toggled in
  EasyStack/scripts/4_World/EasyStack/EasyStack_Settings.c

Packing
-------
The folder to give to DayZ Tools Addon Builder is:

    EasyStack

It contains:
    - config.cpp
    - scripts/4_World/EasyStack/*.c
    - scripts/5_Mission/EasyStack/*.c

Addon Builder output name:
    EasyStack.pbo

After packing:
    - Put EasyStack.pbo into SERVER_UPLOAD_AFTER_PACKING/EasyStack/addons
    - Sign it, put the .bisign beside it
    - Put your .bikey in SERVER_UPLOAD_AFTER_PACKING/EasyStack/keys and the
      server root keys folder
    - Upload SERVER_UPLOAD_AFTER_PACKING/EasyStack to your host
    - Commandline mod list uses ;EasyStack  (no "@")

When it works, server logs show:
    [EasyStack] EasyStack loaded.
