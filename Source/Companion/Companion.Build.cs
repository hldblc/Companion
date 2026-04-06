// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Companion : ModuleRules
{
	public Companion(ReadOnlyTargetRules Target) : base(Target)
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
			"GameplayAbilities",
			"ModularGameplay",
			"GameplayTasks",
			"NavigationSystem",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
