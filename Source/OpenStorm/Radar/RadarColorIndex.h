#pragma once


#include "RadarData.h"

class RadarColorIndexParams {
public:
	float minValue = 0;
	float maxValue = 1;
	void fromRadarData(RadarData* radarData);
};


class RadarColorIndexResult {
public:
	// buffer of values to colors and alphas
	// must use delete[] after use
	float* data;
	// constant size of buffer in bytes
	int byteSize = 262144;
	// lowest value mapped by the buffer
	float lower = 0;
	// highest value mapped by the buffer
	float upper = 0;
};

// Produces buffers for 128 x 128 float4 textures which comes out to 65536 floats or 262144 bytes or 16384 pixels
class RadarColorIndex {
public:
	static RadarColorIndexResult relativeHue(RadarColorIndexParams params);
	static RadarColorIndexResult relativeHueAcid(RadarColorIndexParams params);
};