class PredAIWeaponSettings
{
    bool enabled;
    bool prefer_weapon_in_hands;
    bool fill_compatible_mags;
    bool take_loaded_weapon_to_hands;
    float setup_cooldown;
    int empty_mag_only_penalty;
    int ready_weapon_score_bonus;

    void PredAIWeaponSettings()
    {
        enabled = true;
        prefer_weapon_in_hands = true;
        fill_compatible_mags = true;
        take_loaded_weapon_to_hands = true;
        setup_cooldown = 0.70;
        empty_mag_only_penalty = -35;
        ready_weapon_score_bonus = 60;
    }
}
