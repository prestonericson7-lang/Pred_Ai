class PredAI_Context
{
    eAIBase ai;
    ref PredAI_Personality personality;
    ref PredAI_Memory memory;

    float tick_accumulator;
    float decision_accumulator;
    float score_accumulator;

    vector last_position;
    float stationary_seconds;
    float since_repath;
    float since_idle_action;
    float since_bandage_attempt;
    float since_loot_scan;
    float since_combat_reposition;
    float since_direct_fire;
    float since_weapon_setup;
    float active_goal_seconds;
    float body_camp_seconds;
    float target_seen_seconds;
    float last_hit_seconds;
    float doorway_stare_seconds;

    int score;
    int last_reported_score;
    int active_goal;
    int last_goal;
    string active_goal_reason;

    //! Sensor state - refreshed every tick by PredAI_Sensors
    bool should_bandage;
    bool has_bandage;
    bool has_target;
    bool target_is_body;
    bool has_firearm;
    bool has_weapon_ready;
    bool has_loaded_weapon;
    bool has_unloaded_weapon;
    bool has_ammo;
    bool has_loaded_mag;
    bool has_empty_mag;
    bool has_ammo_pile;
    bool weapon_in_hands;
    bool suppression_active;

    float health;
    float blood01;
    float target_distance;
    float target_threat;

    EntityAI last_hit_source;
    EntityAI target_entity;
    EntityAI best_loot;
    int best_loot_score;
    int friendlies_alive_nearby;

    ItemBase best_bandage;
    Weapon_Base best_weapon;
    Weapon_Base ready_weapon;
    Magazine best_mag;
    Magazine best_ammo_pile;

    void PredAI_Context(eAIBase unit)
    {
        ai = unit;
        personality = new PredAI_Personality();
        memory      = new PredAI_Memory();
        if (ai)
            last_position = ai.GetPosition();

        score = 0;
        last_reported_score = 0;
        target_distance = 9999.0;
        active_goal = PredAI_GoalType.NONE;
        last_goal = PredAI_GoalType.NONE;
        since_bandage_attempt = 999.0;
        since_loot_scan = 999.0;
        since_combat_reposition = 999.0;
        since_direct_fire = 999.0;
        since_weapon_setup = 999.0;
        since_repath = 999.0;
        since_idle_action = 999.0;
        last_hit_seconds = 999.0;
        doorway_stare_seconds = 0.0;
        suppression_active = false;
        friendlies_alive_nearby = 0;
    }

    void AddScore(int value, string reason)
    {
        score += value;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg || !cfg.debug_score)
            return;

        if (score >= last_reported_score + 15 || score <= last_reported_score - 15)
        {
            last_reported_score = score;
            if (ai)
                PredAI_Debug.Score(ai.ToString() + " " + value.ToString() + " " + reason + " total=" + score.ToString());
        }
    }

    void SetGoal(int goal, string reason)
    {
        if (goal == active_goal)
            return;

        last_goal = active_goal;
        active_goal = goal;
        active_goal_seconds = 0.0;
        active_goal_reason = reason;

        if (ai)
            PredAI_Debug.Goal(ai.ToString() + " -> " + PredAI_GoalName.Get(goal) + " " + reason);
    }
}

class PredAI_GoalName
{
    static string Get(int goal)
    {
        switch (goal)
        {
            case PredAI_GoalType.EMERGENCY_MEDICAL: return "EMERGENCY_MEDICAL";
            case PredAI_GoalType.MEDICAL:           return "MEDICAL";
            case PredAI_GoalType.COMBAT:            return "COMBAT";
            case PredAI_GoalType.LOOT:              return "LOOT";
            case PredAI_GoalType.REPOSITION:        return "REPOSITION";
            case PredAI_GoalType.BREAK_BODY_CAMP:   return "BREAK_BODY_CAMP";
            case PredAI_GoalType.IDLE_PATROL:       return "IDLE_PATROL";
            case PredAI_GoalType.WEAPON_SETUP:      return "WEAPON_SETUP";
            case PredAI_GoalType.BREAK_LOS:         return "BREAK_LOS";
            case PredAI_GoalType.FLANK:             return "FLANK";
        }
        return "NONE";
    }
}
