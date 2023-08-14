
#include "RadarData.h"
#include "SparseCompression.h"
#include "NexradSites/NexradSites.h"
#include "./Deps/rsl/rsl.h"

#include "Nexrad.h"


//#include "CoreMinimal.h"
//#include "Engine/Texture2D.h"

#ifdef __INTELLISENSE__
// visual studio may have a fucking meltdown if it includes c++ standard headers move them into the else block here if it happens
#else

#endif
#include <map>
#include <algorithm>
#include <cmath>

#define PIF 3.14159265358979323846f

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

// compares if argument larger is larger than argument smaller within a looped space
inline bool moduloCompare(int smaller, int larger, int n) {
	int offset = n/2 - smaller;
	return modulo(smaller + offset,n) < modulo(larger + offset,n);
}

// compares if smaller is closer to zero than larger within a looped space
inline bool moduloSmallerAbs(int smaller, int larger, int n) {
	return std::min(modulo(smaller,n), modulo(-smaller,n)) < std::min(modulo(larger,n), modulo(-larger,n));
}

// returns first - second where the result is at most n/2
inline int moduloSignedSubtract(int first, int second, int n){
	int result = modulo(first - second, n);
	if(result > n/2){
		result -= n;
	}
	return result;
}

bool RadarData::verbose = false;

void* RadarData::ReadNexradData(const char* filename) {
	NexradRadarReader* reader = new NexradRadarReader();
	reader->LoadFile(std::string(filename));
	return reader;
}

void RadarData::FreeNexradData(void* nexradData) {
	if (nexradData) {
		delete (NexradRadarReader*)nexradData;
	}
}

bool RadarData::LoadNexradVolume(void* nexradData, VolumeType volumeType) {
	if (nexradData) {
		return ((NexradRadarReader*)nexradData)->LoadVolume(this, volumeType);
	}else{
		return false;
	}
}

void RadarData::CopyFrom(RadarData* data) {
	if(buffer == NULL || thetaBufferCount != data->thetaBufferCount || sweepBufferCount != data->sweepBufferCount || radiusBufferCount != data->radiusBufferCount){
		//if (radiusBufferCount == 0) {
		radiusBufferCount = data->radiusBufferCount;
		//}
		//if (sweepBufferCount == 0) {
		if(sweepBufferCount != data->sweepBufferCount){
			if(sweepInfo != NULL){
				delete sweepInfo;
			}
			sweepInfo = new SweepInfo[data->sweepBufferCount]();
		}
		sweepBufferCount = data->sweepBufferCount;
		//}
		//if (thetaBufferCount == 0) {
		if(thetaBufferCount != data->thetaBufferCount || sweepBufferCount != data->sweepBufferCount){
			if(rayInfo != NULL){
				delete rayInfo;
			}
			rayInfo = new RayInfo[data->sweepBufferCount * (data->thetaBufferCount + 2)]();
		}
		thetaBufferCount = data->thetaBufferCount;
		//}
		
		thetaBufferSize = radiusBufferCount;
		sweepBufferSize = (thetaBufferCount + 2) * thetaBufferSize;
		fullBufferSize = sweepBufferCount * sweepBufferSize;
		
		
		// allocate buffer
		if(buffer != NULL){
			delete buffer;
		}
		buffer = new float[fullBufferSize];
		//std::fill(buffer, buffer+fullBufferSize, data->stats.noDataValue);
		if(data->buffer != buffer){
			// only set buffer used size when not copying from self
			usedBufferSize = 0;
		}
	}
	// copy data
	if(data->buffer != NULL && data->buffer != buffer){
		memcpy(buffer, data->buffer, std::min(fullBufferSize, data->usedBufferSize) * sizeof(float));
	}else if(data->bufferCompressed != NULL){
		SparseCompress::decompressToBuffer(buffer, data->bufferCompressed, fullBufferSize);
	}
	// take used buffer size into account
	if(data->usedBufferSize < usedBufferSize){
		// fill newly unused space
		std::fill(buffer + data->usedBufferSize, buffer + usedBufferSize, data->stats.noDataValue);
	}
	usedBufferSize = data->usedBufferSize;
	stats = data->stats;
	if(data->sweepInfo && data->sweepInfo != sweepInfo){
		memcpy(sweepInfo, data->sweepInfo, std::min(sweepBufferCount, data->sweepBufferCount) * sizeof(SweepInfo));
	}
	if(data->rayInfo && data->rayInfo != rayInfo){
		memcpy(rayInfo, data->rayInfo, std::min(sweepBufferCount * (thetaBufferCount + 2), data->sweepBufferCount * (data->thetaBufferCount + 2)) * sizeof(RayInfo));
	}
}





