#include "RadarColorIndex.h"

#include "SystemAPI.h"

#include <math.h>
#include <cmath>
#include <algorithm>

typedef struct float3 {
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
} float3;

float _HSLToRGBhue2rgb(float p, float q, float t) {
	if (t < 0.0f){
		t += 1.0f;
	}
	if (t > 1.0f){
		t -= 1.0f;
	}
	if (t < 0.16666666666f){
		return p + (q - p) * 6.0f * t;
	}
	if (t < 0.5f){
		return q;
	}
	if (t < 0.6666666666f){
		return p + (q - p) * (0.6666666666f - t) * 6.0f;
	}
	return p;
};


float3 HSLToRGB(float h, float s, float l){
	float r, g, b;
	if (s == 0.0f) {
		r = g = b = l; // achromatic
	} else {
		float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
		float p = 2.0f * l - q;

		r = _HSLToRGBhue2rgb(p, q, h + 0.333333333);
		g = _HSLToRGBhue2rgb(p, q, h);
		b = _HSLToRGBhue2rgb(p, q, h - 0.333333333);
	}
	float3 rgb = {r,g,b};
	return rgb;
}

inline float lerp(float value1, float value2, float amount){
	return value1 * (1 - amount) + value2 * amount;
}

void colorRangeHSL(float* buffer, int startIndex, int endIndex, float h1, float s1, float l1, float h2, float s2, float l2){
	int range = endIndex - startIndex - 1;
	int rangeF = range;
	int outLocation = startIndex * 4;
	for(int i = 0; i <= range; i++){
		float between = (float)i / rangeF;
		float3 rgb = HSLToRGB(lerp(h1,h2,between), lerp(s1,s2,between), lerp(l1,l2,between));
		buffer[outLocation] = rgb.x;
		buffer[outLocation + 1] = rgb.y;
		buffer[outLocation + 2] = rgb.z;
		outLocation += 4;
	}
}

int valueToIndex(float lowerBound, float upperBound, float value){
	return std::clamp((value - lowerBound) / (upperBound - lowerBound), 0.0f, 1.0f) * 16384;
}


void RadarColorIndex::Params::fromRadarData(RadarData* radarData) {
	maxValue = radarData->stats.maxValue;
	minValue = radarData->stats.minValue;
}

void RadarColorIndex::Result::Delete() {
	if(data != NULL){
		delete[] data;
		data = NULL;
	}
}

