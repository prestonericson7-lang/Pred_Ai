class CfgPatches
{
    class Pred_Ai
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "DZ_Characters",
            "JM_CF_Scripts",
            "DayZExpansion_Core_Scripts",
            "DayZExpansion_AI_Preload",
            "DayZExpansion_AI_Scripts"
        };
    };
};

class CfgMods
{
    class Pred_Ai
    {
        dir = "Pred_Ai";
        picture = "";
        action = "";
        hideName = 0;
        hidePicture = 0;
        name = "Pred_Ai";
        credits = "Preston";
        author = "Preston";
        authorID = "0";
        version = "11.0";
        type = "mod";
        dependencies[] = {"Game", "World", "Mission"};

        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] = {"Pred_Ai/Scripts/3_Game"};
            };
            class worldScriptModule
            {
                value = "";
                files[] = {"Pred_Ai/Scripts/4_World"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"Pred_Ai/Scripts/5_Mission"};
            };
        };
    };
};
