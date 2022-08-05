#pragma once

//#include "CoreMinimal.h"

#ifdef __INTELLISENSE__
// visual studio has a fucking meltdown if it includes c++ standard headers
#else

#endif
//#include "CoreMinimal.h"
#include <stdint.h>

class RadarData {
public:
	// buffer of volume values
	// dim 0: sweep  dim 1: rotation  dim 2: radius
	float* buffer = NULL;

	// counts can be set when there is no buffer loaded
	// they will be set automaticaly if they are zero

	// number of data points in the radius dimension
	// do not change while buffer is allocated
	int radiusBufferCount = 0;
	// number of data points in theta rotations
	// do not change while buffer is allocated
	int thetaBufferCount = 0;
	// number of full sweep disks in the buffer
	// do not change while buffer is allocated
	int sweepBufferCount = 0;

	

	struct SweepInfo {
		float elevation = 0;
		int id = -1;
		int actualRayCount = 0;
	};

	// array of info about sweeps
	SweepInfo* sweepInfo = NULL;

	// blank distance in pixels between origin and most center pixels of the buffer
	float innerDistance = 0;

	float minValue = 0;
	float maxValue = 0;

	void ReadNexrad(const char* filename);

	struct TextureBuffer {
		float* data;
		int byteSize;
	};

	// returns a uint8 array of the volume each pixel is 4 bytes wide. theta is padded by one pixel on each side
	// buffer must be freed with delete[]
	RadarData::TextureBuffer CreateTextureBufferReflectivity();

	// returns a float array of the sweeps for elevations to be used with the shader
	// buffer must be freed with delete[]
	// the angle index texture maps the angle from the ground to a sweep
	// pixel 65536 is strait up, pixel 32768 is parallel with the ground, pixel 0 is strait down
	// value 0 is no sweep, value 1.0-255.0 specify the index of the sweep, value 1.0 specifies first sweep, values inbetween intagers will interpolate sweeps
	RadarData::TextureBuffer CreateAngleIndexBuffer();
	
	//frees all buffers
	void Deallocate();
	
	~RadarData();
};




