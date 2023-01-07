// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OpenStorm : ModuleRules
{
	public OpenStorm(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "RenderCore", "Engine",  "Slate", "SlateCore"});

		PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "ImGui", "AudioCaptureCore" });
		
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
