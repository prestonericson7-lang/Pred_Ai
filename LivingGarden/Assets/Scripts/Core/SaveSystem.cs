using System.IO;
using UnityEngine;

namespace LivingGarden
{
    [System.Serializable]
    public class CreatureSaveData
    {
        public string creatureName;
        public float posX, posY;
        public CreatureNeeds needs;
        public CreatureMemory memory;
    }

    [System.Serializable]
    public class SaveData
    {
        public CreatureSaveData creature;
        public string savedAt;
    }

    /// <summary>
    /// JSON save/load of the creature's brain, memory and body to persistentDataPath.
    /// Proves requirement #8: "Save/load its brain and memories."
    /// </summary>
    public static class SaveSystem
    {
        private static string Path => System.IO.Path.Combine(Application.persistentDataPath, "living_garden_save.json");

        public static void Save(Creature c)
        {
            if (c == null) return;
            var data = new SaveData
            {
                savedAt = System.DateTime.Now.ToString("HH:mm:ss"),
                creature = new CreatureSaveData
                {
                    creatureName = c.creatureName,
                    posX = c.transform.position.x,
                    posY = c.transform.position.y,
                    needs = c.needs,
                    memory = c.memory
                }
            };
            File.WriteAllText(Path, JsonUtility.ToJson(data, true));
            Debug.Log("[LivingGarden] Saved to " + Path);
        }

        public static bool Load(Creature c)
        {
            if (c == null || !File.Exists(Path)) return false;
            var data = JsonUtility.FromJson<SaveData>(File.ReadAllText(Path));
            if (data?.creature == null) return false;

            c.creatureName = data.creature.creatureName;
            c.needs = data.creature.needs;
            c.memory = data.creature.memory;
            c.transform.position = new Vector3(data.creature.posX, data.creature.posY, 0f);
            Debug.Log("[LivingGarden] Loaded save from " + data.savedAt);
            return true;
        }

        public static bool HasSave => File.Exists(Path);
    }
}
