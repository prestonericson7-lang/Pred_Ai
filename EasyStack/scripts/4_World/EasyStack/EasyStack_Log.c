class EasyStack_Log
{
    static void Msg(string s)
    {
        if (EasyStack_Settings.LOG)
            Print("[EasyStack] " + s);
    }
}
