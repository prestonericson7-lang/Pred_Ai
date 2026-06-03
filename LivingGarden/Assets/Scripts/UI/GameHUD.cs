using UnityEngine;

namespace LivingGarden
{
    /// <summary>
    /// The player's toolset + science inspector, drawn with IMGUI so the bootstrap needs
    /// no Canvas/prefab wiring. Place food, teach words, pet/scold, inspect the brain, save/load.
    /// </summary>
    public class GameHUD : MonoBehaviour
    {
        public Creature creature;
        public Bounds world;

        private int _foodChoice = 0;
        private readonly FoodType[] _foods = { FoodType.Apple, FoodType.Berry, FoodType.BlueMushroom };
        private readonly string[] _foodLabels = { "Apple (safe)", "Berry (safe)", "Bluecap (TOXIC)" };
        private string _wordToTeach = "";
        private string _status = "Click in the world to drop the selected food.";
        private Vector2 _memScroll;

        private void Update()
        {
            // Drop food where the player clicks (ignoring clicks over the HUD panel).
            if (Input.GetMouseButtonDown(0) && Input.mousePosition.x > 320)
            {
                Camera cam = Camera.main;
                Vector3 wp = cam.ScreenToWorldPoint(Input.mousePosition);
                wp.z = 0f;
                if (world.Contains(new Vector3(wp.x, wp.y, world.center.z)))
                {
                    FoodObject.Spawn(_foods[_foodChoice], wp);
                    _status = $"Placed {_foods[_foodChoice].name}.";
                }
            }
        }

        private void OnGUI()
        {
            GUILayout.BeginArea(new Rect(10, 10, 300, Screen.height - 20), GUI.skin.box);

            GUILayout.Label("<b>LIVING GARDEN — v0.1</b>", Rich(14));
            GUILayout.Space(4);

            if (creature == null) { GUILayout.Label("No creature."); GUILayout.EndArea(); return; }

            // --- Body / needs ---
            GUILayout.Label("<b>Body</b>", Rich(12));
            Bar("Hunger", creature.needs.hunger, true);
            Bar("Glucose", creature.needs.glucose, false);
            Bar("Toxins", creature.needs.toxins, true);
            Bar("Health", creature.needs.health, false);
            Bar("Mood", creature.needs.mood, false);
            GUILayout.Label($"Goal: <i>{creature.CurrentGoal}</i>", Rich(11));
            if (creature.needs.IsSick) GUILayout.Label("<color=#7fff7f>Feeling sick...</color>", Rich(11));

            GUILayout.Space(6);

            // --- World tool: place food ---
            GUILayout.Label("<b>World Tool</b>", Rich(12));
            _foodChoice = GUILayout.SelectionGrid(_foodChoice, _foodLabels, 1);

            GUILayout.Space(6);

            // --- Hand tool ---
            GUILayout.Label("<b>Hand Tool</b>", Rich(12));
            GUILayout.BeginHorizontal();
            if (GUILayout.Button("Pet ♥")) creature.Pet();
            if (GUILayout.Button("Scold")) creature.Scold();
            GUILayout.EndHorizontal();

            GUILayout.Space(6);

            // --- Speech / teaching tool ---
            GUILayout.Label("<b>Teaching Tool</b>", Rich(12));
            GUILayout.Label("Word for selected food:", Rich(10));
            _wordToTeach = GUILayout.TextField(_wordToTeach);
            if (GUILayout.Button("Teach Word"))
            {
                creature.memory.TeachWord(_wordToTeach, _foods[_foodChoice].name);
                _status = $"Taught \"{_wordToTeach}\" = {_foods[_foodChoice].name}.";
                _wordToTeach = "";
            }

            GUILayout.Space(6);

            // --- Save / load ---
            GUILayout.BeginHorizontal();
            if (GUILayout.Button("Save")) { SaveSystem.Save(creature); _status = "Saved brain + memory."; }
            GUI.enabled = SaveSystem.HasSave;
            if (GUILayout.Button("Load")) { SaveSystem.Load(creature); _status = "Loaded brain + memory."; }
            GUI.enabled = true;
            GUILayout.EndHorizontal();

            GUILayout.Space(6);

            // --- Science inspector: what the creature has learned ---
            GUILayout.Label("<b>Brain / Memory</b>", Rich(12));
            _memScroll = GUILayout.BeginScrollView(_memScroll, GUILayout.Height(180));
            foreach (var o in creature.memory.objects)
            {
                string tag = o.valence > 0.1f ? "<color=#9fff9f>likes</color>"
                           : o.valence < -0.1f ? "<color=#ff9f9f>avoids</color>" : "unsure";
                GUILayout.Label($"{o.foodName}: {tag} ({o.valence:0.00}, x{o.timesEaten})", Rich(10));
            }
            foreach (var w in creature.memory.words)
                GUILayout.Label($"word \"{w.word}\" = {w.foodName}", Rich(10));
            GUILayout.Space(4);
            GUILayout.Label("<b>Recent episodes</b>", Rich(11));
            for (int i = creature.memory.episodes.Count - 1; i >= 0 && i > creature.memory.episodes.Count - 8; i--)
                GUILayout.Label("• " + creature.memory.episodes[i], Rich(9));
            GUILayout.EndScrollView();

            GUILayout.FlexibleSpace();
            GUILayout.Label("<i>" + _status + "</i>", Rich(10));
            GUILayout.EndArea();
        }

        private static GUIStyle Rich(int size)
        {
            return new GUIStyle(GUI.skin.label) { richText = true, fontSize = size, wordWrap = true };
        }

        private void Bar(string label, float v, bool redIsHigh)
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label(label, GUILayout.Width(60));
            Rect r = GUILayoutUtility.GetRect(150, 14);
            GUI.Box(r, GUIContent.none);
            Color c = redIsHigh ? Color.Lerp(Color.green, Color.red, v) : Color.Lerp(Color.red, Color.green, v);
            var fill = new Rect(r.x + 1, r.y + 1, (r.width - 2) * Mathf.Clamp01(v), r.height - 2);
            var prev = GUI.color; GUI.color = c;
            GUI.DrawTexture(fill, Texture2D.whiteTexture);
            GUI.color = prev;
            GUILayout.EndHorizontal();
        }
    }
}
