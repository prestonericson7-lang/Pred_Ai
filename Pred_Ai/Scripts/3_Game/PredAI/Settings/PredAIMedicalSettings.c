class PredAIMedicalSettings
{
    float health_critical;
    float health_low;
    float blood_low01;
    bool force_bandage;
    float bandage_action_cooldown;
    float bleed_fight_risk_distance;
    float brave_bleed_fight_health;
    float emergency_move_distance;
    int no_bandage_loot_bias;

    void PredAIMedicalSettings()
    {
        health_critical = 38.0;
        health_low = 60.0;
        blood_low01 = 0.70;
        force_bandage = true;
        bandage_action_cooldown = 1.50;
        bleed_fight_risk_distance = 10.0;
        brave_bleed_fight_health = 55.0;
        emergency_move_distance = 12.0;
        no_bandage_loot_bias = 260;
    }
}
