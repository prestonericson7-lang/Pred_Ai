-- =============================================================================
--  PredAiHooks — starter server-side plugin for the Pred Ai BMW BeamMP server
-- =============================================================================
--  A BeamMP "plugin" is just a folder under Resources/Server/ containing at
--  least one .lua file. Top-level .lua files hot-reload when you edit them.
--
--  This file is a CLEAN SCAFFOLD, not a finished feature set. It gives you:
--    * onInit            — registers everything when the plugin loads
--    * join / leave logs + chat announce
--    * a chat-command router (!help, !ping, !who, !race)
--    * a race-timer skeleton built on MP.CreateEventTimer
--    * clearly-marked stubs for the features you said you'd spec later
--      (drift scoring, cops-and-robbers, "spawn a random BMW")
--
--  API verified against the BeamMP 3.X server reference:
--    https://docs.beammp.com/scripting/server/latest-server-reference/
-- =============================================================================

-- ---------------------------------------------------------------------------
--  onInit — called automatically once when the plugin is loaded.
--  This is where you register every event handler.
-- ---------------------------------------------------------------------------
function onInit()
    print("[PredAiHooks] loaded.")

    -- player_id-based events
    MP.RegisterEvent("onPlayerJoining",   "PA_onPlayerJoining")
    MP.RegisterEvent("onPlayerDisconnect","PA_onPlayerDisconnect")

    -- chat: returning 1 from the handler cancels (hides) the message
    MP.RegisterEvent("onChatMessage",     "PA_onChatMessage")

    -- vehicle spawns (handy later for drift scoring / car restrictions)
    MP.RegisterEvent("onVehicleSpawn",    "PA_onVehicleSpawn")
end

-- ---------------------------------------------------------------------------
--  Join / leave
-- ---------------------------------------------------------------------------
function PA_onPlayerJoining(player_id)
    local name = MP.GetPlayerName(player_id) or ("#" .. player_id)
    print("[PredAiHooks] joining: " .. name)
    -- -1 = broadcast to everyone
    MP.SendChatMessage(-1, name .. " is joining — say hi. Type !help for commands.")
end

function PA_onPlayerDisconnect(player_id)
    local name = MP.GetPlayerName(player_id) or ("#" .. player_id)
    print("[PredAiHooks] left: " .. name)
end

-- ---------------------------------------------------------------------------
--  Chat command router
--  Commands start with "!". Return 1 to hide the command from chat,
--  return 0 (or nothing) to let normal chat through.
-- ---------------------------------------------------------------------------
function PA_onChatMessage(player_id, player_name, message)
    if message:sub(1, 1) ~= "!" then
        return 0 -- normal chat, let it through
    end

    local cmd = message:match("^!(%S+)") or ""
    cmd = cmd:lower()

    if cmd == "help" then
        MP.SendChatMessage(player_id, "Commands: !help  !ping  !who  !race")
    elseif cmd == "ping" then
        MP.SendChatMessage(player_id, "pong")
    elseif cmd == "who" then
        local names = {}
        for _, n in pairs(MP.GetPlayers()) do names[#names + 1] = n end
        MP.SendChatMessage(player_id, "Online (" .. #names .. "): " .. table.concat(names, ", "))
    elseif cmd == "race" then
        PA_startRaceCountdown()
    else
        MP.SendChatMessage(player_id, "Unknown command '" .. cmd .. "'. Try !help")
    end

    return 1 -- hide the "!command" itself from everyone's chat
end

-- ---------------------------------------------------------------------------
--  Race timer skeleton
--  Counts "3..2..1..GO" using a repeating event timer, then cancels itself.
-- ---------------------------------------------------------------------------
local raceCount = 0

function PA_startRaceCountdown()
    raceCount = 3
    MP.SendChatMessage(-1, "Race starting in...")
    -- fire PA_raceTick once per second (1000 ms)
    MP.RegisterEvent("PA_raceTick", "PA_raceTick")
    MP.CreateEventTimer("PA_raceTick", 1000)
end

function PA_raceTick()
    if raceCount > 0 then
        MP.SendChatMessage(-1, tostring(raceCount) .. "...")
        raceCount = raceCount - 1
    else
        MP.SendChatMessage(-1, "GO!")
        MP.CancelEventTimer("PA_raceTick")
    end
end

-- ---------------------------------------------------------------------------
--  Vehicle spawn hook (foundation for later features)
-- ---------------------------------------------------------------------------
function PA_onVehicleSpawn(player_id, vehicle_id, data)
    -- 'data' is the vehicle JSON. Parse it here later to e.g. restrict to BMWs,
    -- track which car a player is in for drift scoring, etc.
    -- Return 1 to BLOCK a spawn (e.g. a banned vehicle).
    return 0
end

-- =============================================================================
--  TODO — features you said you'd spec separately. Stubbed on purpose.
-- =============================================================================
--  * Drift scoring:   needs live vehicle telemetry (speed/yaw/slip-angle),
--                     which lives client-side. Plan: a small client mod that
--                     samples drift state and reports up via a triggered
--                     event the server tallies here.
--  * Cops & robbers:  team assignment on join + win/lose conditions; build on
--                     PA_onPlayerJoining and the chat router above.
--  * "Random BMW":    spawning a *specific* vehicle is a client-side action;
--                     the server would push a request to the client via
--                     MP.TriggerClientEvent and a paired client-mod handler.
--  Keep each of these as its own folder under Resources/Server/ so they stay
--  independent and easy to enable/disable.
-- =============================================================================
