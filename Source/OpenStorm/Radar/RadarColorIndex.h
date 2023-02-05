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
		// must use Delete when the result is no longer needed
		float* data = NULL;
		// constant size of buffer in bytes
		int byteSize = 262144;
		int floatSize = 65536;
		// lowest value mapped by the buffer
		float lower = 0;
		// highest value mapped by the buffer
		float upper = 0;
		// free the result
		void Delete();
	};
	
	// static Result relativeHue(Params params, Result* reuseResult);
	// static Result relativeHueAcid(Params params, Result* reuseResult);
	// static Result reflectivityColors(Params params, Result* reuseResult);
	// static Result velocityColors(Params params, Result* reuseResult);
	
	// Generates a valueIndex to map data to colors and opacity
	virtual Result GenerateColorIndex(Params params, Result* resultToReuse) = 0;
	
	// modify the opacity with a multiplier and a relative cutoff
	virtual void ModifyOpacity(float opacityMultiplier, float cutoff, Result* existingResult);
	
	// gets the default color index based on the type of the volume
	static RadarColorIndex* GetDefaultColorIndexForData(RadarData* radarData);
	
protected:
	// helper function to initialize result
	Result BasicSetup(float lowerBound, float upperBound, Result* existingResult);
	// modify opacity for indexes where the lowest is in the center like velocity 
	//void ModifyOpacitySymmetric(float opacityMultiplier, float cutoff, Result* existingResult);
};

class RadarColorIndexReflectivity : public RadarColorIndex {
public:
	virtual Result GenerateColorIndex(Params params, Result* resultToReuse) override;
	static RadarColorIndexReflectivity defaultInstance;
};

class RadarColorIndexVelocity : public RadarColorIndex {
public:
	virtual Result GenerateColorIndex(Params params, Result* resultToReuse) override;
	static RadarColorIndexVelocity defaultInstance;
};

class RadarColorIndexRelativeHue : public RadarColorIndex {
public:
	virtual Result GenerateColorIndex(Params params, Result* resultToReuse) override;
	static RadarColorIndexRelativeHue defaultInstance;
};

class RadarColorIndexRelativeHueAcid : public RadarColorIndex {
public:
	virtual Result GenerateColorIndex(Params params, Result* resultToReuse) override;
	static RadarColorIndexRelativeHueAcid defaultInstance;
};