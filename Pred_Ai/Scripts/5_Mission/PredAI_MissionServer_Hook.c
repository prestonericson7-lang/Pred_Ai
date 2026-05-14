modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();

        //! Start Pred_Ai runtime once on server init
        PredAI_Runtime.Start();
    }
}
