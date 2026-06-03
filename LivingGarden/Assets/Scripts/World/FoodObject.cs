using UnityEngine;

namespace LivingGarden
{
    /// <summary>
    /// A piece of food sitting in the world. Spawned entirely from code by the Bootstrap / player tools.
    /// </summary>
    public class FoodObject : MonoBehaviour
    {
        public FoodType type;

        public static FoodObject Spawn(FoodType type, Vector3 pos)
        {
            var go = new GameObject("Food_" + type.name);
            go.transform.position = pos;

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = VisualFactory.Circle();
            sr.color = type.color;
            go.transform.localScale = Vector3.one * 0.45f;
            sr.sortingOrder = 5;

            var food = go.AddComponent<FoodObject>();
            food.type = type;
            return food;
        }
    }
}
