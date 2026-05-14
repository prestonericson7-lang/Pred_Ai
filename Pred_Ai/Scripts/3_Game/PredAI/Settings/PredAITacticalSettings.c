class PredAITacticalSettings
{
    float break_los_health_pct;
    float break_los_min_distance;
    float break_los_search_radius;
    int   break_los_sample_count;
    bool  reload_behind_cover;
    float suppression_window_seconds;
    float doorway_stare_seconds;
    float doorway_wall_check_distance;
    float cohesion_radius_m;
    int   friend_death_survival_penalty;
    int   friend_alive_push_bonus;
    float push_bravery_aggression_min;

    void PredAITacticalSettings()
    {
        break_los_health_pct          = 0.45;
        break_los_min_distance        = 12.0;
        break_los_search_radius       = 35.0;
        break_los_sample_count        = 8;
        reload_behind_cover           = true;
        suppression_window_seconds    = 2.0;
        doorway_stare_seconds         = 3.5;
        doorway_wall_check_distance   = 1.0;
        cohesion_radius_m             = 45.0;
        friend_death_survival_penalty = 35;
        friend_alive_push_bonus       = 12;
        push_bravery_aggression_min   = 1.30;
    }
}
