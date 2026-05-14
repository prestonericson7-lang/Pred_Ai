class PredAIMemorySettings
{
    float threat_memory_seconds;
    float loot_scan_memo_seconds;
    float teammate_death_memo_seconds;
    float last_seen_decay_seconds;
    float purge_interval_seconds;
    int   max_teammate_death_entries;
    int   max_loot_scan_entries;
    float loot_scan_skip_radius;

    void PredAIMemorySettings()
    {
        threat_memory_seconds        = 30.0;
        loot_scan_memo_seconds       = 8.0;
        teammate_death_memo_seconds  = 90.0;
        last_seen_decay_seconds      = 20.0;
        purge_interval_seconds       = 5.0;
        max_teammate_death_entries   = 6;
        max_loot_scan_entries        = 6;
        loot_scan_skip_radius        = 18.0;
    }
}
