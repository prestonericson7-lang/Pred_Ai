class PredAI_Constants
{
    static const string VERSION = "11.0";
    static const string SETTINGS_DIR = "$profile:PredAI_Config";
    static const string SETTINGS_PATH = "$profile:PredAI_Config/PredAI_Settings.json";
}

enum PredAI_GoalType
{
    NONE = 0,
    EMERGENCY_MEDICAL = 1,
    MEDICAL = 2,
    COMBAT = 3,
    LOOT = 4,
    REPOSITION = 5,
    BREAK_BODY_CAMP = 6,
    IDLE_PATROL = 7,
    WEAPON_SETUP = 8,
    BREAK_LOS = 9,
    FLANK = 10
}
