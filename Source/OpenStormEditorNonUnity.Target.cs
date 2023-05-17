// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

using System;
using System.Reflection;

public class OpenStormEditorNonUnityTarget : TargetRules{
	public OpenStormEditorNonUnityTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "OpenStorm" } );
		
		bUseUnityBuild = false;
		//bUsePCHFiles = false;
	}
}
