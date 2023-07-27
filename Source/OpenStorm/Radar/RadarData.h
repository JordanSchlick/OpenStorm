#pragma once

//#include "CoreMinimal.h"

#ifdef __INTELLISENSE__
// visual studio has a fucking meltdown if it includes c++ standard headers
#else

#endif
//#include "CoreMinimal.h"
#include <stdint.h>
#include <stddef.h>
#include <math.h>

// holds the data for a volume of radar
class RadarData {
public:
	enum VolumeType {
		// can be used to indicate there is no volume, a RadarData object should never have this type and instead have VOLUME_UNKNOWN
		VOLUME_NONE = -1,
		
		// used when the type of a volume is not known
		VOLUME_UNKNOWN = 0,
		
		// raw volume types
		
		VOLUME_REFLECTIVITY = 1,
		VOLUME_VELOCITY = 2,
		VOLUME_SPECTRUM_WIDTH = 3,
		VOLUME_CORELATION_COEFFICIENT = 4,
		VOLUME_DIFFERENTIAL_REFLECTIVITY = 5,
		VOLUME_DIFFERENTIAL_PHASE_SHIFT = 6,
		
		// computed volume types
		
		VOLUME_VELOCITY_DEALIASED = 102,
		VOLUME_ROTATION = 103,
		VOLUME_STORM_RELATIVE_VELOCITY = 104,
	};
	
	// log extra information
	static bool verbose;
	
	// buffer of volume values
	// may be null if not loaded or compressed
	// each sweep has a padding ray at the beginning and end of it that was copied from the other side to make interpolation more efficient
	// dim 0: sweep  dim 1: rotation  dim 2: radius
	float* buffer = NULL;
	
	
	// counts can be set when there is no buffer loaded
	// they will be set automaticaly if they are zero

	// number of data points in the radius dimension
	// do not change while buffer is allocated
	int radiusBufferCount = 0;
	// number of data points in theta rotations
	// there are 2 more than this count in the buffer that serve as padding for interpolation
	// do not change while buffer is allocated
	int thetaBufferCount = 0;
	// number of full sweep disks in the buffer
	// do not change while buffer is allocated
	int sweepBufferCount = 0;

	// sizes of sections of the buffer
	// size of one ray
	int thetaBufferSize = 0;
	// size of an entire sweep
	int sweepBufferSize = 0;
	// size of the whole volume
	int fullBufferSize = 0;
	// amount of buffer currently used. indexes above this should be blank.
	// the compressed buffer will also decompress to this size
	int usedBufferSize = 0;
	
	// if the data should be compressed as it is read
	bool doCompress = false;
	// A buffer where empty space is compressed
	float* bufferCompressed = NULL;
	// number of elements in the compressed buffer
	int compressedBufferSize = 0;

	struct Stats {
		// blank distance in pixels between origin and most center pixels of the buffer
		float innerDistance = 0;
		// minumum value in the data
		float minValue = 0;
		// maximum value in the data
		float maxValue = 0;
		// length of a pixel along the radius in meters
		float pixelSize = 250;
		// date time as seconds since epoch when the volume scan started
		double beginTime = 0;
		// date time as seconds since epoch when the volume scan ended
		double endTime = 0;
		// value for when there is no data, the default is -infinity
		float noDataValue = -INFINITY;
		// ( TODO: Implement NaN support in the shader) value for invalid data such as range folding, the defaults is NaN
		// float invalidValue = NAN;
		
		// bounds in pixels including the inner distance, defined by a sphere with a radius of boundRadius and the top and bottom cut off
		float boundRadius = 0;
		float boundUpper = 0;
		float boundLower = 0;
		
		// location of the radar in the world
		double latitude = 0; 
		double longitude = 0; 
		double altitude = 0;
		
		// the type of radar volume
		VolumeType volumeType = VOLUME_UNKNOWN;
	};
	
	// hold data for a texture
	class TextureBuffer {
	public:
		float* data = NULL;
		int byteSize = 0;
		bool doDelete = true;
		void Delete();
	};

	
	// information about the volume
	Stats stats = {};

	// info about a single sweep
	class SweepInfo {
	public:
		// elevation from flat in degrees
		float elevationAngle = 0;
		// sweep index of sweep, -1 if no sweep exists
		int id = -1;
		// number of not interpolated rays in sweep
		int actualRayCount = 0;
	};

	// array of info about sweeps
	SweepInfo* sweepInfo = NULL;

	// info about ray, positions are relative and can be safely used as offsets in the buffer without modulo because they are already normalized to never point outside of the same sweep
	class RayInfo {
	public:
		// if the ray was interpolated from actual rays
		bool interpolated = true;
		// relative position of closest actual ray in sweep, does not need modulo before using as offset
		int closestTheta = 0;
		// relative position of previous actual ray in sweep,  does not need modulo before using as offset
		int previousTheta = -1;
		// relative position of next actual ray in sweep, does not need modulo before using as offset
		int nextTheta = 1;
		// angle of the actual ray, not set for interpolated rays
		float actualAngle = 0;
		
		int sweep = 0;
		int theta = 0;
	};
	
	
	// array of info about rays
	RayInfo* rayInfo = NULL;
	
	// parse rsl nexrad data 
	static void* ReadNexradData(const char* filename);
	// free rsl nexrad data
	static void FreeNexradData(void* nexradData);
	// load rsl nexrad volume into this radar data
	bool LoadNexradVolume(void* nexradData, VolumeType volumeType);
	
	// copies data from another and decompresses it if needed
	void CopyFrom(RadarData* data);

	// returns a float array of the sweeps for elevations to be used with the shader
	// buffer must be freed with delete[]
	// the angle index texture maps the angle from the ground to a sweep
	// pixel 65536 is strait up, pixel 32768 is parallel with the ground, pixel 0 is strait down
	// value -1.0 is no sweep, value 0.0-255.0 specify the index of the sweep, value 0.0 specifies first sweep, values inbetween integers will interpolate sweeps
	RadarData::TextureBuffer CreateAngleIndexBuffer();
	
	// compress the buffer and deallocate the full buffer
	void Compress();
	
	// decompress into the main buffer but leaves compressed buffer unfreed
	void Decompress();
	
	// returns true if it is compressed and the main buffer is unallocated
	bool IsCompressed();
	
	// interpolate real rays to fill buffer
	void Interpolate();
	
	// interpolate real rays to fill buffer for a single sweep
	void InterpolateSweep(int sweepIndex, float* sweepBuffer);
	
	// gets the memory usage of this object in bytes
	int MemoryUsage();
	
	//frees all buffers
	void Deallocate();
	
	~RadarData();
};




