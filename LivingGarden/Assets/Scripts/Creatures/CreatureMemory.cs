using System.Collections.Generic;
using UnityEngine;

namespace LivingGarden
{
    /// <summary>
    /// Per-food-type association strength. This IS the creature's tiny neural synapse:
    /// a weight learned by reward/punishment that says "this thing was good or bad for me".
    /// </summary>
    [System.Serializable]
    public class ObjectMemory
    {
        public string foodName;
        public float valence;   // -1 (avoid) .. +1 (seek), the learned synapse weight
        public int timesEaten;
    }

    /// <summary>A word the player taught, mapped to the food type it names.</summary>
    [System.Serializable]
    public class WordMeaning
    {
        public string word;
        public string foodName;
        public float strength; // grows with repeated teaching
    }

    /// <summary>
    /// The saveable memory bank. Dictionaries aren't serializable by JsonUtility,
    /// so we store lists and index them at runtime.
    /// </summary>
    [System.Serializable]
    public class CreatureMemory
    {
        public List<ObjectMemory> objects = new List<ObjectMemory>();
        public List<WordMeaning> words = new List<WordMeaning>();
        public List<string> episodes = new List<string>();

        public ObjectMemory GetOrCreate(string foodName)
        {
            foreach (var o in objects)
                if (o.foodName == foodName) return o;
            var m = new ObjectMemory { foodName = foodName, valence = 0f, timesEaten = 0 };
            objects.Add(m);
            return m;
        }

        public float Valence(string foodName)
        {
            foreach (var o in objects)
                if (o.foodName == foodName) return o.valence;
            return 0f; // unknown
        }

        public bool Knows(string foodName)
        {
            foreach (var o in objects)
                if (o.foodName == foodName && o.timesEaten > 0) return true;
            return false;
        }

        /// <summary>Hebbian-style reward: nudge the synapse weight toward the experienced outcome.</summary>
        public void Reinforce(string foodName, float reward, float learnRate)
        {
            var m = GetOrCreate(foodName);
            m.valence = Mathf.Clamp(m.valence + (reward - m.valence) * learnRate, -1f, 1f);
            m.timesEaten++;
        }

        public void TeachWord(string word, string foodName)
        {
            word = word.Trim().ToLowerInvariant();
            if (string.IsNullOrEmpty(word)) return;
            foreach (var w in words)
            {
                if (w.word == word && w.foodName == foodName)
                {
                    w.strength = Mathf.Clamp01(w.strength + 0.34f);
                    return;
                }
            }
            words.Add(new WordMeaning { word = word, foodName = foodName, strength = 0.34f });
        }

        public string WordFor(string foodName)
        {
            WordMeaning best = null;
            foreach (var w in words)
                if (w.foodName == foodName && (best == null || w.strength > best.strength))
                    best = w;
            return (best != null && best.strength > 0.5f) ? best.word : null;
        }

        public void Remember(string episode)
        {
            episodes.Add(episode);
            if (episodes.Count > 40) episodes.RemoveAt(0);
        }
    }
}
