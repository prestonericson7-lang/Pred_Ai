//! Threat scoring + group/squad cohesion.
//! Uses the real Expansion API: eAIBase.GetGroup() returning eAIGroup,
//! and eAIGroup.GetMember(int)/Count() for member iteration.
//! Source: ai_c_files_expantion_11_2 lines 15204 (GetGroup), 27343 (GetMember), 6136 (Count).
class PredAI_ThreatModel
{
    //! Rank an eAITarget for engagement priority.
    //! Larger = more dangerous / more attractive.
    static int RankThreat(PredAI_Context ctx, eAITarget t)
    {
        if (!ctx || !t)
            return 0;

        float threat = t.GetThreat();
        float dist   = t.GetDistance();

        //! Closer threats score higher (clamp distance to 200 m)
        float closeness = 200.0 - dist;
        if (closeness < 0.0) closeness = 0.0;

        int score = Math.Round(threat * 100.0 + closeness * 0.5);

        //! Bonus if this target lines up with our last-known damage source
        if (ctx.memory && ctx.memory.has_last_damage_source)
        {
            vector tpos = t.GetPosition();
            if (vector.Distance(tpos, ctx.memory.last_damage_source_pos) < 6.0)
                score += 25;
        }

        return score;
    }

    //! Count registered AI in the same Expansion group within radius.
    static int CountFriendliesNearby(PredAI_Context ctx, float radius)
    {
        if (!ctx || !ctx.ai)
            return 0;

        eAIGroup g = ctx.ai.GetGroup();
        if (!g)
            return 0;

        int count = 0;
        vector here = ctx.ai.GetPosition();

        //! Walk Expansion's group member list. Members are DayZPlayerImplement;
        //! cast to eAIBase to confirm AI (skip human players).
        int n = g.Count();
        for (int i = 0; i < n; i++)
        {
            DayZPlayerImplement m = g.GetMember(i);
            if (!m || m == ctx.ai)
                continue;

            eAIBase other;
            if (!Class.CastTo(other, m))
                continue;

            if (!other.IsAlive() || other.IsDamageDestroyed())
                continue;

            if (vector.Distance(other.GetPosition(), here) <= radius)
                count++;
        }

        return count;
    }

    //! Called from EEKilled BEFORE Runtime.Unregister. Penalises every
    //! same-group AI within tactical.cohesion_radius_m and seeds their
    //! memory with the death position so they know where to avoid / retreat to.
    static void NotifyFriendlyDeath(eAIBase deceased)
    {
        if (!deceased)
            return;

        eAIGroup g = deceased.GetGroup();
        if (!g)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg)
            return;

        vector deathPos = deceased.GetPosition();
        float radius    = cfg.tactical.cohesion_radius_m;
        int penalty     = cfg.tactical.friend_death_survival_penalty;

        if (!PredAI_Runtime.s_AI)
            return;

        array<eAIBase> keys = new array<eAIBase>;
        for (int k = 0; k < PredAI_Runtime.s_AI.Count(); k++)
            keys.Insert(PredAI_Runtime.s_AI.GetKey(k));

        foreach (eAIBase other : keys)
        {
            if (!other || other == deceased || !other.IsAlive())
                continue;

            eAIGroup og = other.GetGroup();
            if (og != g)
                continue;

            if (vector.Distance(other.GetPosition(), deathPos) > radius)
                continue;

            PredAI_Context octx;
            if (!PredAI_Runtime.s_AI.Find(other, octx) || !octx)
                continue;

            if (octx.memory)
                octx.memory.RecordTeammateDeath(deathPos);
            octx.AddScore(-penalty, "friend-died");
        }
    }

    //! Bravery + aggression check; gated on having buddies alive nearby.
    static bool ShouldPush(PredAI_Context ctx)
    {
        if (!ctx || !ctx.personality)
            return false;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg)
            return false;

        if (ctx.friendlies_alive_nearby < 2)
            return false;

        return (ctx.personality.bravery + ctx.personality.aggression) >= cfg.tactical.push_bravery_aggression_min;
    }
}
