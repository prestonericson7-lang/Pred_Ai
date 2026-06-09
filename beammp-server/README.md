# Pred Ai BMW — BeamMP Private Server Kit

Everything you need to stand up a private **BeamMP** (BeamNG.drive multiplayer)
server on your laptop so you and your friend in Michigan can drive BMWs
together, with the mods auto-downloading to anyone who joins.

> **Read this first — why this is a "kit" and not a finished server.**
> This was assembled by Claude running in an isolated **Linux cloud
> container**, not on your Windows laptop. I can't reach your laptop, your
> `C:\Users\pedis\Downloads\` mod files, your router, or run
> `BeamMP-Server.exe` for you. The files you attached never reached my
> environment. So instead of pretending to "drop your zips in and launch," I
> built you a **correct, ready-to-run kit**: the validated config, the exact
> folder layout, a Windows launcher, a `.gitignore` that protects your
> AuthKey, a Lua plugin scaffold, and the step-by-step below. You run it on
> your laptop — the parts that physically need your machine and router are
> clearly marked **(YOU)**.

---

## What's in this folder

```
beammp-server/
├── README.md                     ← this guide
├── ServerConfig.toml             ← your config (paste AuthKey, set Name/Map)
├── start-server.bat              ← double-click to launch on Windows
├── .gitignore                    ← keeps mod zips + your AuthKey out of git
└── Resources/
    ├── Client/                   ← mod .zip files go here (auto-download)
    │   └── README.md
    └── Server/                   ← server-side Lua plugins
        ├── README.md
        └── PredAiHooks/main.lua  ← starter plugin (chat cmds, race timer…)
