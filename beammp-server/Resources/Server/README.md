# Resources/Server — server-side Lua plugins

This is where your **server-side Lua plugins** live (drift scoring, race timer,
cops-and-robbers, admin tools, etc.). Nothing here is auto-downloaded to
players — it runs only on the host.

## How plugins are structured

- **Each subfolder of `Resources/Server/` is one plugin.** Example:
  `Resources/Server/PredAiHooks/`.
- A plugin must contain **at least one `.lua` file**. The convention is
  `main.lua`.
- **Top-level `.lua` files hot-reload** when you save changes — no restart
  needed for quick iteration.
- Keep each feature in its **own plugin folder** so you can enable/disable them
  independently (just move a folder out to disable it).

## The lifecycle in one breath

```lua
function onInit()
    -- runs once when the plugin loads; register your handlers here
    MP.RegisterEvent("onChatMessage", "myChatHandler")
end

function myChatHandler(player_id, player_name, message)
    if message == "!ping" then
        MP.SendChatMessage(player_id, "pong")
        return 1   -- return 1 to CANCEL the event (hide the message)
    end
    return 0       -- 0 / nothing = let it through
end
```

## Verified API quick-reference (BeamMP 3.X)

**Register a handler**
- `MP.RegisterEvent(event_name, function_name)`

**Common events** (return `1` from a handler to cancel that event):
| Event | Args |
|-------|------|
| `onInit` | *(none)* |
| `onPlayerConnecting` | `player_id` |
| `onPlayerJoining` | `player_id` |
| `onPlayerDisconnect` | `player_id` |
| `onChatMessage` | `player_id, player_name, message` |
| `onVehicleSpawn` | `player_id, vehicle_id, data` |
| `onVehicleEdited` | `player_id, vehicle_id, data` |
| `onVehicleDeleted` | `player_id, vehicle_id` |
| `onVehicleReset` | `player_id, vehicle_id, data` |
| `onConsoleInput` | `input` |
| `onShutdown` | *(none)* |

**Handy functions**
- `MP.SendChatMessage(player_id, message)` — `player_id = -1` broadcasts to all
- `MP.GetPlayerName(player_id) -> string`
- `MP.GetPlayers() -> table` (id → name)
- `MP.CreateEventTimer(event_name, interval_ms)` / `MP.CancelEventTimer(event_name)`
- `MP.TriggerClientEvent(player_id, event, data)` — call into a client mod

Full reference: <https://docs.beammp.com/scripting/server/latest-server-reference/>

## What's here now

- **`PredAiHooks/`** — a starter plugin: join/leave announce, a `!help / !ping /
  !who / !race` chat-command router, a race-countdown timer, and clearly marked
  stubs for drift scoring, cops-and-robbers, and "spawn a random BMW" (the
  features you'll spec later).
