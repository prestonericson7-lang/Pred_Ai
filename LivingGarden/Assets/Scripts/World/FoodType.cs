using UnityEngine;

namespace LivingGarden
{
    /// <summary>
    /// Food is "real inside the body": each type carries nutrients and toxins, not just a hunger number.
    /// This is the seed of the diet system described in the design doc.
    /// </summary>
    [System.Serializable]
    public struct FoodType
    {
        public string name;
        public Color color;
        public float glucose;   // raises blood sugar / lowers hunger
        public float water;     // hydration
        public float toxin;     // 0..1 chance/strength of making the creature sick
        public string smellTag; // sweet / earthy / etc.

        public static FoodType Apple => new FoodType
        {
            name = "apple", color = new Color(0.85f, 0.2f, 0.2f),
            glucose = 0.45f, water = 0.30f, toxin = 0f, smellTag = "sweet"
        };

        public static FoodType BlueMushroom => new FoodType
        {
            name = "bluecap", color = new Color(0.30f, 0.45f, 0.95f),
            glucose = 0.15f, water = 0.05f, toxin = 0.8f, smellTag = "earthy"
        };

        public static FoodType Berry => new FoodType
        {
            name = "berry", color = new Color(0.55f, 0.15f, 0.6f),
            glucose = 0.30f, water = 0.20f, toxin = 0f, smellTag = "tart"
        };
    }
}