```

---

## Setup — do this on your laptop

### 1. Get the **dedicated Server** (not the game launcher) **(YOU)**

Download the **BeamMP Server** from <https://beammp.com/> → *Host a Server* /
Downloads. **This is a different download from the BeamMP client launcher** you
use to play — make sure you grab the **Server**.

Put it in a **permanent folder** (not Downloads/Temp). The simplest path:
**copy `BeamMP-Server.exe` into *this* `beammp-server/` folder**, right next to
`ServerConfig.toml` and `start-server.bat`. Then everything lines up.

### 2. Get your AuthKey and put it in the config **(YOU)**

1. Go to <https://keymaster.beammp.com>, log in with Discord.
2. Left side → **Keys** → green **+** → create a key → copy the **AuthKey**.
3. Open `ServerConfig.toml`, find `AuthKey = "PASTE-YOUR-AUTHKEY-HERE"`, and
   paste your real key between the quotes.

> The AuthKey is required even for a private server — without a valid one the
> server prints an `[ERROR]` and won't fully start. **Treat it like a
> password: never share it, never paste it in chat/screenshots, never commit
> it.** See *"Keep your AuthKey out of git"* below.

### 3. Confirm the rest of the config

I built `ServerConfig.toml` to match your brief. Verify these match what you
want (I used placeholders where you didn't give me a value, because **your
original `ServerConfig.toml` never reached me — I couldn't read your actual
values**):

| Setting | Value here | Note |
|---|---|---|
| `Port` | **30814** | ✅ as you specified — forward this, TCP **and** UDP |
| `AuthKey` | *placeholder* | ⬅ paste your real key |
| `Name` | `Pred Ai BMW Server` | change to taste |
| `MaxPlayers` | `8` | conservative for 8 GB; bump if you want |
| `Map` | `/levels/gridmap_v2/info.json` | stock; see custom-map note |
| `Private` | `true` | hidden from public list, friends connect by IP |

### 4. Add the BMW mods **(YOU)**

Copy your mod **`.zip` files — keep them zipped, do NOT extract** — into
`Resources/Client/`:

- `BMW_1M_modland.zip`
- `BMW3_F30-M3-Touring_by_hxmxnn_modland.zip`
- `BMW_E90_M3.etiketbros.zip`
- `mate30_als_modland.zip`  ← ⚠️ see *Flags* below

**Add a few at a time**, launching and joining once between batches so a bad
mod is easy to spot. They auto-download to every joiner — nobody installs
anything by hand.

### 5. Launch and verify a clean start

Double-click **`start-server.bat`** (or run `BeamMP-Server.exe` directly).
You want the console to settle into a "running / listening" state with **no
`[ERROR]` and no `[WARN]`** lines.

- `[ERROR]` = must fix (bad/blank AuthKey, port already in use, broken mod).
- `[WARN]` = usually safe to ignore (e.g. "a newer server version is
  available"), but read it.
- A player getting a **"done"/"start" rejection on join** almost always means
  a broken or incompatible client mod → use the isolation steps in
  *Troubleshooting*.

---

## Network — the physical bits (all **YOU**, on your router/PC)

Your laptop is on **wired ethernet** — good, more stable than wifi for hosting.

1. **Find your laptop's local IP.** `ipconfig` in Command Prompt → the
   **IPv4 Address** of your Ethernet adapter (e.g. `192.168.1.42`).
2. **Pin it** with a **DHCP reservation / static assignment** in your router so
   that IP never changes on reboot (a changed IP silently breaks your forward).
3. **Port-forward `30814` to that local IP — both TCP *and* UDP.** BeamMP uses
   TCP for the initial handshake + mod downloads and UDP for live driving, so
   you need both. Some routers have a single "Both/TCP+UDP" option; others make
   you add two rules.
4. **Allow it through Windows Firewall**, inbound **and** outbound, for the
   server `.exe` (and TCP+UDP 30814). First launch usually pops a firewall
   prompt — tick **Private** *and* **Public** and allow.
5. **Verify from outside while the server is running** using the BeamMP server
   checker (CheckBeamMP) — search "BeamMP server checker" / the tool linked
   from the BeamMP site. Enter your **public IP** and `30814`. Green = the
   outside world can reach you.

---

## How you each connect

- **You (same machine as the server):** BeamMP launcher → **Direct Connect** →
  `127.0.0.1:30814`. (Use loopback, *not* your public IP — see note.)
- **Your friend (Michigan):** you text him your **public IP** + port; he types
  `YOUR.PUBLIC.IP:30814` into his **Direct Connect** tab. There is no invite
  button — direct connect is the whole flow.

> **Find your public IP:** Google "what is my IP" on the host laptop.
> **Note (NAT loopback):** many home routers won't let you reach your *own*
> public IP from inside your LAN. That's why you use `127.0.0.1`. If you ever
> test your public IP from your own laptop and it fails, that's expected and
> does **not** mean your friend can't connect — verify with the external
> checker instead.

---

## ⚠️ Flags — things I'd double-check before you count on this

I was asked to flag anything that looks off. I couldn't see your actual
`ServerConfig.toml` or open your mod zips, so this is based on your brief, the
filenames, and the BeamMP spec:

1. **`mate30_als_modland.zip` isn't an obvious BMW.** "mate30 / als" doesn't
   read like a BMW vehicle. If it's an anti-lag-sound or a parts/config mod
   that *depends on a base vehicle or another mod*, it's a classic cause of the
   "done/start" join rejection. Confirm it's a standalone, BeamMP-compatible
   **vehicle** mod (or just leave it out of the first batch).
2. **8 GB RAM, hosting *and* playing on the same laptop is tight.**
   BeamNG.drive itself wants ~16 GB comfortably; 8 GB is the floor. The server
   process is light, but the *game* on top of it will be your bottleneck. Close
   other apps, keep in-game graphics modest, and that's partly why I set
   `MaxPlayers`/`MaxCars` conservatively. If you stutter, lower them.
3. **First-join download size.** Every new player downloads the **sum** of
   everything in `Resources/Client` over your home **upload** speed. Four
   ~150 MB cars ≈ ~600 MB on your friend's first connect — expect a wait. Your
   "100–200 MB per car, add a few at a time" rule is the right instinct; keep
   to it.
4. **`Map` must actually exist.** I defaulted to the stock `gridmap_v2`. If your
   file pointed at a **custom** map, that map's `.zip` must *also* be in
   `Resources/Client` or joiners can't load in.
5. **`Private = true`** hides you from the public browser (friends-only via IP)
   — matches your goal. You still need a valid AuthKey regardless.

---

## Troubleshooting

**Server won't start / `[ERROR]` about authentication**
→ AuthKey is blank or wrong. Re-copy it from keymaster.beammp.com into
`ServerConfig.toml`.

**`[ERROR]` port already in use**
→ Another process (or a previous server still running) holds 30814. Close it,
or change `Port` in the config (and update your forward + firewall to match).

**Friend can connect but you can't (or vice-versa)**
→ You use `127.0.0.1:30814`; he uses your public IP. If *he* can't get in,
re-check the forward (TCP **and** UDP, pointing at the laptop's *current* local
IP) and the firewall, then re-run the external checker.

**A mod makes joins fail ("done"/"start" rejection)** — isolate it:
1. Move **all** mod zips out of `Resources/Client`. Confirm a clean join.
2. Add them back **one at a time**, joining after each.
3. The one that breaks the join is the culprit — remove/replace it. (Start by
   suspecting flag #1 above.)

**`[WARN]` about a newer server version**
→ Harmless to play, but updating the server is good hygiene (security + mod
compatibility). It does not block anything.

---

## Keep your AuthKey out of git

`ServerConfig.toml` is committed here as a **template** with a placeholder. Once
you paste your **real** AuthKey into it on your laptop, stop git from tracking
that change so you never push your key:

```bash
cd beammp-server
git update-index --skip-worktree ServerConfig.toml
```

`Resources/Client/*.zip` and the server `.exe`/logs are already git-ignored, so
your mods and binaries stay local too.

---

## What's wired for later

- **More vehicle mods:** just drop zips into `Resources/Client` — the layout
  scales without changes.
- **Server-side Lua:** `Resources/Server/PredAiHooks/main.lua` is a working
  scaffold (chat commands + race-timer skeleton) with clearly marked stubs for
  **drift scoring**, **cops-and-robbers**, and **"spawn a random BMW."** Each
  future feature should be its own folder under `Resources/Server/`. See
  `Resources/Server/README.md` for the verified 3.X API.
- **BeamJoy / similar** later: it installs as a server plugin under
  `Resources/Server/`, so this structure is already ready for it.

---

## Sources

- [BeamMP Docs — Server Installation](https://docs.beammp.com/server/create-a-server/)
- [BeamMP Docs — Server Manual](https://docs.beammp.com/server/manual/)
- [BeamMP Docs — Server Lua 3.X reference](https://docs.beammp.com/scripting/server/latest-server-reference/)
- [BeamMP Keymaster (AuthKey)](https://keymaster.beammp.com)
- [BeamMP Wiki — Server installation](https://wiki.beammp.com/en/home/server-installation)
