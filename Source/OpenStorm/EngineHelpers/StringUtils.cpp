#include "StringUtils.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformProcess.h"
#else
#include "Linux/LinuxPlatformProcess.h"
#endif

// #include "GenericPlatform/GenericPlatformProcess.h"



// get path relative to project root
std::string StringUtils::GetRelativePath(FString inString){
	FString file =  FPaths::Combine(FPaths::ProjectDir(), inString);
	FString fullFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*file);
	return FStringToSTDString(fullFilePath);
}

std::string StringUtils::GetUserPath(FString inString){
	FString file =  FPaths::Combine(FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("OpenStorm")), inString);
	FString fullFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*file);
	return FStringToSTDString(fullFilePath);
}