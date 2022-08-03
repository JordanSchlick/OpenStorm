// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class OpenStormTarget : TargetRules
{
	public OpenStormTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "OpenStorm" } );
		
		bUseUnityBuild = false;
		bUsePCHFiles = false;
		
		
		//SourceFileWorkingSet.Provider = "None";
		//SourceFileWorkingSet.RepositoryPath = "";
		//SourceFileWorkingSet.GitPath = "";
	}
}
