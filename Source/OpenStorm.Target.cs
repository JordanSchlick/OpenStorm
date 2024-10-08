// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class OpenStormTarget : TargetRules
{
	public OpenStormTarget( TargetInfo Target) : base(Target)
	{
		Name = "OpenStorm";
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.AddRange( new string[] { "OpenStorm" } );
		
		bUseUnityBuild = false;
		bUsePCHFiles = false;
		
		
		//SourceFileWorkingSet.Provider = "None";
		//SourceFileWorkingSet.RepositoryPath = "";
		//SourceFileWorkingSet.GitPath = "";
	}
}
