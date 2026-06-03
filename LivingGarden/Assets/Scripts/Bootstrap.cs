using UnityEngine;

namespace LivingGarden
{
    /// <summary>
    /// THE BOOTSTRAP.
    ///
    /// Workflow:
    ///   1. Open the project in Unity 6.
    ///   2. In any empty scene, create an empty GameObject (GameObject > Create Empty).
    ///   3. Add this Bootstrap component to it (Add Component > Bootstrap).
    ///   4. Press Play.
    ///
    /// On Play this single script constructs the ENTIRE playable scene from code:
    /// camera, lit background, world bounds, food, one living creature, and the HUD.
    /// Nothing else needs to be wired in the editor.
    /// </summary>
    [DefaultExecutionOrder(-100)]
    public class Bootstrap : MonoBehaviour
    {
        [Header("World")]
        public Vector2 worldSize = new Vector2(16, 9);
        public Color groundColor = new Color(0.18f, 0.22f, 0.16f);

        [Header("Creature")]
        public string creatureName = "Pip";

        [Header("Starting food")]
        public int startingApples = 2;
        public int startingMushrooms = 1;

        private Bounds _world;

        private void Start()
        {
            _world = new Bounds(Vector3.zero, new Vector3(worldSize.x, worldSize.y, 0f));

            BuildCamera();
            BuildGround();
            BuildBorders();
            BuildStartingFood();
            Creature creature = BuildCreature();
            BuildHUD(creature);

            Debug.Log("[LivingGarden] Bootstrap complete. The garden is alive.");
        }

        private void BuildCamera()
        {
            Camera cam = Camera.main;
            if (cam == null)
            {
                var go = new GameObject("Main Camera");
                go.tag = "MainCamera";
                cam = go.AddComponent<Camera>();
            }
            cam.orthographic = true;
            cam.orthographicSize = worldSize.y * 0.5f + 0.5f;
            cam.transform.position = new Vector3(0, 0, -10);
            cam.backgroundColor = new Color(0.08f, 0.09f, 0.10f);
            cam.clearFlags = CameraClearFlags.SolidColor;
        }

        private void BuildGround()
        {
            var go = new GameObject("Ground");
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = VisualFactory.Square();
            sr.color = groundColor;
            sr.sortingOrder = -10;
            go.transform.localScale = new Vector3(worldSize.x, worldSize.y, 1f);
        }

        private void BuildBorders()
        {
            // Thin walls so the world reads as an enclosed garden.
            CreateWall(new Vector3(0, worldSize.y / 2f, 0), new Vector3(worldSize.x, 0.2f, 1));
            CreateWall(new Vector3(0, -worldSize.y / 2f, 0), new Vector3(worldSize.x, 0.2f, 1));
            CreateWall(new Vector3(worldSize.x / 2f, 0, 0), new Vector3(0.2f, worldSize.y, 1));
            CreateWall(new Vector3(-worldSize.x / 2f, 0, 0), new Vector3(0.2f, worldSize.y, 1));
        }

        private void CreateWall(Vector3 pos, Vector3 scale)
        {
            var go = new GameObject("Wall");
            go.transform.position = pos;
            go.transform.localScale = scale;
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = VisualFactory.Square();
            sr.color = new Color(0.30f, 0.34f, 0.28f);
            sr.sortingOrder = -5;
        }

        private void BuildStartingFood()
        {
            for (int i = 0; i < startingApples; i++)
                FoodObject.Spawn(FoodType.Apple, RandomPoint());
            for (int i = 0; i < startingMushrooms; i++)
                FoodObject.Spawn(FoodType.BlueMushroom, RandomPoint());
        }

        private Creature BuildCreature()
        {
            return Creature.Spawn(creatureName, Vector3.zero, _world);
        }

        private void BuildHUD(Creature creature)
        {
            var go = new GameObject("GameHUD");
            var hud = go.AddComponent<GameHUD>();
            hud.creature = creature;
            hud.world = _world;
        }

        private Vector3 RandomPoint()
        {
            return new Vector3(
                Random.Range(_world.min.x + 1.5f, _world.max.x - 1.5f),
                Random.Range(_world.min.y + 1.5f, _world.max.y - 1.5f),
                0f);
        }
    }
}
