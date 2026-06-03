#if UNITY_EDITOR
using UnityEditor;
using UnityEngine;

namespace LivingGarden
{
    /// <summary>
    /// Optional convenience. Does the exact manual step for you:
    /// "create an empty GameObject and assign the Bootstrap to it."
    /// Menu: Living Garden > Create Bootstrap Object
    /// </summary>
    public static class BootstrapMenu
    {
        [MenuItem("Living Garden/Create Bootstrap Object")]
        public static void Create()
        {
            var existing = Object.FindFirstObjectByType<Bootstrap>();
            if (existing != null)
            {
                Selection.activeGameObject = existing.gameObject;
                Debug.Log("[LivingGarden] A Bootstrap already exists in the scene.");
                return;
            }

            var go = new GameObject("Bootstrap");
            go.AddComponent<Bootstrap>();
            Selection.activeGameObject = go;
            Undo.RegisterCreatedObjectUndo(go, "Create Bootstrap");
            Debug.Log("[LivingGarden] Bootstrap object created. Press Play to grow the garden.");
        }
    }
}
#endif
