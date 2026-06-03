# Living Garden — Unity 6 (Bootstrap build)

A top-down 2D artificial-life sim — a *spiritual successor* to the old "Norns"
artificial-life games (its own creatures, names and world, no shared IP). You play
a god/parent/scientist who influences creatures rather than controlling them.

This drop is **Version 0.1**: prove the creature is alive.

> One creature can: get hungry → see food → walk to it → eat → remember if the
> food was good or bad → learn a word for it → react differently next time →
> save/load its brain and memories.

---

## The bootstrap workflow (this is the whole point)

You do **not** build the scene by hand. You attach one script to one empty object
and everything spawns from code at Play time.

1. Open this folder as a project in **Unity 6** (Unity Hub → Add → select this folder).
   - Editor version is pinned in `ProjectSettings/ProjectVersion.txt`; let Unity
     upgrade it to whatever Unity 6 you have installed.
2. Open the default `SampleScene` (or any empty scene).
3. **GameObject → Create Empty.**
4. With it selected: **Add Component → Bootstrap.**
5. Press **Play.**

The `Bootstrap` script builds the entire scene on Play: orthographic camera, ground,
walls, starting food, one living creature, and the on-screen HUD. Nothing else to wire.

> Shortcut: menu **Living Garden → Create Bootstrap Object** does steps 3–4 for you.

---

## Playing v0.1

A control panel is drawn on the left (IMGUI — no Canvas needed):

- **World Tool** — pick a food (Apple/Berry are safe, Bluecap is toxic), then
  **click in the world** to drop it.
- **Hand Tool** — Pet (raises mood) / Scold (lowers mood, interrupts the creature).
- **Teaching Tool** — type a word, click *Teach Word* to associate it with the
  selected food. Once learned, the creature "says" the word when it sees that food.
- **Save / Load** — writes the creature's brain + memory to JSON in
  `Application.persistentDataPath`.
- **Brain / Memory** — live inspector: learned food valences (likes/avoids),
  taught words, and recent episodic memories.

### Watch the learning happen
1. Drop a **Bluecap** mushroom. When hungry, the creature may try it, get sick
   (turns greenish, mood drops) and its valence for `bluecap` goes negative.
2. Next time it's hungry it **avoids** the bluecap and prefers apples — unless it's
   starving, in which case desperation overrides caution.
3. Teach it the word "yum" for apple; it'll announce "yum" when it spots one.

---

## Project layout

```
Assets/Scripts/
├── Bootstrap.cs              # builds the whole scene on Play
├── Core/
│   ├── VisualFactory.cs      # runtime-generated sprites (no art assets needed)
│   └── SaveSystem.cs         # JSON save/load of brain + memory
├── Creatures/
│   ├── Creature.cs           # the Sense→Decide→Act→Learn→Remember loop
│   ├── CreatureNeeds.cs      # drives + body chemistry (hunger/glucose/toxins/health)
│   └── CreatureMemory.cs     # object valences, word meanings, episodes (Hebbian reinforce)
├── World/
│   ├── FoodType.cs           # nutrients/toxins per food
│   └── FoodObject.cs         # food in the world
├── UI/
│   └── GameHUD.cs            # player tools + science inspector
└── Editor/
    └── BootstrapMenu.cs      # one-click bootstrap object creation
```

## What's intentionally NOT here yet (the roadmap)

Breeding, genetics, disease, toys, full language, machines, social behavior,
dreaming, families, and large neural graphs all stack on top of this foundation —
exactly the order in the design doc. v0.1 only has to prove the creature feels alive.
The memory `valence` value is literally a single learned synapse weight; scaling that
into the full `NeuralGraph` is the next step.
```
