class PredAI_ScoreDirector
{
    static void Tick(PredAI_Context ctx, float dt)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (ctx.score_accumulator < cfg.score_eval_seconds)
            return;

        ctx.score_accumulator = 0.0;

        if (ctx.should_bandage)
            ctx.AddScore(cfg.scoring.bleeding_tick, "bleeding-open-risk");

        if (ctx.has_loaded_weapon && ctx.weapon_in_hands)
            ctx.AddScore(2, "armed-ready");
        else if (ctx.has_firearm && !ctx.has_loaded_weapon)
            ctx.AddScore(-8, "weapon-no-ammo");
        else if (!ctx.has_firearm && ctx.has_target)
            ctx.AddScore(-12, "threat-no-weapon");

        if (ctx.has_empty_mag && !ctx.has_loaded_mag && !ctx.has_ammo_pile)
            ctx.AddScore(-5, "empty-mag-no-ammo-source");

        if (ctx.stationary_seconds > cfg.navigation.idle_repath_seconds && !ctx.has_target)
            ctx.AddScore(cfg.scoring.idle_tick, "idle-too-long");
        else if (ctx.stationary_seconds < 1.0)
            ctx.AddScore(cfg.scoring.moving_tick, "moving");

        if (ctx.has_target && !ctx.target_is_body)
            ctx.AddScore(cfg.scoring.target_engaged, "active-threat-engaged");

        if (ctx.target_is_body)
        {
            ctx.body_camp_seconds += cfg.score_eval_seconds;
            if (ctx.body_camp_seconds > cfg.combat.anti_body_camp_seconds)
                ctx.AddScore(cfg.scoring.body_camping, "body-camping");
        }
        else
        {
            ctx.body_camp_seconds = 0.0;
        }

        if (ctx.active_goal == ctx.last_goal && ctx.active_goal != PredAI_GoalType.NONE && ctx.active_goal_seconds > 12.0)
            ctx.AddScore(cfg.scoring.bad_goal_repeat, "goal-stale");

        if (ctx.score <= cfg.scoring.panic_decision_below && ctx.since_repath >= cfg.navigation.repath_seconds)
            PredAI_NavigationDirector.ForceRepath(ctx, "panic-low-score", cfg.navigation.body_camp_break_distance);
    }
}
