
#include <string>
#include "CoreMinimal.h"

namespace StringUtils {
	// convert FString to std::string
	inline std::string FStringToSTDString(FString inStr){
		// return std::string(StringCast<ANSICHAR>(*inStr).Get());
		return std::string(TCHAR_TO_UTF8(*inStr));
	}
	
	//convert std::string to FString
	inline FString STDStringToFString(std::string inStr){
		return FString(UTF8_TO_TCHAR(inStr.c_str()));
	}
	
	// get path relative to project root
	std::string GetRelativePath(FString inString);
	
	// get path relative to project root
	inline std::string GetRelativePath(std::string inString){
		return GetRelativePath(STDStringToFString(inString));
	}
	
	// get path relative to user settings directory, it is AppData/OpenStorm/ on windows
	std::string GetUserPath(FString inString);
	
	// get path relative to user settings directory, it is AppData/OpenStorm/ on windows
	inline std::string GetUserPath(std::string inString){
		return GetUserPath(STDStringToFString(inString));
	}
}