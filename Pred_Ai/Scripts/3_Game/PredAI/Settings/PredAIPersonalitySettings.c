class PredAIPersonalitySettings
{
    float bravery_min;
    float bravery_max;
    float survival_min;
    float survival_max;
    float greed_min;
    float greed_max;
    float aggression_min;
    float aggression_max;
    float discipline_min;
    float discipline_max;

    void PredAIPersonalitySettings()
    {
        bravery_min = 0.20;
        bravery_max = 0.95;
        survival_min = 0.25;
        survival_max = 0.98;
        greed_min = 0.10;
        greed_max = 0.90;
        aggression_min = 0.20;
        aggression_max = 0.95;
        discipline_min = 0.25;
        discipline_max = 0.95;
    }
}