RadarData::TextureBuffer RadarData::CreateAngleIndexBuffer() {
	float* textureBuffer = new float[65536];
	std::fill(textureBuffer, textureBuffer + 65536, -1.0f);
	if(sweepInfo != NULL){
		//int divider = (stats.maxValue - stats.minValue) / 256 + 1;
		int firstIndex = sweepBufferCount;
		int lastIndex = -1;
		for (int sweepIndex = 0; sweepIndex < sweepBufferCount; sweepIndex++) {
			RadarData::SweepInfo &info = sweepInfo[sweepIndex];
			if(info.id != -1){
				firstIndex = std::min(firstIndex, sweepIndex);
				lastIndex = std::max(lastIndex, sweepIndex);
			}
			
		}
		for (int sweepIndex = firstIndex + 1; sweepIndex < sweepBufferCount; sweepIndex++) {
			RadarData::SweepInfo &info1 = sweepInfo[sweepIndex - 1];
			RadarData::SweepInfo &info2 = sweepInfo[sweepIndex];
			if(info2.id == -1){
				break;
			}
			int startLocation = info1.elevationAngle * 32768 / 90 + 32768;
			int endLocation = info2.elevationAngle * 32768 / 90 + 32768;
			int delta = endLocation - startLocation;
			float deltaF = delta;
			//fprintf(stderr, "delta: %i %f\n", delta, info1.elevation);
			float firstSweepIndex = sweepIndex - 1.0f;
			if (startLocation < 0 || endLocation >= 65536) {
				fprintf(stderr, "invalid elevation found while calculating angle index\n");
			} else {
				for (int i = 0; i <= delta; i++) {
					float subLocation = (float)i / deltaF;
					textureBuffer[startLocation + i] = firstSweepIndex + subLocation;
				}
			}
		}
		if(firstIndex == lastIndex){
			// display the single sweep with a width of 0.4 degrees
			RadarData::SweepInfo &info = sweepInfo[firstIndex];
			int startLocation = (info.elevationAngle - 0.2) * 32768 / 90 + 32768;
			int endLocation = (info.elevationAngle + 0.2) * 32768 / 90 + 32768;
			int delta = endLocation - startLocation;
			float firstSweepIndex = firstIndex;
			if (startLocation < 0 || endLocation >= 65536) {
				fprintf(stderr, "invalid elevation found while calculating angle index\n");
			} else {
				for (int i = 0; i <= delta; i++) {
					textureBuffer[startLocation + i] = firstSweepIndex;
				}
			}
		}
	}

	/*for (int i = 0; i < 65536; i++) {
		if (i >= 32768) {
			float sweep = ((i - 32768) * 32) / 32768.0;
			//if (!((int)sweep - sweep < 0.2 && sweep - (int)sweep < 0.2)) {
			//	sweep = 0;
			//}
			textureBuffer[i] = sweep;
		} else {
			textureBuffer[i] = 0;
		}
		//rawAngleIndexImageData[i] = 0;
	}*/
	RadarData::TextureBuffer returnValue;
	returnValue.data = textureBuffer;
	returnValue.byteSize = 65536 * 4;
	return returnValue;
}

void RadarData::Compress() {
	//return;
	/*if(bufferCompressed != NULL && buffer != NULL){
		delete bufferCompressed;
		bufferCompressed = NULL;
		compressedBufferSize = 0;
	}*/
	if(bufferCompressed == NULL && buffer != NULL){
		SparseCompress::CompressorState compressorState = {};
		compressorState.emptyValue = stats.noDataValue;
		compressorState.preCompressedSize = fullBufferSize / 10;
		SparseCompress::compressStart(&compressorState);
		SparseCompress::compressValues(&compressorState, buffer, usedBufferSize);
		bufferCompressed = SparseCompress::compressEnd(&compressorState);
		compressedBufferSize = compressorState.sizeAllocated;
	}
	if(buffer != NULL){
		delete buffer;
		buffer = NULL;
	}
}

void RadarData::Decompress() {
	if(bufferCompressed != NULL && buffer == NULL){
		CopyFrom(this);
	}
}

bool RadarData::IsCompressed() {
	return buffer == NULL && bufferCompressed != NULL;
}


