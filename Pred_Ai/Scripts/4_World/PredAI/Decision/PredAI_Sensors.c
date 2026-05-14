class PredAI_Sensors
{
    static void Update(PredAI_Context ctx, float dt)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        eAIBase ai = ctx.ai;

        //! Movement/stuck detection
        vector p = ai.GetPosition();
        float moved = vector.Distance(p, ctx.last_position);
        if (moved < cfg.navigation.move_threshold_meters)
            ctx.stationary_seconds += dt;
        else
        {
            ctx.stationary_seconds = 0.0;
            ctx.last_position = p;
        }

        //! Health sensors - safe vanilla DayZ calls
        ctx.health   = ai.GetHealth("", "");
        ctx.blood01  = ai.GetHealth01("", "Blood");

        //! Bandage need: blood below threshold OR health low
        //! NOTE: We check blood01 first; 0.90 = 90% blood volume, detects active bleed early.
        ctx.should_bandage = (ctx.blood01 < 0.90 || ctx.health < cfg.medical.health_low);

        //! Weapon inventory state (also sets has_firearm, has_loaded_weapon, etc.)
        ctx.has_firearm       = false;
        ctx.has_weapon_ready  = false;
        ctx.has_loaded_weapon = false;
        ctx.has_unloaded_weapon = false;
        ctx.has_ammo          = false;
        ctx.has_loaded_mag    = false;
        ctx.has_empty_mag     = false;
        ctx.has_ammo_pile     = false;
        ctx.weapon_in_hands   = false;
        ctx.best_weapon       = null;
        ctx.ready_weapon      = null;
        ctx.best_mag          = null;
        ctx.best_ammo_pile    = null;
        ctx.best_bandage      = null;
        ctx.has_bandage       = false;

        PredAI_WeaponDirector.ReadInventoryState(ctx);

        //! Target sensors
        ctx.has_target      = false;
        ctx.target_is_body  = false;  //! eAITarget body detection requires deeper Expansion API; kept safe
        ctx.target_distance = 9999.0;
        ctx.target_threat   = 0.0;

        ctx.target_entity = null;
        eAITarget target = null;
        if (ai.TargetCount() > 0)
            target = ai.GetTarget(0);

        if (target)
        {
            ctx.has_target         = true;
            ctx.target_seen_seconds += dt;
            ctx.target_distance    = target.GetDistance();
            ctx.target_threat      = target.GetThreat();

            Object tobj = target.GetObject();
            EntityAI tent;
            if (tobj && Class.CastTo(tent, tobj))
                ctx.target_entity = tent;

            //! Memorise last-seen position for future BreakLOS / loot decisions.
            if (ctx.memory)
                ctx.memory.SetLastSeenTarget(target.GetPosition());
        }
        else
        {
            ctx.target_seen_seconds = 0.0;
        }

        //! Squad cohesion read once per tick - used by Brain and CombatDirector.
        ctx.friendlies_alive_nearby = PredAI_ThreatModel.CountFriendliesNearby(ctx, cfg.tactical.cohesion_radius_m);

        //! Accumulate all timers
        ctx.since_repath            += dt;
        ctx.since_idle_action       += dt;
        ctx.since_bandage_attempt   += dt;
        ctx.since_loot_scan         += dt;
        ctx.since_combat_reposition += dt;
        ctx.since_direct_fire       += dt;
        ctx.since_weapon_setup      += dt;
        ctx.active_goal_seconds     += dt;
        ctx.last_hit_seconds        += dt;
        ctx.decision_accumulator    += dt;
        ctx.score_accumulator       += dt;
    }
}
