using UnrealBuildTool;

public class LivingGarden : ModuleRules
{
	public LivingGarden(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",

			// 2D side-view rendering
			"Paper2D",

			// FX
			"Niagara",

			// AI / behavior
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",

			// Procedural world bootstrap
			"PCG",

			// Machine learning for advanced creature cognition
			"NNE",
			"LearningAgents"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});
	}
}
