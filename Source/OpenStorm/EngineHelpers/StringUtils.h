
#include <string>
#include "CoreMinimal.h"

namespace StringUtils {
	inline std::string FStringToSTDString(FString inStr){
		// return std::string(StringCast<ANSICHAR>(*inStr).Get());
		return std::string(TCHAR_TO_UTF8(*inStr));
	}
	inline FString STDStringToFString(std::string inStr){
		return FString(UTF8_TO_TCHAR(inStr.c_str()));
	}
	
	// get path relative to project root
	inline std::string GetRelativePath(FString inString){
		FString file =  FPaths::Combine(FPaths::ProjectDir(), inString);
		FString fullFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*file);
		return std::string(StringCast<ANSICHAR>(*fullFilePath).Get());
	}
	
	// get path relative to user settings directory, it is AppData/OpenStorm/ on windows
	inline std::string GetUserPath(FString inString){
		FString file =  FPaths::Combine(FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("OpenStorm")), inString);
		FString fullFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*file);
		return std::string(StringCast<ANSICHAR>(*fullFilePath).Get());
	}
}