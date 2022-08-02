// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomShaders.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCustomShadersModule"

void FCustomShadersModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString ShaderDir = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("/CustomShaders/Shaders"));
	AddShaderSourceDirectoryMapping("/CustomShaders/Shaders", ShaderDir);

	FString ProjectShaderDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("/Shaders"));
	AddShaderSourceDirectoryMapping("/Shaders", ProjectShaderDir);


	UE_LOG(LogTemp, Display, TEXT("CustomShaders: /Shaders Mapped"));
}

void FCustomShadersModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCustomShadersModule, CustomShaders)