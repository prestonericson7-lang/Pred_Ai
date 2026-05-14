modded class eAIBase
{
    override void Init()
    {
        super.Init();

        if (g_Game && g_Game.IsServer())
            PredAI_Runtime.Register(this);
    }

    //! V11 FIX: real signature is (bool doSim, float timeslice).
    //! V10 used (float pDt) which never bound - the brain was silently dead.
    //! Source: ai_c_files_expantion_11_2 line 7155 + 32834.
    override void eAI_OnUpdate(bool doSim, float timeslice)
    {
        super.eAI_OnUpdate(doSim, timeslice);

        if (!doSim)
            return;

        if (g_Game && g_Game.IsServer())
            PredAI_Runtime.Tick(this, timeslice);
    }

    override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
    {
        super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

        if (g_Game && g_Game.IsServer())
            PredAI_Runtime.OnHit(this, source);
    }

    override void EEKilled(Object killer)
    {
        //! Notify squad BEFORE unregister so the cohesion sweep still sees this AI's group.
        if (g_Game && g_Game.IsServer())
            PredAI_ThreatModel.NotifyFriendlyDeath(this);

        super.EEKilled(killer);

        if (g_Game && g_Game.IsServer())
            PredAI_Runtime.OnKilled(this, killer);
    }

    override void eAI_OnInventoryEnter(ItemBase item)
    {
        super.eAI_OnInventoryEnter(item);

        if (g_Game && g_Game.IsServer())
            PredAI_Runtime.OnInventoryEnter(this, item);
    }
}
