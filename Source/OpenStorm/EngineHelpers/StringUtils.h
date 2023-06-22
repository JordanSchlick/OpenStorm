
#include <string>
#include "CoreMinimal.h"

namespace StringUtils {
	inline std::string FStringToSTDString(FString inStr){
		return std::string(StringCast<ANSICHAR>(*inStr).Get());
	}
}