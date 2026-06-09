# Resources/Client — auto-download mods go here

Drop your vehicle/map mod **`.zip` files in this folder, still zipped — do NOT
extract them.** When a player joins, the BeamMP launcher automatically
downloads every `.zip` in here that they don't already have. Nobody installs
anything by hand.

## Rules of thumb

- **Keep them zipped.** One `.zip` per mod, as downloaded.
- **Add a few at a time.** After adding mods, launch the server and join once to
  confirm a clean start before adding the next batch. This is how you find a
  bad mod fast (see the troubleshooting section in the top-level `README.md`).
- **Size:** ~100–200 MB per car is the comfortable range. Remember every joiner
  downloads the *total* of everything in here on their first connect, over your
  home upload speed — four 150 MB cars ≈ ~600 MB per new player.
- **Custom map?** The map's `.zip` goes here too, *and* its path must be set as
  `Map = "..."` in `ServerConfig.toml`.

## Your BMW mods (planned)

Copy these into this folder on your laptop:

- `BMW_1M_modland.zip`
- `BMW3_F30-M3-Touring_by_hxmxnn_modland.zip`
- `BMW_E90_M3.etiketbros.zip`
- `mate30_als_modland.zip`  ← ⚠️ see the note in the top-level `README.md`;
  this filename isn't an obvious BMW — verify it's a vehicle mod you actually
  want and that it's BeamMP-compatible before relying on it.

> These `.zip` files are intentionally **git-ignored** (see `.gitignore`) — they
> live only on your laptop, not in the repo.
