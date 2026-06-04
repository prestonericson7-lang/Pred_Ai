using UnrealBuildTool;
using System.Collections.Generic;

public class LivingGardenEditorTarget : TargetRules
{
	public LivingGardenEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("LivingGarden");
	}
}
