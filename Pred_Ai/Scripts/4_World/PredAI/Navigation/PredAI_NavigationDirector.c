class PredAI_NavigationDirector
{
    static vector RandomNearby(vector origin, float distance)
    {
        vector p = origin + Vector(Math.RandomFloatInclusive(-distance, distance), 0.0, Math.RandomFloatInclusive(-distance, distance));
        PredAISettings cfg = PredAI_Config.Get();
        if (cfg.navigation.surface_snap_y && GetGame())
            p[1] = GetGame().SurfaceY(p[0], p[2]);

        return p;
    }

    static void ForceRepath(PredAI_Context ctx, string reason, float distance)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg.navigation.anti_stuck_enabled)
            return;

        if (ctx.since_repath < cfg.navigation.repath_seconds)
            return;

        eAIBase ai = ctx.ai;
        vector origin = ai.GetPosition();

        if (ctx.has_target && ctx.target_entity)
            origin = ctx.target_entity.GetPosition();

        vector dest = RandomNearby(origin, distance);
        ai.OverrideTargetPosition(dest, false, 2.0, true);
        ctx.since_repath = 0.0;
        ctx.stationary_seconds = 0.0;
        ctx.AddScore(cfg.scoring.stuck, reason);
    }

    static void BreakBodyCamp(PredAI_Context ctx)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        vector dest = RandomNearby(ctx.ai.GetPosition(), cfg.navigation.body_camp_break_distance);
        ctx.ai.OverrideTargetPosition(dest, false, 2.0, true);
        ctx.body_camp_seconds = 0.0;
        ctx.since_repath = 0.0;
        ctx.AddScore(5, "break-body-camp");
    }

    static void IdlePatrol(PredAI_Context ctx)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (ctx.since_idle_action < cfg.navigation.idle_repath_seconds)
            return;

        vector dest = RandomNearby(ctx.ai.GetPosition(), cfg.navigation.idle_goal_move_distance);
        ctx.ai.OverrideTargetPosition(dest, false, 2.0, true);
        ctx.since_idle_action = 0.0;
        ctx.stationary_seconds = 0.0;
        ctx.AddScore(2, "idle-patrol-move");
    }
}
