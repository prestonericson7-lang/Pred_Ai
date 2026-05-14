//! Per-AI threat / activity memory. Aged each tick; entries purged after TTL.
class PredAI_Memory
{
    vector last_damage_source_pos;
    float  last_damage_source_age;
    bool   has_last_damage_source;

    vector last_seen_target_pos;
    float  last_seen_target_age;
    bool   has_last_seen_target;

    ref array<vector> teammate_death_positions;
    ref array<float>  teammate_death_ages;

    ref array<vector> recent_loot_scan_centers;
    ref array<float>  recent_loot_scan_ages;

    float purge_accumulator;

    void PredAI_Memory()
    {
        last_damage_source_pos      = "0 0 0";
        last_damage_source_age      = 9999.0;
        has_last_damage_source      = false;

        last_seen_target_pos        = "0 0 0";
        last_seen_target_age        = 9999.0;
        has_last_seen_target        = false;

        teammate_death_positions    = new array<vector>;
        teammate_death_ages         = new array<float>;
        recent_loot_scan_centers    = new array<vector>;
        recent_loot_scan_ages       = new array<float>;
        purge_accumulator           = 0.0;
    }

    void SetLastDamageSource(vector pos)
    {
        last_damage_source_pos = pos;
        last_damage_source_age = 0.0;
        has_last_damage_source = true;
    }

    void SetLastSeenTarget(vector pos)
    {
        last_seen_target_pos = pos;
        last_seen_target_age = 0.0;
        has_last_seen_target = true;
    }

    void RecordTeammateDeath(vector pos)
    {
        PredAISettings cfg = PredAI_Config.Get();
        teammate_death_positions.Insert(pos);
        teammate_death_ages.Insert(0.0);
        while (teammate_death_positions.Count() > cfg.memory.max_teammate_death_entries)
        {
            teammate_death_positions.Remove(0);
            teammate_death_ages.Remove(0);
        }
    }

    void RecordLootScan(vector center)
    {
        PredAISettings cfg = PredAI_Config.Get();
        recent_loot_scan_centers.Insert(center);
        recent_loot_scan_ages.Insert(0.0);
        while (recent_loot_scan_centers.Count() > cfg.memory.max_loot_scan_entries)
        {
            recent_loot_scan_centers.Remove(0);
            recent_loot_scan_ages.Remove(0);
        }
    }

    bool HasRecentLootScan(vector here)
    {
        PredAISettings cfg = PredAI_Config.Get();
        float radius = cfg.memory.loot_scan_skip_radius;
        for (int i = 0; i < recent_loot_scan_centers.Count(); i++)
        {
            if (recent_loot_scan_ages[i] > cfg.memory.loot_scan_memo_seconds)
                continue;
            if (vector.Distance(recent_loot_scan_centers[i], here) < radius)
                return true;
        }
        return false;
    }

    int CountNearbyTeammateDeaths(vector here, float radius)
    {
        int hits = 0;
        for (int i = 0; i < teammate_death_positions.Count(); i++)
        {
            if (vector.Distance(teammate_death_positions[i], here) < radius)
                hits++;
        }
        return hits;
    }

    void Tick(float dt)
    {
        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg)
            return;

        last_damage_source_age += dt;
        if (last_damage_source_age > cfg.memory.threat_memory_seconds)
            has_last_damage_source = false;

        last_seen_target_age += dt;
        if (last_seen_target_age > cfg.memory.last_seen_decay_seconds)
            has_last_seen_target = false;

        purge_accumulator += dt;
        if (purge_accumulator < cfg.memory.purge_interval_seconds)
        {
            //! Still age all entries each tick.
            for (int a = 0; a < teammate_death_ages.Count(); a++)
                teammate_death_ages[a] = teammate_death_ages[a] + dt;
            for (int b = 0; b < recent_loot_scan_ages.Count(); b++)
                recent_loot_scan_ages[b] = recent_loot_scan_ages[b] + dt;
            return;
        }

        purge_accumulator = 0.0;

        for (int i = teammate_death_positions.Count() - 1; i >= 0; i--)
        {
            teammate_death_ages[i] = teammate_death_ages[i] + dt;
            if (teammate_death_ages[i] > cfg.memory.teammate_death_memo_seconds)
            {
                teammate_death_positions.Remove(i);
                teammate_death_ages.Remove(i);
            }
        }

        for (int j = recent_loot_scan_centers.Count() - 1; j >= 0; j--)
        {
            recent_loot_scan_ages[j] = recent_loot_scan_ages[j] + dt;
            if (recent_loot_scan_ages[j] > cfg.memory.loot_scan_memo_seconds)
            {
                recent_loot_scan_centers.Remove(j);
                recent_loot_scan_ages.Remove(j);
            }
        }
    }
}
