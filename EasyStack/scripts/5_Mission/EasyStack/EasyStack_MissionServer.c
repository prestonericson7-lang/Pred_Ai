// EasyStack - startup confirmation in the server script log, mirroring the
// pattern used elsewhere in this repo so you can verify the PBO mounted.
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();

        if (EasyStack_Settings.LOG)
        {
            Print("[EasyStack] EasyStack loaded.");
            Print("[EasyStack] AutoStack=" + EasyStack_Settings.ENABLE_AUTO_QUANTITY_STACK.ToString()
                + " DurabilityCombine=" + EasyStack_Settings.ENABLE_DURABILITY_COMBINE.ToString()
                + " AmmoCap=99");
        }
    }
}
