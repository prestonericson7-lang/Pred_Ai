class PredAI_MedicalDirector
{
    static void Execute(PredAI_Context ctx, bool emergency)
    {
        if (!ctx || !ctx.ai)
            return;

        PredAISettings cfg = PredAI_Config.Get();
        if (!cfg.medical.force_bandage || !ctx.should_bandage)
            return;

        eAIBase ai = ctx.ai;

        //! ctx.best_bandage is populated each tick by PredAI_WeaponDirector.ReadInventoryState
        ItemBase bandage = ctx.best_bandage;

        if (!bandage)
        {
            //! No bandage in inventory - penalise and go loot one
            ctx.AddScore(-cfg.medical.no_bandage_loot_bias, "bleeding-no-bandage");
            PredAI_LootDirector.Execute(ctx, cfg.tick_seconds);
            return;
        }

        //! Slow down and crouch to self-treat
        ai.Notify_Melee(false);
        ai.OverrideMovementSpeed(true, 1);
        ai.OverrideStance(DayZPlayerConstants.STANCEIDX_CROUCH, false);

        //! In emergency: move away from the fight first
        if (emergency && ctx.since_repath >= 1.0)
        {
            ctx.since_repath = 0.0;
            vector safe = PredAI_NavigationDirector.RandomNearby(ai.GetPosition(), cfg.medical.emergency_move_distance);
            ai.OverrideTargetPosition(safe, false, 1.0, true);
        }

        //! Swap current hands item for the bandage
        ItemBase hands = ItemBase.Cast(ai.GetItemInHands());
        if (hands && hands != bandage)
        {
            if (!hands.IsDamageDestroyed())
                ai.eAI_TakeItemToInventory(hands);
        }

        hands = ItemBase.Cast(ai.GetItemInHands());
        if (hands != bandage)
        {
            if (ctx.since_bandage_attempt >= cfg.medical.bandage_action_cooldown)
            {
                ctx.since_bandage_attempt = 0.0;
                if (ai.eAI_TakeItemToHands(bandage))
                {
                    //! Expansion AI will automatically use a bandage-type item held in hands
                    ctx.AddScore(10, "bandage-to-hands");
                }
                else
                {
                    ctx.AddScore(-8, "bandage-to-hands-failed");
                }
            }
        }
        //! If bandage IS in hands, Expansion AI handles the use action automatically.
        //! No manual StartActionObject call needed.
    }
}
