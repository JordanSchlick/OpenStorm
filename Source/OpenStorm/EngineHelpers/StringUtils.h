
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
}