// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MusicGame : ModuleRules
{
	public MusicGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
            "SlateCore",
			"Niagara"
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MusicGame",
			"MusicGame/Variant_Platforming",
			"MusicGame/Variant_Platforming/Animation",
			"MusicGame/Variant_Combat",
			"MusicGame/Variant_Combat/AI",
			"MusicGame/Variant_Combat/Animation",
			"MusicGame/Variant_Combat/Gameplay",
			"MusicGame/Variant_Combat/Interfaces",
			"MusicGame/Variant_Combat/UI",
			"MusicGame/Variant_SideScrolling",
			"MusicGame/Variant_SideScrolling/AI",
			"MusicGame/Variant_SideScrolling/Gameplay",
			"MusicGame/Variant_SideScrolling/Interfaces",
			"MusicGame/Variant_SideScrolling/UI",
			"MusicGame/Rhythm"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
