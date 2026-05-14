class PredAIScoreSettings
{
    int bleeding_tick;
    int idle_tick;
    int moving_tick;
    int healing;
    int target_engaged;
    int target_killed;
    int looting_useful;
    int weapon_ready;
    int stuck;
    int body_camping;
    int risk_success;
    int bad_goal_repeat;
    int panic_decision_below;

    void PredAIScoreSettings()
    {
        bleeding_tick = -30;
        idle_tick = -6;
        moving_tick = 2;
        healing = 40;
        target_engaged = 8;
        target_killed = 60;
        looting_useful = 18;
        weapon_ready = 24;
        stuck = -18;
        body_camping = -40;
        risk_success = 20;
        bad_goal_repeat = -8;
        panic_decision_below = -75;
    }
}
