class PredAI_CombatDirector
{
    static void Execute(PredAI_Context ctx, float dt)
    {
        if (!ctx || !ctx.ai || !ctx.has_target)
            return;

        eAIBase ai = ctx.ai;

        //! GetTarget(0) = primary target from Expansion AI targeting list
        eAITarget target = null;
        if (ai.TargetCount() > 0)
            target = ai.GetTarget(0);
        if (!target)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        float dist = target.GetDistance();

        //! Try to get a loaded, ready weapon
        Weapon_Base gun = ai.eAI_GetAnyWeaponToUse(true, false);

        //! No ready gun but we have ammo pile - trigger weapon setup first
        if (!gun && ctx.best_weapon && (ctx.has_ammo_pile || ctx.has_loaded_mag))
        {
            PredAI_WeaponDirector.Execute(ctx, dt);
            gun = ai.eAI_GetAnyWeaponToUse(true, false);
        }

        if (gun)
        {
            //! Put weapon in hands if setting is on
            if (cfg.combat.force_weapon_select)
                PredAI_WeaponDirector.TryPutWeaponInHands(ctx, gun);

            //! Suppression: stay crouched if recently hit. Lets Expansion's AI engine
            //! still aim/fire but biases stance toward survival.
            if (ctx.suppression_active)
                ai.OverrideStance(DayZPlayerConstants.STANCEIDX_CROUCH, false);

            //! Movement speed scaling by range - Expansion AI handles aiming + firing automatically.
            //! Push tighter if we have squad support, hang back otherwise.
            bool pushing = PredAI_ThreatModel.ShouldPush(ctx);
            if (dist <= cfg.combat.close_range)
                ai.OverrideMovementSpeed(true, 2);
            else if (dist <= cfg.combat.mid_range)
                ai.OverrideMovementSpeed(true, 1);
            else if (pushing)
                ai.OverrideMovementSpeed(true, 1);
            else
                ai.OverrideMovementSpeed(false, 0);
        }
        else
        {
            //! No firearm - melee if close, otherwise run away and find a weapon
            if (dist <= cfg.combat.close_range)
            {
                ai.Notify_Melee(true);
                ai.OverrideMovementSpeed(true, 3);
                ctx.AddScore(2, "melee-pressure");
            }
            else
            {
                ai.SetMovementSpeedLimit(3);
                ctx.AddScore(-4, "combat-no-usable-weapon");
                PredAI_WeaponDirector.Execute(ctx, dt);
                PredAI_LootDirector.Execute(ctx, dt);
            }
        }
    }

    //! Called by Brain when the FLANK goal is chosen. Moves to a side position
    //! relative to the target. Cooldown gated by combat.combat_reposition_seconds.
    static void Flank(PredAI_Context ctx, float dt)
    {
        if (!ctx || !ctx.ai)
            return;

        eAIBase ai = ctx.ai;
        if (ai.TargetCount() <= 0)
        {
            //! Target lost mid-flank - fall through to normal combat handler.
            Execute(ctx, dt);
            return;
        }

        eAITarget target = ai.GetTarget(0);
        if (!target)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (ctx.since_combat_reposition < cfg.combat.combat_reposition_seconds)
        {
            //! Still moving - let the existing combat behaviour drive shoot/aim.
            Execute(ctx, dt);
            return;
        }

        ctx.since_combat_reposition = 0.0;
        vector tpos = target.GetPosition();
        vector flank = PredAI_NavigationDirector.RandomNearby(tpos, cfg.combat.flank_distance);
        ai.OverrideTargetPosition(flank, false, 2.0, true);
        ctx.AddScore(3, "flank-goal-repath");

        //! Still want the AI to shoot while flanking.
        Execute(ctx, dt);
    }
}