/*

RadarColorIndex::Result RadarColorIndex::relativeHue(RadarColorIndex::Params params, Result* reuseResult) {
	RadarColorIndex::Result result = {};
	result.lower = params.minValue;
	result.upper = params.maxValue;
	if(reuseResult != NULL && reuseResult->data != NULL){
		result.data = reuseResult->data;
	}
	if(result.data == NULL){
		result.data = new float[65536]();
	}
	
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

RadarColorIndex::Result RadarColorIndex::relativeHueAcid(RadarColorIndex::Params params, Result* reuseResult) {
	RadarColorIndex::Result result = {};
	result.lower = params.minValue;
	result.upper = params.maxValue;
	if(reuseResult != NULL && reuseResult->data != NULL){
		result.data = reuseResult->data;
	}
	if(result.data == NULL){
		result.data = new float[65536]();
	}
	
	
	for (int i = 0; i < 16384; i++) {
		float value = (i / 16383.0f);
		float hue = 1 - fmod(value + SystemAPI::CurrentTime(), 1.0);
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
	result.data[3] = 0;
	return result;
};
RadarColorIndex::Result RadarColorIndex::reflectivityColors(RadarColorIndex::Params params, Result* reuseResult) {
	RadarColorIndex::Result result = {};
	float l = -20;
	result.lower = l;
	float u = 80;
	result.upper = u;
	if(reuseResult != NULL && reuseResult->data != NULL){
		result.data = reuseResult->data;
	}
	if(result.data == NULL){
		result.data = new float[65536]();
	}
	
	//gray
	colorRangeHSL(result.data, valueToIndex(l,u,-20), valueToIndex(l,u,15),  0,0,0.1,  0,0,0.4);
	//blue
	colorRangeHSL(result.data, valueToIndex(l,u,15), valueToIndex(l,u,30),  0.5,0.6,0.5,  0.66,1,0.5);
	//green
	colorRangeHSL(result.data, valueToIndex(l,u,30), valueToIndex(l,u,40),  0.4,1,0.5,  0.33,1,0.5);
	//yellow
	colorRangeHSL(result.data, valueToIndex(l,u,40), valueToIndex(l,u,50),  0.25,1,0.5,  0.166,1,0.5);
	//red
	colorRangeHSL(result.data, valueToIndex(l,u,50), valueToIndex(l,u,60),  0,1,0.5,  0,1,0.5);
	//purple
	colorRangeHSL(result.data, valueToIndex(l,u,60), valueToIndex(l,u,70),  0.92,1,0.5,  0.83,1,0.5);
	//white
	colorRangeHSL(result.data, valueToIndex(l,u,70), valueToIndex(l,u,80),  0.83,0.3,0.8,  0.83,0,1);
	
	
	for (int i = 0; i < 16384; i++) {
		float value = (i / 16383.0f);
		if(valueToIndex(l,u,60) <= i && i < valueToIndex(l,u,70)){
			//purple
			value *= 1.5;
		}
		if(valueToIndex(l,u,70) <= i && i < valueToIndex(l,u,80)){
			//white
			value *= 2;
		}
		value *= 2;
		result.data[i * 4 + 3] = value;
	}
	return result;
};

RadarColorIndex::Result RadarColorIndex::velocityColors(Params params, Result* reuseResult) {
	RadarColorIndex::Result result = {};
	float l = -30;
	result.lower = l;
	float u = 30;
	result.upper = u;
	if(reuseResult != NULL && reuseResult->data != NULL){
		result.data = reuseResult->data;
	}
	if(result.data == NULL){
		result.data = new float[65536]();
	}
	
	// green
	colorRangeHSL(result.data, valueToIndex(-100,100,-100), valueToIndex(-100,100,0), 0.0,1,0.5, 0.0,0.2,0.5);
	// red
	colorRangeHSL(result.data, valueToIndex(-100,100,0), valueToIndex(-100,100,100),  0.33,0.2,0.5,  0.33,1,0.5);
	
	for (int i = 0; i < 16384; i++) {
		float value = (abs(i - 8191.5f) / 8191.5f );
		result.data[i * 4 + 3] = value;
	}
	result.data[8191 * 4 + 3] = 0;
	result.data[8192 * 4 + 3] = 0;
	return result;
}

*/

RadarColorIndex::Result RadarColorIndex::GenerateColorIndex(Params params, Result* resultToReuse)
{
	return BasicSetup(0, 1, resultToReuse);
}

void RadarColorIndex::ModifyOpacity(float opacityMultiplier, float cutoff, Result* existingResult)
{
	int max = std::min((int)(cutoff * 16384), (int)16384);
	for (int i = 0; i < max; i++) {
		existingResult->data[i * 4 + 3] = 0;
	}
	if(opacityMultiplier != 1.0f){
		for (int i = max; i < 16384; i++) {
			existingResult->data[i * 4 + 3] = existingResult->data[i * 4 + 3] * opacityMultiplier;
		}
	}
}

RadarColorIndex* RadarColorIndex::GetDefaultColorIndexForData(RadarData* radarData) {
	switch (radarData->stats.volumeType)
	{
		case RadarData::VOLUME_REFLECTIVITY:
			return &RadarColorIndexReflectivity::defaultInstance;
			break;
			
		case RadarData::VOLUME_VELOCITY:
		case RadarData::VOLUME_VELOCITY_ANTIALIASED:
			return &RadarColorIndexVelocity::defaultInstance;
			break;
	
		default:
			return &RadarColorIndexRelativeHue::defaultInstance;
			break;
	}
}

RadarColorIndex::Result RadarColorIndex::BasicSetup(float lowerBound, float upperBound, Result* resultToReuse) {
	RadarColorIndex::Result result = {};
	result.lower = lowerBound;
	result.upper = upperBound;
	if(resultToReuse != NULL && resultToReuse->data != NULL){
		result.data = resultToReuse->data;
	}
	if(result.data == NULL){
		result.data = new float[65536]();
	}
	return result;
}

