class PredAI_Personality
{
    float bravery;
    float survival;
    float greed;
    float aggression;
    float discipline;

    void PredAI_Personality()
    {
        PredAISettings cfg = PredAI_Config.Get();
        PredAIPersonalitySettings p = cfg.personality;
        bravery    = Math.RandomFloatInclusive(p.bravery_min, p.bravery_max);
        survival   = Math.RandomFloatInclusive(p.survival_min, p.survival_max);
        greed      = Math.RandomFloatInclusive(p.greed_min, p.greed_max);
        aggression = Math.RandomFloatInclusive(p.aggression_min, p.aggression_max);
        discipline = Math.RandomFloatInclusive(p.discipline_min, p.discipline_max);
    }

    bool WillRiskBleedingFight(float health, float targetDistance)
    {
        PredAISettings cfg = PredAI_Config.Get();
        if (health < cfg.medical.brave_bleed_fight_health)
            return false;

        if (targetDistance > cfg.medical.bleed_fight_risk_distance)
            return false;

        return bravery + aggression > survival + 0.15;
    }

    bool WantsLootUnderPressure(float targetDistance)
    {
        PredAISettings cfg = PredAI_Config.Get();
        if (targetDistance < cfg.combat.close_range)
            return greed > 0.82 && discipline < 0.45;

        return greed > 0.62;
    }
}
