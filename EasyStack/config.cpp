// EasyStack - automatic stacking + safe durability combining for DayZ.
// No "@" is used anywhere: server mods are referenced by the plain folder
// name "EasyStack" (the "@" prefix is only a client launcher convention).

class CfgPatches
{
    class EasyStack
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Weapons"
        };
    };
};

class CfgMods
{
    class EasyStack
    {
        dir = "EasyStack";
        name = "EasyStack";
        type = "mod";
        dependencies[] =
        {
            "World",
            "Mission"
        };

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "EasyStack/scripts/4_World"
                };
            };

            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "EasyStack/scripts/5_Mission"
                };
            };
        };
    };
};

// ---------------------------------------------------------------------------
// Ammo cap = 99 for loose ammo piles (ALL ammunition).
//
// "count" on an ammo pile is its maximum stack size. We raise the shared base
// (Ammunition_Base) so any pile - vanilla or modded - that does not override
// "count" inherits 99, and we also raise every known vanilla pile explicitly
// because several of them set their own "count".
//
// Weapon MAGAZINE capacities are NOT touched: real magazines derive from
// Magazine_Base, not Ammunition_Base, so a 30-round mag still holds 30.
// ---------------------------------------------------------------------------
class CfgMagazines
{
    class Ammunition_Base
    {
        count = 99;
    };

    class Ammo_762x39:        Ammunition_Base { count = 99; };
    class Ammo_762x39Tracer:  Ammunition_Base { count = 99; };
    class Ammo_762x54:        Ammunition_Base { count = 99; };
    class Ammo_762x54Tracer:  Ammunition_Base { count = 99; };
    class Ammo_308Win:        Ammunition_Base { count = 99; };
    class Ammo_308WinTracer:  Ammunition_Base { count = 99; };
    class Ammo_556x45:        Ammunition_Base { count = 99; };
    class Ammo_556x45Tracer:  Ammunition_Base { count = 99; };
    class Ammo_545x39:        Ammunition_Base { count = 99; };
    class Ammo_545x39Tracer:  Ammunition_Base { count = 99; };
    class Ammo_9x19:          Ammunition_Base { count = 99; };
    class Ammo_9x39:          Ammunition_Base { count = 99; };
    class Ammo_9x39AP:        Ammunition_Base { count = 99; };
    class Ammo_380:           Ammunition_Base { count = 99; };
    class Ammo_357:           Ammunition_Base { count = 99; };
    class Ammo_45ACP:         Ammunition_Base { count = 99; };
    class Ammo_22:            Ammunition_Base { count = 99; };
    class Ammo_12gaPellets:   Ammunition_Base { count = 99; };
    class Ammo_12gaSlug:      Ammunition_Base { count = 99; };
    class Ammo_12gaRubberSlug:Ammunition_Base { count = 99; };
    class Ammo_46x30:         Ammunition_Base { count = 99; };
    class Ammo_338:           Ammunition_Base { count = 99; };
    class Ammo_303:           Ammunition_Base { count = 99; };
    class Ammo_40mm_Explosive:Ammunition_Base { count = 99; };
};
