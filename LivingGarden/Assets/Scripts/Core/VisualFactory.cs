using UnityEngine;

namespace LivingGarden
{
    /// <summary>
    /// Generates simple sprites at runtime so the project needs zero imported art assets.
    /// Everything you see in Living Garden v0.1 is drawn from code.
    /// </summary>
    public static class VisualFactory
    {
        private static Sprite _circle;
        private static Sprite _square;

        public static Sprite Circle()
        {
            if (_circle == null) _circle = BuildCircle(64);
            return _circle;
        }

        public static Sprite Square()
        {
            if (_square == null) _square = BuildSquare(64);
            return _square;
        }

        private static Sprite BuildCircle(int size)
        {
            var tex = new Texture2D(size, size, TextureFormat.RGBA32, false);
            tex.filterMode = FilterMode.Bilinear;
            float r = size * 0.5f;
            Vector2 c = new Vector2(r, r);
            for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                float d = Vector2.Distance(new Vector2(x + 0.5f, y + 0.5f), c);
                float a = Mathf.Clamp01(r - d);
                tex.SetPixel(x, y, new Color(1, 1, 1, a));
            }
            tex.Apply();
            return Sprite.Create(tex, new Rect(0, 0, size, size), new Vector2(0.5f, 0.5f), size);
        }

        private static Sprite BuildSquare(int size)
        {
            var tex = new Texture2D(size, size, TextureFormat.RGBA32, false);
            tex.filterMode = FilterMode.Bilinear;
            var px = new Color[size * size];
            for (int i = 0; i < px.Length; i++) px[i] = Color.white;
            tex.SetPixels(px);
            tex.Apply();
            return Sprite.Create(tex, new Rect(0, 0, size, size), new Vector2(0.5f, 0.5f), size);
        }
    }
}
