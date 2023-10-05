
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
	
	std::string GetRelativePath(FString inString);
	
	// get path relative to project root
	inline std::string GetRelativePath(std::string inString){
		return GetRelativePath(STDStringToFString(inString));
	}
	
	// get path relative to user settings directory, it is AppData/OpenStorm/ on windows
	std::string GetUserPath(FString inString);
}