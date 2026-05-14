class PredAI_Brain
{
    static void Tick(PredAI_Context ctx, float dt)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();

        //! Suppression flag is derived every tick - cheap, no cooldown.
        PredAI_TacticalDirector.ApplySuppression(ctx);

        //! Between decisions: keep executing the active goal
        if (ctx.decision_accumulator < cfg.brain_decision_cooldown)
        {
            ExecuteGoal(ctx, cfg, ctx.active_goal, dt);
            return;
        }

        ctx.decision_accumulator = 0.0;
        int goal = ChooseGoal(ctx, cfg);
        ctx.SetGoal(goal, BuildReason(ctx));
        ExecuteGoal(ctx, cfg, goal, dt);
    }

    //! 13-level priority cascade - order matches the brief exactly.
    static int ChooseGoal(PredAI_Context ctx, PredAISettings cfg)
    {
        //! 1. EMERGENCY_MEDICAL - critical health/blood + bandage available
        if (ctx.should_bandage && ctx.has_bandage &&
            (ctx.health <= cfg.medical.health_critical || ctx.blood01 <= cfg.medical.blood_low01))
            return PredAI_GoalType.EMERGENCY_MEDICAL;

        //! 2. COMBAT (bleeding-fight) - brave personality + close target + bandage
        if (ctx.should_bandage && ctx.has_bandage && ctx.has_target &&
            ctx.personality.WillRiskBleedingFight(ctx.health, ctx.target_distance))
            return PredAI_GoalType.COMBAT;

        //! 3. MEDICAL - bleeding with bandage
        if (ctx.should_bandage && ctx.has_bandage)
            return PredAI_GoalType.MEDICAL;

        //! 4. LOOT (for bandage) - bleeding, no bandage
        if (ctx.should_bandage && !ctx.has_bandage)
            return PredAI_GoalType.LOOT;

        //! 5. BREAK_LOS - hurt enough + known threat + enough range to escape
        if (PredAI_TacticalDirector.ShouldBreakLOS(ctx))
            return PredAI_GoalType.BREAK_LOS;

        //! 6. BREAK_BODY_CAMP - target is a corpse and we've been here too long
        if (ctx.target_is_body && ctx.body_camp_seconds >= cfg.combat.anti_body_camp_seconds)
            return PredAI_GoalType.BREAK_BODY_CAMP;

        //! 7. REPOSITION - stuck OR doorway-stare
        if (ctx.stationary_seconds >= cfg.navigation.stuck_seconds ||
            PredAI_TacticalDirector.IsDoorwayStare(ctx))
            return PredAI_GoalType.REPOSITION;

        //! 8. WEAPON_SETUP - no loaded weapon + target or stationary
        if (PredAI_WeaponDirector.NeedsWeaponSetup(ctx) &&
            (ctx.has_target || !ctx.has_loaded_weapon || ctx.stationary_seconds > 2.5))
            return PredAI_GoalType.WEAPON_SETUP;

        //! 9. COMBAT - actual threat that meets engage threshold
        if (ctx.has_target && ctx.target_threat >= cfg.combat.min_threat_to_engage)
        {
            //! 10. FLANK - combat + stationary too long becomes a separate goal
            if (ctx.stationary_seconds >= cfg.combat.flank_after_stationary_seconds)
                return PredAI_GoalType.FLANK;

            return PredAI_GoalType.COMBAT;
        }

        //! 11. LOOT - no weapon OR greedy + safe
        if (cfg.loot.enabled && (!ctx.has_firearm || (!ctx.has_loaded_weapon && !ctx.has_ammo) ||
            ctx.personality.greed > 0.78))
            return PredAI_GoalType.LOOT;

        //! 12. IDLE_PATROL - stationary too long with nothing else to do
        if (ctx.stationary_seconds >= cfg.navigation.idle_repath_seconds)
            return PredAI_GoalType.IDLE_PATROL;

        //! 13. NONE
        return PredAI_GoalType.NONE;
    }

    static string BuildReason(PredAI_Context ctx)
    {
        string r = "score=" + ctx.score.ToString();
        if (ctx.should_bandage)        r += " bleeding";
        if (ctx.has_target)            r += " target";
        if (ctx.target_is_body)        r += " body";
        if (ctx.suppression_active)    r += " suppressed";
        if (ctx.friendlies_alive_nearby > 0) r += " buddies=" + ctx.friendlies_alive_nearby.ToString();
        if (!ctx.has_firearm)          r += " no_weapon";
        if (ctx.has_firearm && !ctx.has_loaded_weapon) r += " weapon_needs_ammo";
        if (ctx.has_empty_mag && !ctx.has_loaded_mag && !ctx.has_ammo_pile) r += " empty_mag_only";
        if (ctx.has_loaded_weapon && !ctx.weapon_in_hands) r += " weapon_not_hands";
        if (ctx.stationary_seconds > 3.0) r += " stationary=" + ctx.stationary_seconds.ToString();
        return r;
    }

    static void ExecuteGoal(PredAI_Context ctx, PredAISettings cfg, int goal, float dt)
    {
        switch (goal)
        {
            case PredAI_GoalType.EMERGENCY_MEDICAL:
                PredAI_MedicalDirector.Execute(ctx, true);
                return;
            case PredAI_GoalType.MEDICAL:
                PredAI_MedicalDirector.Execute(ctx, false);
                return;
            case PredAI_GoalType.COMBAT:
                PredAI_CombatDirector.Execute(ctx, dt);
                return;
            case PredAI_GoalType.FLANK:
                PredAI_CombatDirector.Flank(ctx, dt);
                return;
            case PredAI_GoalType.LOOT:
                PredAI_LootDirector.Execute(ctx, dt);
                return;
            case PredAI_GoalType.REPOSITION:
                PredAI_NavigationDirector.ForceRepath(ctx, "stuck-reposition", cfg.navigation.idle_goal_move_distance);
                return;
            case PredAI_GoalType.BREAK_BODY_CAMP:
                PredAI_NavigationDirector.BreakBodyCamp(ctx);
                return;
            case PredAI_GoalType.BREAK_LOS:
                PredAI_TacticalDirector.BreakLOS(ctx);
                return;
            case PredAI_GoalType.IDLE_PATROL:
                PredAI_NavigationDirector.IdlePatrol(ctx);
                return;
            case PredAI_GoalType.WEAPON_SETUP:
                PredAI_WeaponDirector.Execute(ctx, dt);
                return;
        }
    }
}
