# Living Garden

A 2D side-view **artificial-life simulation** built in Unreal Engine 5.4+. The world
is entirely procedurally bootstrapped at runtime and inhabited by **Garden Species** —
autonomous creatures driven by advanced neural/behavioral AI.

This is an **original implementation** of the side-scrolling life-sim genre. It uses no
third-party game assets, code, or trademarked names; all content is generated or authored
within this project.

---

## Step 1 — Project Setup (this commit)

### Required plugins (enabled in `LivingGarden.uproject`)
| Plugin | Purpose |
| --- | --- |
| **Paper2D** | 2D sprites / side-view world rendering |
| **Niagara** | Particle & atmosphere FX |
| **PCG** | Procedural Content Generation for the world bootstrap |
| **EnhancedInput** | Observer camera + point-and-click interaction |
| **NNE** + **NNERuntimeORT** | Neural Network Engine for creature cognition inference |
| **LearningAgents** | Reinforcement / imitation learning for Garden Species |
| **GameplayBehaviors**, **StateTree** | High-level behavior arbitration on top of Behavior Trees |

Engine modules pulled in via `LivingGarden.Build.cs`: `AIModule`, `GameplayTasks`,
`NavigationSystem`, `StateTreeModule`, `GameplayStateTreeModule`.

### Folder structure
```
LivingGarden/
├── LivingGarden.uproject
├── Config/                         # Engine/Game/Input .ini (GameMode registered here)
├── Content/                        # All .uasset content (created in-editor)
│   ├── Maps/                       #   L_Garden — the single empty bootstrap level
│   ├── Blueprints/                 #   BP_ subclasses of the C++ core
│   ├── Data/                       #   Data Assets that drive procedural generation
│   ├── Sprites/ Flipbooks/ FX/     #   Paper2D + Niagara assets
│   └── UI/
└── Source/LivingGarden/
    ├── Core/                       # GameMode, PlayerController, Observer pawn
    ├── Bootstrap/                  # Runtime procedural world generation pipeline
    ├── World/                      # Terrain, flora, items, biome actors (later steps)
    ├── Creatures/                  # Garden Species pawns + AI/cognition (later steps)
    └── Data/                       # Data Asset classes (later steps)
```

### Bootstrap system architecture
Nothing in the world is hand-placed. When the player presses **Start**, the GameMode
calls `UWorldBootstrapSubsystem::BeginBootstrap(Config)`. The subsystem runs an ordered,
deterministic, optionally time-sliced pipeline (`EGardenBootstrapPhase`):

```
Idle → Seeding → Terrain → Atmosphere → Flora → Items → Fauna
     → Species → Cognition → Finalize → Complete
```

- A single seeded `FRandomStream` feeds every generator, so a given **WorldSeed**
  reproduces an identical world 1:1.
- Each phase reports progress via `OnPhaseChanged` (drives the loading screen) and the
  pipeline ends with `OnBootstrapComplete`.
- Time-slicing spreads heavy generation across frames so the loading screen stays live.

Configuration is supplied by `FGardenBootstrapConfig` (world size, seed, species count,
object density). In later steps this is sourced from `UGardenWorldDataAsset`.

### Core classes (this commit)
| Class | Role |
| --- | --- |
| `ALivingGardenGameMode` | Session lifecycle; triggers + listens to the bootstrap |
| `UWorldBootstrapSubsystem` | Phased, seeded, runtime world generator |
| `AGardenObserverPawn` | Orthographic side-view camera that pans the world |
| `ALivingGardenPlayerController` | Enhanced Input → camera & interaction |

### Editor steps to finish Step 1
1. Generate project files (right-click `.uproject` → *Generate Visual Studio project files*)
   and build the **LivingGardenEditor** target.
2. Create an empty level `Content/Maps/L_Garden` (delete the floor/objects; keep it bare).
   The bootstrap fills it at runtime.
3. Confirm **World Settings → GameMode Override** is empty (the project default is
   `LivingGardenGameMode`, set in `Config/DefaultEngine.ini`).
4. Press **Play**: with `bAutoStart = true` the Output Log shows the bootstrap stepping
   `Seeding → … → Complete` and prints the resolved seed.
