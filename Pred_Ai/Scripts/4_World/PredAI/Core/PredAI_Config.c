class PredAI_Config
{
    static ref PredAISettings s_Settings;

    static PredAISettings Get()
    {
        if (!s_Settings)
            Load();

        return s_Settings;
    }

    static void Load()
    {
        s_Settings = new PredAISettings();

        if (!FileExist(PredAI_Constants.SETTINGS_PATH))
        {
            MakeDirectory(PredAI_Constants.SETTINGS_DIR);
            s_Settings.EnsureDefaults();
            JsonFileLoader<PredAISettings>.JsonSaveFile(PredAI_Constants.SETTINGS_PATH, s_Settings);
            Print("[Pred_Ai] Created default settings at " + PredAI_Constants.SETTINGS_PATH);
            return;
        }

        JsonFileLoader<PredAISettings>.JsonLoadFile(PredAI_Constants.SETTINGS_PATH, s_Settings);
        s_Settings.EnsureDefaults();
        Print("[Pred_Ai] Loaded settings from " + PredAI_Constants.SETTINGS_PATH);
    }
}