void RadarData::Interpolate(){
	if(!buffer){
		fprintf(stderr, "Error: cannot interpolate without main buffer allocated\n");
		return;
	}
	for(int sweep = 0; sweep < sweepBufferCount; sweep++){
		InterpolateSweep(sweep, buffer + (sweep * sweepBufferSize));
	}
}

void RadarData::InterpolateSweep(int sweepIndex, float *sweepBuffer){
	// calculate RayInfo
	{
		// start of rays for this sweep
		int rayInfoOffset = (thetaBufferCount + 2) * sweepIndex + 1;
		int firstRay = -1;
		int lastRay = -1;
		RayInfo* lastRayInfo = NULL;
		for(int theta = 0; theta < thetaBufferCount; theta++){
			RayInfo* thisRayInfo = &rayInfo[rayInfoOffset + theta];
			if(!thisRayInfo->interpolated){
				if(firstRay == -1){
					firstRay = theta;
				}else{
					// fill in interpolated rays
					for(int i = lastRay + 1; i < theta; i++){
						RayInfo* interRayInfo = &rayInfo[rayInfoOffset + i];
						interRayInfo->previousTheta = lastRay - i;
						interRayInfo->nextTheta = theta - i;
						interRayInfo->theta = i;
						interRayInfo->sweep = sweepIndex;
						// angle of center interpolated of ray
						float angle = (i + 0.5) * 360.0f / (float)thetaBufferCount;
						if(thisRayInfo->actualAngle - angle < angle - lastRayInfo->actualAngle){
							interRayInfo->closestTheta = theta - i;
						}else{
							interRayInfo->closestTheta = lastRay - i;
						}
					}
					lastRayInfo->nextTheta = theta - lastRay;
					thisRayInfo->previousTheta = lastRay - theta;
					thisRayInfo->sweep = sweepIndex;
					thisRayInfo->theta = theta;
				}
				lastRay = theta;
				lastRayInfo = thisRayInfo;
			}
		}
		if(firstRay == -1){
			// no rays were in the sweep, just link first and last ray
			rayInfo[rayInfoOffset].previousTheta = thetaBufferCount - 1;
			rayInfo[rayInfoOffset + thetaBufferCount - 1].nextTheta = 1 - thetaBufferCount;
		}else{
			RayInfo* firstRayInfo = &rayInfo[rayInfoOffset + firstRay];
			// fill in warped around interpolated rays between last and first
			//fprintf(stderr, "%i %i   ", lastRay + 1, firstRay + thetaBufferCount);
			for(int i = lastRay + 1; i < firstRay + thetaBufferCount; i++){
				int iWrapped = i % thetaBufferCount;
				RayInfo* interRayInfo = &rayInfo[rayInfoOffset + iWrapped];
				interRayInfo->previousTheta = lastRay - iWrapped;
				interRayInfo->nextTheta = firstRay - iWrapped;
				interRayInfo->theta = iWrapped;
				interRayInfo->sweep = sweepIndex;
				// angle of center interpolated of ray
				float angle = (iWrapped + 0.5) * 360.0f / (float)thetaBufferCount;
				//fprintf(stderr, "%i %i %f %f %f   ", sweepIndex, iWrapped, (firstRayInfo->actualAngle + 360), angle, lastRayInfo->actualAngle);
				//if((firstRayInfo->actualAngle + 360) - angle < angle - (lastRayInfo->actualAngle)){
				if(moduloSmallerAbs(firstRayInfo->actualAngle - angle, lastRayInfo->actualAngle - angle, 360.0f)){
					interRayInfo->closestTheta = firstRay - iWrapped;
				}else{
					interRayInfo->closestTheta = lastRay - iWrapped;
				}
				//interRayInfo->closestTheta = -100;
			}
			// connect first and last ray
			lastRayInfo->nextTheta = firstRay - lastRay;
			lastRayInfo->theta = lastRay;
			lastRayInfo->sweep = sweepIndex;
			firstRayInfo->previousTheta = lastRay - firstRay;
			firstRayInfo->theta = firstRay;
			firstRayInfo->sweep = sweepIndex;
		}
		// create RayInfo for padded rays
		// I think these are broken so don't use them
		// They may actually work now but I haven't tested
		rayInfo[rayInfoOffset - 1] = rayInfo[rayInfoOffset + thetaBufferCount - 1];
		rayInfo[rayInfoOffset - 1].interpolated = true;
		rayInfo[rayInfoOffset - 1].closestTheta += thetaBufferCount;
		rayInfo[rayInfoOffset - 1].nextTheta += thetaBufferCount;
		rayInfo[rayInfoOffset - 1].previousTheta += thetaBufferCount;
		rayInfo[rayInfoOffset - 1].theta = -1;
		rayInfo[rayInfoOffset - 1].sweep = sweepIndex;
		rayInfo[rayInfoOffset + thetaBufferCount] = rayInfo[rayInfoOffset];
		rayInfo[rayInfoOffset + thetaBufferCount].interpolated = true;
		rayInfo[rayInfoOffset + thetaBufferCount].closestTheta -= thetaBufferCount;
		rayInfo[rayInfoOffset + thetaBufferCount].nextTheta -= thetaBufferCount;
		rayInfo[rayInfoOffset + thetaBufferCount].previousTheta -= thetaBufferCount;
		rayInfo[rayInfoOffset + thetaBufferCount].theta = thetaBufferCount;
		rayInfo[rayInfoOffset + thetaBufferCount].sweep = sweepIndex;
	}
	
	
	// interpolate ray data
	int rayInfoOffset = (thetaBufferCount + 2) * sweepIndex + 1;
	for (int theta = 0; theta < thetaBufferCount; theta++) {
		RayInfo* info = &rayInfo[rayInfoOffset + theta];
		if (info->interpolated) {
			// fill in blank by interpolating surroundings
			
			int gapDistance = info->nextTheta - info->previousTheta;
			if(gapDistance < 0){
				gapDistance += thetaBufferCount;
			}
			if(gapDistance <= std::max(thetaBufferCount / 50, 2)){
				int previousRayAbs = theta + info->previousTheta;
				int nextRayAbs = theta + info->nextTheta;
				if(nextRayAbs > thetaBufferCount || thetaBufferCount < 0 || previousRayAbs < 0 || previousRayAbs > thetaBufferCount){
					fprintf(stderr, "sgap distance %i %i %i %i %i %i\n", gapDistance, theta, previousRayAbs, nextRayAbs, info->previousTheta, info->nextTheta);
					continue;
				}
				for (int radius = 0; radius < radiusBufferCount; radius++) {
					float previousValue = sweepBuffer[radius + (previousRayAbs + 1) * thetaBufferSize];
					float nextValue = sweepBuffer[radius + (nextRayAbs + 1) * thetaBufferSize];
					for (int thetaTo = previousRayAbs + 1; thetaTo <= previousRayAbs + gapDistance - 1; thetaTo++) {
						float interLocation = (float)(thetaTo - previousRayAbs) / (float)(gapDistance);
						// write interpolated value
						sweepBuffer[radius + (modulo(thetaTo, thetaBufferCount) + 1) * thetaBufferSize] = previousValue * (1.0 - interLocation) + nextValue * interLocation;
					}
				}
				// advance past filled rays
				if(nextRayAbs >= theta){
					theta = nextRayAbs;
				}else{
					break;
				}
				
			}
		}
	}
	
	
	// pad theta with ray from other side to allow correct interpolation in shader at edges
	memcpy(
		sweepBuffer,
		sweepBuffer + (thetaBufferCount * thetaBufferSize),
		thetaBufferSize * sizeof(float));
	memcpy(
		sweepBuffer + (((thetaBufferCount + 1) * thetaBufferSize)),
		sweepBuffer + (thetaBufferSize),
		thetaBufferSize * sizeof(float));
}

