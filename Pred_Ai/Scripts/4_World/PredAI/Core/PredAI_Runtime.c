class PredAI_Runtime
{
    static ref map<eAIBase, ref PredAI_Context> s_AI = new map<eAIBase, ref PredAI_Context>();
    static bool s_Started;

    static void Start()
    {
        if (s_Started)
            return;

        s_Started = true;
        PredAI_Config.Load();
        Print("[Pred_Ai] V11 runtime started - direct eAIBase hook active.");
    }

    static PredAI_Context Register(eAIBase ai)
    {
        if (!ai)
            return null;

        Start();

        PredAI_Context ctx;
        if (!s_AI.Find(ai, ctx))
        {
            ctx = new PredAI_Context(ai);
            s_AI.Insert(ai, ctx);
            PredAI_Debug.Log("registered " + ai.ToString());
        }

        return ctx;
    }

    static void Unregister(eAIBase ai)
    {
        if (!ai || !s_AI)
            return;

        if (s_AI.Contains(ai))
        {
            s_AI.Remove(ai);
            PredAI_Debug.Log("unregistered " + ai.ToString());
        }
    }

    static void Tick(eAIBase ai, float dt)
    {
        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg || !cfg.enabled || !ai || !ai.IsAlive())
            return;

        PredAI_Context ctx = Register(ai);
        if (!ctx)
            return;

        ctx.tick_accumulator += dt;
        if (ctx.tick_accumulator < cfg.tick_seconds)
            return;

        float step = ctx.tick_accumulator;
        ctx.tick_accumulator = 0.0;

        //! Age memory first so sensors see fresh purge state.
        if (ctx.memory)
            ctx.memory.Tick(step);

        PredAI_Sensors.Update(ctx, step);
        PredAI_ScoreDirector.Tick(ctx, step);
        PredAI_Brain.Tick(ctx, step);
    }

    static void OnKilled(eAIBase ai, Object killer)
    {
        PredAI_Context ctx;
        if (s_AI && s_AI.Find(ai, ctx) && ctx)
            ctx.AddScore(PredAI_Config.Get().scoring.target_killed, "death-scored");

        Unregister(ai);
    }

    static void OnHit(eAIBase ai, EntityAI source)
    {
        PredAI_Context ctx;
        if (!s_AI || !s_AI.Find(ai, ctx) || !ctx)
            return;

        ctx.last_hit_seconds = 0.0;
        ctx.AddScore(-6, "hit-by-threat");
        if (source)
        {
            ctx.last_hit_source = source;
            if (ctx.memory)
                ctx.memory.SetLastDamageSource(source.GetPosition());
        }
    }

    static void OnInventoryEnter(eAIBase ai, ItemBase item)
    {
        PredAI_Context ctx;
        if (!s_AI || !s_AI.Find(ai, ctx) || !ctx || !item)
            return;

        ctx.AddScore(PredAI_InventoryDirector.ScoreItem(ai, item), "inventory-enter");
    }
}