RadarColorIndexReflectivity RadarColorIndexReflectivity::defaultInstance = {};
RadarColorIndex::Result RadarColorIndexReflectivity::GenerateColorIndex(Params params, Result* resultToReuse) {
	float l = -20;
	float u = 80;
	RadarColorIndex::Result result = BasicSetup(l, u, resultToReuse);
	
	//gray
	colorRangeHSL(result.data, valueToIndex(l,u,-20), valueToIndex(l,u,15),  0,0,0.1,  0,0,0.4);
	//blue
	colorRangeHSL(result.data, valueToIndex(l,u,15), valueToIndex(l,u,30),  0.5,0.6,0.5,  0.66,1,0.5);
	//green
	colorRangeHSL(result.data, valueToIndex(l,u,30), valueToIndex(l,u,40),  0.4,1,0.5,  0.33,1,0.5);
	//yellow
	colorRangeHSL(result.data, valueToIndex(l,u,40), valueToIndex(l,u,50),  0.25,1,0.5,  0.166,1,0.5);
	//red
	colorRangeHSL(result.data, valueToIndex(l,u,50), valueToIndex(l,u,60),  0,1,0.5,  0,1,0.5);
	//purple
	colorRangeHSL(result.data, valueToIndex(l,u,60), valueToIndex(l,u,70),  0.92,1,0.5,  0.83,1,0.5);
	//white
	colorRangeHSL(result.data, valueToIndex(l,u,70), valueToIndex(l,u,80),  0.83,0.3,0.8,  0.83,0,1);
	
	
	for (int i = 0; i < 16384; i++) {
		float value = (i / 16383.0f);
		if(valueToIndex(l,u,60) <= i && i < valueToIndex(l,u,70)){
			//purple
			value *= 1.5;
		}
		if(valueToIndex(l,u,70) <= i && i < valueToIndex(l,u,80)){
			//white
			value *= 2;
		}
		value *= 2;
		result.data[i * 4 + 3] = value;
	}
	return result;
}

RadarColorIndexVelocity RadarColorIndexVelocity::defaultInstance = {};
RadarColorIndex::Result RadarColorIndexVelocity::GenerateColorIndex(Params params, Result* resultToReuse) {
	float l = -30;
	float u = 30;
	RadarColorIndex::Result result = BasicSetup(l, u, resultToReuse);
	
	// green
	colorRangeHSL(result.data, valueToIndex(-100,100,-100), valueToIndex(-100,100,0), 0.0,1,0.5, 0.0,0.2,0.5);
	// red
	colorRangeHSL(result.data, valueToIndex(-100,100,0), valueToIndex(-100,100,100),  0.33,0.2,0.5,  0.33,1,0.5);
	
	for (int i = 0; i < 16384; i++) {
		float value = (abs(i - 8191.5f) / 8191.5f );
		result.data[i * 4 + 3] = value;
	}
	result.data[8191 * 4 + 3] = 0;
	result.data[8192 * 4 + 3] = 0;
	return result;
}

RadarColorIndexRelativeHue RadarColorIndexRelativeHue::defaultInstance = {};
RadarColorIndex::Result RadarColorIndexRelativeHue::GenerateColorIndex(Params params, Result* resultToReuse) {
	RadarColorIndex::Result result = BasicSetup(params.minValue, params.maxValue, resultToReuse);
	
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
}

RadarColorIndexRelativeHueAcid RadarColorIndexRelativeHueAcid::defaultInstance = {};
RadarColorIndex::Result RadarColorIndexRelativeHueAcid::GenerateColorIndex(Params params, Result* resultToReuse) {
	RadarColorIndex::Result result = BasicSetup(params.minValue, params.maxValue, resultToReuse);
	
	double currentTime = SystemAPI::CurrentTime();
	for (int i = 0; i < 16384; i++) {
		float value = (i / 16383.0f);
		float hue = 1 - fmod(value + currentTime, 1.0);
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
	result.data[3] = 0;
	return result;
}






