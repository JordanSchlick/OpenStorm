#include "RadarColorIndex.h"

#include "SystemAPI.h"

#include <math.h>
#include <cmath>
#include <algorithm>

void RadarColorIndexParams::fromRadarData(RadarData* radarData) {
	maxValue = radarData->maxValue;
	minValue = radarData->minValue;
}


RadarColorIndexResult RadarColorIndex::relativeHue(RadarColorIndexParams params) {
	RadarColorIndexResult result = {};
	result.lower = params.minValue;
	result.upper = params.maxValue;
	result.data = new float[655360]();
	
	for (int i = 0; i < 16384; i++) {
		float value = (i / 16383.0f);
		float hue = 0.85 - fmod(value, 1.0) * 0.9;
		float R = std::clamp(std::abs(hue * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f);
		float G = std::clamp(2 - std::abs(hue * 6.0f - 2.0f), 0.0f, 1.0f);
		float B = std::clamp(2 - std::abs(hue * 6.0f - 4.0f), 0.0f, 1.0f);
		//R = i;
		//G = i;
		//B = i;
		result.data[i * 4 + 0] = R;
		result.data[i * 4 + 1] = G;
		result.data[i * 4 + 2] = B;
		result.data[i * 4 + 3] = value;
		//fprintf(stderr,"%f\n",value);
	}

	return result;
};

RadarColorIndexResult RadarColorIndex::relativeHueAcid(RadarColorIndexParams params) {
	RadarColorIndexResult result = {};
	result.lower = 0.0;
	result.upper = 1.0;
	result.data = new float[655360]();
	
	for (int i = 0; i < 16384; i++) {
		float value = (i / 16383.0f);
		float hue = 1 - fmod(value + SystemAPI::currentTime(), 1.0);
		float R = std::clamp(std::abs(hue * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f);
		float G = std::clamp(2 - std::abs(hue * 6.0f - 2.0f), 0.0f, 1.0f);
		float B = std::clamp(2 - std::abs(hue * 6.0f - 4.0f), 0.0f, 1.0f);
		//R = i;
		//G = i;
		//B = i;
		result.data[i * 4 + 0] = R;
		result.data[i * 4 + 1] = G;
		result.data[i * 4 + 2] = B;
		result.data[i * 4 + 3] = value;
		//fprintf(stderr,"%f\n",value);
	}

	return result;
};


