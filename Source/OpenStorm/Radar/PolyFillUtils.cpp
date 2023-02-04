
#include <algorithm>

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
	//C++17 specific stuff here
#else
	float std::clamp(float value, float minValue, float maxValue){
		return std::min(std::max(value, minValue), maxValue);
	}
#endif