using System.Collections.Generic;
using UnityEngine;

namespace LivingGarden
{
    /// <summary>
    /// One living creature. Runs the core behavior loop every frame:
    ///   Sense -> Update chemistry/needs -> Recall memory -> Decide -> Act -> Measure -> Learn -> Remember.
    /// This is the whole game in miniature (Version 0.1).
    /// </summary>
    public class Creature : MonoBehaviour
    {
        public string creatureName = "Norn";
        public CreatureNeeds needs = new CreatureNeeds();
        public CreatureMemory memory = new CreatureMemory();

        public float moveSpeed = 1.6f;
        public float sightRadius = 8f;
        public float eatRadius = 0.5f;
        public float learnRate = 0.5f;

        public string CurrentGoal { get; private set; } = "idle";

        private SpriteRenderer _sr;
        private FoodObject _target;
        private Vector3 _wanderPoint;
        private float _wanderTimer;
        private float _speakCooldown;
        private string _floatingWord;
        private float _floatingTimer;
        private Bounds _world;

        public void Init(string name, Bounds worldBounds)
        {
            creatureName = name;
            _world = worldBounds;
            _wanderPoint = transform.position;
        }

        private void Awake()
        {
            _sr = GetComponent<SpriteRenderer>();
            if (_world.size == Vector3.zero) _world = new Bounds(Vector3.zero, new Vector3(16, 9, 0));
        }

        private void Update()
        {
            float dt = Time.deltaTime;

            // --- Update body chemistry + needs ---
            needs.Tick(dt);

            // --- Tint reflects internal state (sick = green, happy = bright, sad = dim) ---
            Color baseCol = new Color(0.95f, 0.8f, 0.55f);
            if (needs.IsSick) baseCol = Color.Lerp(baseCol, new Color(0.4f, 0.8f, 0.3f), needs.toxins);
            _sr.color = Color.Lerp(baseCol * 0.6f, baseCol, needs.mood);

            // --- Decide + act ---
            Think(dt);

            // --- Floating speech timer ---
            if (_floatingTimer > 0f) _floatingTimer -= dt;
            if (_speakCooldown > 0f) _speakCooldown -= dt;
        }

        // The decision system: pick a goal, pick a target, pick an action.
        private void Think(float dt)
        {
            if (needs.IsHungry)
            {
                if (_target == null) _target = ChooseFood();

                if (_target != null)
                {
                    CurrentGoal = "seek " + _target.type.name;
                    MaybeSpeak(_target.type.name); // name what it sees if it knows the word
                    MoveToward(_target.transform.position, dt);

                    if (Vector3.Distance(transform.position, _target.transform.position) <= eatRadius)
                        EatTarget();
                    return;
                }

                CurrentGoal = "hungry, searching";
            }
            else
            {
                CurrentGoal = "content";
            }

            Wander(dt);
        }

        // Perception + recall: look at nearby food and score it by learned valence.
        private FoodObject ChooseFood()
        {
            FoodObject best = null;
            float bestScore = float.NegativeInfinity;

            foreach (var f in FindObjectsByType<FoodObject>(FindObjectsSortMode.None))
            {
                float dist = Vector3.Distance(transform.position, f.transform.position);
                if (dist > sightRadius) continue;

                float valence = memory.Valence(f.type.name);

                // Avoid known-bad food unless desperate.
                if (valence < -0.3f && !needs.IsStarving) continue;

                // Curiosity nudges the creature to try unknown food.
                float curiosity = memory.Knows(f.type.name) ? 0f : 0.25f;

                float score = valence + curiosity - dist * 0.04f;
                if (score > bestScore)
                {
                    bestScore = score;
                    best = f;
                }
            }
            return best;
        }

        private void EatTarget()
        {
            if (_target == null) return;
            FoodType food = _target.type;

            float before = needs.Wellbeing;
            needs.Eat(food);
            float after = needs.Wellbeing;

            // Reward = change in wellbeing, with toxins counted as a strong punisher.
            float reward = Mathf.Clamp((after - before) * 3f - food.toxin * 1.5f, -1f, 1f);
            memory.Reinforce(food.name, reward, learnRate);

            string outcome = reward >= 0f ? "felt good" : "got sick";
            memory.Remember($"Ate {food.name} -> {outcome} (reward {reward:0.00})");

            if (reward < 0f)
            {
                needs.mood = Mathf.Clamp01(needs.mood - 0.3f);
                Say(food.name, true); // recoil
            }

            Destroy(_target.gameObject);
            _target = null;
        }

        private void MoveToward(Vector3 pos, float dt)
        {
            transform.position = Vector3.MoveTowards(transform.position, pos, moveSpeed * dt);
        }

        private void Wander(float dt)
        {
            _wanderTimer -= dt;
            if (_wanderTimer <= 0f || Vector3.Distance(transform.position, _wanderPoint) < 0.2f)
            {
                _wanderTimer = Random.Range(1.5f, 3.5f);
                _wanderPoint = new Vector3(
                    Random.Range(_world.min.x + 1, _world.max.x - 1),
                    Random.Range(_world.min.y + 1, _world.max.y - 1),
                    0f);
            }
            MoveToward(_wanderPoint, dt * 0.6f);
        }

        private void MaybeSpeak(string foodName)
        {
            if (_speakCooldown > 0f) return;
            string word = memory.WordFor(foodName);
            if (!string.IsNullOrEmpty(word))
            {
                Say(word, false);
                _speakCooldown = 2.5f;
            }
        }

        public void Say(string word, bool distress)
        {
            _floatingWord = distress ? $"!{word}!" : word;
            _floatingTimer = 1.6f;
        }

        // --- Player interactions ---
        public void Pet()
        {
            needs.mood = Mathf.Clamp01(needs.mood + 0.25f);
            memory.Remember("Player petted me (felt safe)");
            Say("♥", false);
        }

        public void Scold()
        {
            needs.mood = Mathf.Clamp01(needs.mood - 0.25f);
            memory.Remember("Player scolded me");
            Say("...", true);
            _target = null; // interrupt current action
        }

        private void OnGUI()
        {
            // Name + floating speech rendered above the creature in screen space.
            Camera cam = Camera.main;
            if (cam == null) return;
            Vector3 sp = cam.WorldToScreenPoint(transform.position + Vector3.up * 0.6f);
            if (sp.z < 0) return;
            float y = Screen.height - sp.y;

            var style = new GUIStyle(GUI.skin.label) { alignment = TextAnchor.MiddleCenter, fontSize = 12 };
            style.normal.textColor = Color.white;
            GUI.Label(new Rect(sp.x - 60, y - 24, 120, 18), creatureName, style);

            if (_floatingTimer > 0f && !string.IsNullOrEmpty(_floatingWord))
            {
                var s2 = new GUIStyle(GUI.skin.label) { alignment = TextAnchor.MiddleCenter, fontSize = 16, fontStyle = FontStyle.Bold };
                s2.normal.textColor = Color.yellow;
                GUI.Label(new Rect(sp.x - 60, y - 44, 120, 20), _floatingWord, s2);
            }
        }

        public static Creature Spawn(string name, Vector3 pos, Bounds worldBounds)
        {
            var go = new GameObject("Creature_" + name);
            go.transform.position = pos;
            go.transform.localScale = Vector3.one * 0.8f;

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = VisualFactory.Circle();
            sr.color = new Color(0.95f, 0.8f, 0.55f);
            sr.sortingOrder = 10;

            var c = go.AddComponent<Creature>();
            c.Init(name, worldBounds);
            return c;
        }
    }
}
