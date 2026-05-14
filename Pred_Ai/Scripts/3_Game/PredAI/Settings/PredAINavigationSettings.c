class PredAINavigationSettings
{
    bool anti_stuck_enabled;
    float stuck_seconds;
    float repath_seconds;
    float idle_repath_seconds;
    float body_camp_break_distance;
    float idle_goal_move_distance;
    bool surface_snap_y;
    float move_threshold_meters;

    void PredAINavigationSettings()
    {
        anti_stuck_enabled = true;
        stuck_seconds = 3.25;
        repath_seconds = 1.15;
        idle_repath_seconds = 4.25;
        body_camp_break_distance = 32.0;
        idle_goal_move_distance = 30.0;
        surface_snap_y = true;
        move_threshold_meters = 0.35;
    }
}
