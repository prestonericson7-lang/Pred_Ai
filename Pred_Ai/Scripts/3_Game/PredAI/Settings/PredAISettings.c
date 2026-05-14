class PredAISettings
{
    int version;
    bool enabled;
    bool debug;
    bool debug_score;
    bool debug_goal;
    float tick_seconds;
    float score_eval_seconds;
    float brain_decision_cooldown;

    ref PredAIMedicalSettings medical;
    ref PredAICombatSettings combat;
    ref PredAINavigationSettings navigation;
    ref PredAILootSettings loot;
    ref PredAIWeaponSettings weapon;
    ref PredAIScoreSettings scoring;
    ref PredAIMemorySettings memory;
    ref PredAITacticalSettings tactical;
    ref PredAIPersonalitySettings personality;

    void PredAISettings()
    {
        version = 11;
        enabled = true;
        debug = false;
        debug_score = false;
        debug_goal = true;
        tick_seconds = 0.15;
        score_eval_seconds = 0.75;
        brain_decision_cooldown = 0.35;

        medical     = new PredAIMedicalSettings();
        combat      = new PredAICombatSettings();
        navigation  = new PredAINavigationSettings();
        loot        = new PredAILootSettings();
        weapon      = new PredAIWeaponSettings();
        scoring     = new PredAIScoreSettings();
        memory      = new PredAIMemorySettings();
        tactical    = new PredAITacticalSettings();
        personality = new PredAIPersonalitySettings();
    }

    void EnsureDefaults()
    {
        if (!medical)     medical     = new PredAIMedicalSettings();
        if (!combat)      combat      = new PredAICombatSettings();
        if (!navigation)  navigation  = new PredAINavigationSettings();
        if (!loot)        loot        = new PredAILootSettings();
        if (!weapon)      weapon      = new PredAIWeaponSettings();
        if (!scoring)     scoring     = new PredAIScoreSettings();
        if (!memory)      memory      = new PredAIMemorySettings();
        if (!tactical)    tactical    = new PredAITacticalSettings();
        if (!personality) personality = new PredAIPersonalitySettings();

        if (version < 11) version = 11;
        if (tick_seconds <= 0.0) tick_seconds = 0.15;
        if (score_eval_seconds <= 0.0) score_eval_seconds = 0.75;
        if (brain_decision_cooldown <= 0.0) brain_decision_cooldown = 0.35;
        if (navigation.move_threshold_meters <= 0.0) navigation.move_threshold_meters = 0.35;
        if (navigation.repath_seconds <= 0.0) navigation.repath_seconds = 1.15;
        if (navigation.idle_repath_seconds <= 0.0) navigation.idle_repath_seconds = 4.25;
        if (loot.scan_radius <= 0.0) loot.scan_radius = 60.0;
        if (loot.pickup_distance <= 0.0) loot.pickup_distance = 2.85;
        if (loot.max_objects_per_scan <= 0) loot.max_objects_per_scan = 112;
        if (loot.scan_cooldown <= 0.0) loot.scan_cooldown = 0.85;
        if (combat.direct_fire_cooldown <= 0.0) combat.direct_fire_cooldown = 0.45;
        if (weapon.setup_cooldown <= 0.0) weapon.setup_cooldown = 0.70;

        if (memory.threat_memory_seconds <= 0.0) memory.threat_memory_seconds = 30.0;
        if (memory.loot_scan_memo_seconds <= 0.0) memory.loot_scan_memo_seconds = 8.0;
        if (memory.teammate_death_memo_seconds <= 0.0) memory.teammate_death_memo_seconds = 90.0;
        if (memory.last_seen_decay_seconds <= 0.0) memory.last_seen_decay_seconds = 20.0;
        if (memory.purge_interval_seconds <= 0.0) memory.purge_interval_seconds = 5.0;
        if (memory.max_teammate_death_entries <= 0) memory.max_teammate_death_entries = 6;
        if (memory.max_loot_scan_entries <= 0) memory.max_loot_scan_entries = 6;
        if (memory.loot_scan_skip_radius <= 0.0) memory.loot_scan_skip_radius = 18.0;

        if (tactical.break_los_health_pct <= 0.0) tactical.break_los_health_pct = 0.45;
        if (tactical.break_los_min_distance <= 0.0) tactical.break_los_min_distance = 12.0;
        if (tactical.break_los_search_radius <= 0.0) tactical.break_los_search_radius = 35.0;
        if (tactical.break_los_sample_count <= 0) tactical.break_los_sample_count = 8;
        if (tactical.suppression_window_seconds <= 0.0) tactical.suppression_window_seconds = 2.0;
        if (tactical.doorway_stare_seconds <= 0.0) tactical.doorway_stare_seconds = 3.5;
        if (tactical.doorway_wall_check_distance <= 0.0) tactical.doorway_wall_check_distance = 1.0;
        if (tactical.cohesion_radius_m <= 0.0) tactical.cohesion_radius_m = 45.0;
        if (tactical.push_bravery_aggression_min <= 0.0) tactical.push_bravery_aggression_min = 1.30;
    }
}
