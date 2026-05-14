class PredAICombatSettings
{
    float min_threat_to_engage;
    float close_range;
    float mid_range;
    float long_range;
    bool force_weapon_select;
    bool allow_direct_fire_command;
    float direct_fire_cooldown;
    bool prefer_cover_reposition;
    float anti_body_camp_seconds;
    float flank_after_stationary_seconds;
    float combat_reposition_seconds;
    float close_sidestep_distance;
    float flank_distance;

    void PredAICombatSettings()
    {
        min_threat_to_engage = 0.18;
        close_range = 15.0;
        mid_range = 60.0;
        long_range = 150.0;
        force_weapon_select = true;
        allow_direct_fire_command = true;
        direct_fire_cooldown = 0.45;
        prefer_cover_reposition = true;
        anti_body_camp_seconds = 5.0;
        flank_after_stationary_seconds = 2.25;
        combat_reposition_seconds = 2.75;
        close_sidestep_distance = 4.0;
        flank_distance = 12.0;
    }
}