int RadarData::MemoryUsage(){
	int usage = sizeof(RadarData);
	if(buffer != NULL){
		usage += sizeof(float) * fullBufferSize;
	}
	usage += sizeof(float) * compressedBufferSize;
	if(sweepInfo != NULL){
		usage += sizeof(SweepInfo) * sweepBufferCount;
	}
	if(rayInfo != NULL){
		usage += sizeof(RayInfo) * sweepBufferCount * (thetaBufferCount + 2);
	}
	return usage;
}

void RadarData::Deallocate(){
	if(buffer != NULL){
		delete[] buffer;
		buffer = NULL;
		usedBufferSize = 0;
	}
	if(bufferCompressed != NULL){
		delete[] bufferCompressed;
		bufferCompressed = NULL;
	}
	if(sweepInfo != NULL){
		delete[] sweepInfo;
		sweepInfo = NULL;
	}
	if(rayInfo != NULL){
		delete[] rayInfo;
		rayInfo = NULL;
	}
}



RadarData::~RadarData()
{
	RadarData::Deallocate();
	if(verbose){
		fprintf(stderr,"freed RadarData\n");
	}
}

void RadarData::TextureBuffer::Delete() {
	if(doDelete && data != NULL){
		delete[] data;
		data = NULL;
	}
}
