using UnityEngine;

namespace LivingGarden
{
    /// <summary>
    /// Drives + a small body-chemistry simulation. Hunger is driven by blood glucose,
    /// toxins make the creature sick and damage health. This is the v0.1 slice of the
    /// full Needs + Chemistry systems in the design doc.
    /// </summary>
    [System.Serializable]
    public class CreatureNeeds
    {
        [Range(0, 1)] public float hunger = 0.4f;   // 0 = full, 1 = starving
        [Range(0, 1)] public float energy = 1f;     // tiredness inverse
        [Range(0, 1)] public float glucose = 0.6f;  // blood sugar
        [Range(0, 1)] public float hydration = 0.7f;
        [Range(0, 1)] public float toxins = 0f;      // 0 = clean
        [Range(0, 1)] public float health = 1f;
        [Range(0, 1)] public float mood = 0.6f;      // affected by petting / scolding

        public float hungerRate = 0.020f;   // per second
        public float toxinDecay = 0.05f;
        public float glucoseBurn = 0.015f;

        public bool IsHungry => hunger > 0.55f;
        public bool IsStarving => hunger > 0.9f;
        public bool IsSick => toxins > 0.35f;

        public void Tick(float dt)
        {
            glucose = Mathf.Clamp01(glucose - glucoseBurn * dt);
            // Low blood sugar drives hunger up; high sugar eases it.
            float drive = Mathf.Lerp(hungerRate * 2f, -hungerRate, glucose);
            hunger = Mathf.Clamp01(hunger + drive * dt);

            hydration = Mathf.Clamp01(hydration - 0.01f * dt);
            toxins = Mathf.Clamp01(toxins - toxinDecay * dt);

            if (IsSick)
                health = Mathf.Clamp01(health - 0.05f * dt * (toxins));
            else
                health = Mathf.Clamp01(health + 0.01f * dt);

            // Mood drifts toward a baseline set by wellbeing.
            mood = Mathf.MoveTowards(mood, Wellbeing, 0.1f * dt);
        }

        /// <summary>A single scalar the brain uses to score how good life currently is.</summary>
        public float Wellbeing => Mathf.Clamp01(0.5f + 0.3f * (1f - hunger) + 0.2f * (1f - toxins) - 0.2f * (1f - health));

        public void Eat(FoodType food)
        {
            glucose = Mathf.Clamp01(glucose + food.glucose);
            hunger = Mathf.Clamp01(hunger - food.glucose * 1.2f);
            hydration = Mathf.Clamp01(hydration + food.water);
            toxins = Mathf.Clamp01(toxins + food.toxin);
        }
    }
}
