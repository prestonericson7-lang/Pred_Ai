//! Break LOS, doorway-stare detection, suppression awareness.
//! BreakLOS samples N candidate points around the AI and picks one where the
//! straight line back to the threat is blocked (cover). Uses GetGame().IsBoxColliding
//! along the segment as a coarse occlusion test (vanilla DayZ).
class PredAI_TacticalDirector
{
    //! Pick a destination on the OPPOSITE side from the threat and route there.
    static void BreakLOS(PredAI_Context ctx)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg)
            return;

        vector threat;
        if (!ResolveThreatPos(ctx, threat))
            return;

        vector here = ctx.ai.GetPosition();
        vector away = here - threat;
        if (vector.Distance(here, threat) < 0.001)
            away = "1 0 0";
        away.Normalize();

        float radius = cfg.tactical.break_los_search_radius;
        int   samples = cfg.tactical.break_los_sample_count;
        if (samples < 4) samples = 4;

        vector best = here + away * radius;
        bool   foundCover = false;
        float  bestDistToThreat = vector.Distance(best, threat);

        //! Sample points in a half-circle pointing away from threat.
        for (int i = 0; i < samples; i++)
        {
            float frac = (i + 1) / Math.Max(1.0, samples + 1);
            float angle = -90.0 + frac * 180.0;
            vector dir = RotateY(away, angle);
            vector candidate = here + dir * radius;

            if (cfg.navigation.surface_snap_y && GetGame())
                candidate[1] = GetGame().SurfaceY(candidate[0], candidate[2]);

            //! Cover means: the line from candidate -> threat is OBSTRUCTED.
            if (IsLineObstructed(candidate, threat))
            {
                float d = vector.Distance(candidate, threat);
                if (!foundCover || d > bestDistToThreat)
                {
                    best = candidate;
                    bestDistToThreat = d;
                    foundCover = true;
                }
            }
        }

        ctx.ai.OverrideTargetPosition(best, false, 2.0, true);
        ctx.since_repath = 0.0;
        ctx.stationary_seconds = 0.0;
        if (foundCover)
            ctx.AddScore(8, "break-los-found-cover");
        else
            ctx.AddScore(2, "break-los-blind-retreat");
    }

    //! True if AI has been stuck near a wall for too long (doorway / window stare).
    static bool IsDoorwayStare(PredAI_Context ctx)
    {
        if (!ctx || !ctx.ai)
            return false;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg)
            return false;

        //! Only count stationary time as doorway-stare if a wall is within reach.
        if (ctx.stationary_seconds < cfg.tactical.doorway_stare_seconds)
            return false;

        vector here = ctx.ai.GetPosition();
        float d = cfg.tactical.doorway_wall_check_distance;
        vector forward = ctx.ai.GetDirection();
        forward.Normalize();
        vector test = here + forward * d;
        test[1] = here[1] + 1.2;

        if (!GetGame())
            return false;

        //! Coarse: any box-collide a meter ahead at chest height = wall.
        return GetGame().IsBoxColliding(test, "0 0 0", "0.3 0.3 0.3", null);
    }

    //! Mirror last_hit_seconds onto suppression flag so CombatDirector can read it.
    static void ApplySuppression(PredAI_Context ctx)
    {
        if (!ctx)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg)
        {
            ctx.suppression_active = false;
            return;
        }

        ctx.suppression_active = ctx.last_hit_seconds < cfg.tactical.suppression_window_seconds;
    }

    //! Returns true once the AI position is set; brain calls BreakLOS, this is a wrapper
    //! used by the Brain cascade to test the precondition.
    static bool ShouldBreakLOS(PredAI_Context ctx)
    {
        if (!ctx || !ctx.ai)
            return false;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg)
            return false;

        if (!ctx.memory || !ctx.memory.has_last_damage_source)
            return false;

        float healthFrac = ctx.ai.GetHealth01("", "");
        if (healthFrac >= cfg.tactical.break_los_health_pct)
            return false;

        vector here = ctx.ai.GetPosition();
        return vector.Distance(here, ctx.memory.last_damage_source_pos) >= cfg.tactical.break_los_min_distance;
    }

    static bool ResolveThreatPos(PredAI_Context ctx, out vector threat)
    {
        threat = "0 0 0";

        if (ctx.memory && ctx.memory.has_last_damage_source)
        {
            threat = ctx.memory.last_damage_source_pos;
            return true;
        }

        if (ctx.last_hit_source)
        {
            threat = ctx.last_hit_source.GetPosition();
            return true;
        }

        if (ctx.target_entity)
        {
            threat = ctx.target_entity.GetPosition();
            return true;
        }

        return false;
    }

    static bool IsLineObstructed(vector a, vector b)
    {
        if (!GetGame())
            return false;

        vector mid = (a + b) * 0.5;
        mid[1] = mid[1] + 1.0;
        //! 0.3 m^3 box at the midpoint: if it overlaps any collider, line is blocked.
        return GetGame().IsBoxColliding(mid, "0 0 0", "0.3 0.3 0.3", null);
    }

    static vector RotateY(vector v, float angleDeg)
    {
        float r = angleDeg * Math.DEG2RAD;
        float c = Math.Cos(r);
        float s = Math.Sin(r);
        return Vector(v[0] * c - v[2] * s, v[1], v[0] * s + v[2] * c);
    }
}
