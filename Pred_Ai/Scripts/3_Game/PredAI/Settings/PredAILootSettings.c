class PredAILootSettings
{
    bool enabled;
    float scan_radius;
    float pickup_distance;
    int max_objects_per_scan;
    float scan_cooldown;
    int weapon_weight;
    int ammo_weight;
    int empty_mag_weight;
    int medical_weight;
    int food_weight;
    int ruined_penalty;
    int loaded_mag_bonus;
    int compatible_ammo_bonus;
    int weapon_need_bonus;
    int hands_weapon_bonus;

    void PredAILootSettings()
    {
        enabled = true;
        scan_radius = 60.0;
        pickup_distance = 2.85;
        max_objects_per_scan = 112;
        scan_cooldown = 0.85;
        weapon_weight = 135;
        ammo_weight = 125;
        empty_mag_weight = 45;
        medical_weight = 190;
        food_weight = 25;
        ruined_penalty = -1000;
        loaded_mag_bonus = 55;
        compatible_ammo_bonus = 70;
        weapon_need_bonus = 95;
        hands_weapon_bonus = 40;
    }
}
