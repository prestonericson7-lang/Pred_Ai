class PredAI_Debug
{
    static void Log(string msg)
    {
        PredAISettings cfg = PredAI_Config.Get();
        if (cfg && cfg.debug)
            Print("[Pred_Ai] " + msg);
    }

    static void Goal(string msg)
    {
        PredAISettings cfg = PredAI_Config.Get();
        if (cfg && cfg.debug_goal)
            Print("[Pred_Ai][Goal] " + msg);
    }

    static void Score(string msg)
    {
        PredAISettings cfg = PredAI_Config.Get();
        if (cfg && cfg.debug_score)
            Print("[Pred_Ai][Score] " + msg);
    }
}
