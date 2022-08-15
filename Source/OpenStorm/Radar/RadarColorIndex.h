#pragma once


#include "RadarData.h"



// Produces buffers for 128 x 128 float4 textures which comes out to 65536 floats or 262144 bytes or 16384 pixels
// Results have a Delete method to free them. Some functions can reuse a previously allocated result.
class RadarColorIndex {
	
public:
	class Params {
	public:
		float minValue = 0;
		float maxValue = 1;
		// loads values from a RadarData object
		void fromRadarData(RadarData* radarData);
	};

	class Result {
	public:
		// buffer of values to colors and alphas
		// must use delete[] after use
		float* data = NULL;
		// constant size of buffer in bytes
		int byteSize = 262144;
		// lowest value mapped by the buffer
		float lower = 0;
		// highest value mapped by the buffer
		float upper = 0;
		
		void Delete();
	};
	
	static Result relativeHue(Params params);
	static Result relativeHueAcid(Params params);
	static Result reflectivityColors(Params params, Result* reuseResult);
};