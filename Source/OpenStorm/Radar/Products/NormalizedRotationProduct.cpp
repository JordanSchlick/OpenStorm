#include "NormalizedRotationProduct.h"
#include <algorithm>
#include <cmath>

#define PIF 3.14159265358979323846f

RadarProductNormalizedRotation::RadarProductNormalizedRotation(){
	volumeType = RadarData::VOLUME_ROTATION;
	productType = PRODUCT_DERIVED_VOLUME;
	name = "Normalized Rotation";
	shortName = "NROT";
	dependencies = {RadarData::VOLUME_VELOCITY_DEALIASED};
}

// distance in looped space
inline float moduloDistance(float distance, float n){
	return fmin(fabs(distance), n - fabs(distance));
}

RadarData* RadarProductNormalizedRotation::deriveVolume(std::map<RadarData::VolumeType, RadarData *> inputProducts){
	RadarData* radarData = new RadarData();
	radarData->CopyFrom(inputProducts[RadarData::VOLUME_VELOCITY_DEALIASED], true);
	radarData->buffer = new float[radarData->fullBufferSize];
	std::fill(radarData->buffer, radarData->buffer + radarData->fullBufferSize, 0.0f);
	
	float maxValue = 0;
	float minValue = 0;
	RadarData* vol = radarData;
	RadarData* src = inputProducts[RadarData::VOLUME_VELOCITY_DEALIASED];
	src->Decompress();
	for(int sweep = 0; sweep < vol->sweepBufferCount; sweep++){
		for(int theta = 0; theta < vol->thetaBufferCount; theta++){
			RadarData::RayInfo* rayInfo = &vol->rayInfo[sweep * (vol->thetaBufferCount + 2) + theta + 1];
			if(!rayInfo->interpolated){
				RadarData::RayInfo* rayInfoPrev = &vol->rayInfo[sweep * (vol->thetaBufferCount + 2) + theta + 1 + rayInfo->previousTheta];
				RadarData::RayInfo* rayInfoNext = &vol->rayInfo[sweep * (vol->thetaBufferCount + 2) + theta + 1 + rayInfo->nextTheta];
				int location = vol->thetaBufferSize * (theta + 1) + vol->sweepBufferSize * sweep;
				float* ray = vol->buffer + location;
				float* raySrc = src->buffer + location;
				float* raySrcPrev = raySrc + rayInfo->previousTheta * vol->thetaBufferSize;
				float* raySrcNext = raySrc + rayInfo->nextTheta * vol->thetaBufferSize;
				float angleToPrev = moduloDistance(rayInfo->actualAngle - rayInfoPrev->actualAngle, 360);
				float angleToNext = moduloDistance(rayInfoNext->actualAngle - rayInfo->actualAngle, 360);
				float distanceFactorPrev = std::tan(angleToPrev * PIF / 180.0f);
				float distanceFactorNext = std::tan(angleToNext * PIF / 180.0f);
				
				
				for(int radius = 0; radius < vol->radiusBufferCount - 1; radius++){
					float distance = (src->stats.innerDistance + radius) * src->stats.pixelSize;
					// avoid significantly boosting values close to the radar
					distance = std::max(distance, 50000.0f);
					float value = raySrc[radius];
					if(value == 0 || std::isnan(value)){
						continue;
					}
					float valuePrev = raySrcPrev[radius];
					float valueNext = raySrcNext[radius];
					float rotPrev = (value - valuePrev) / (distance * distanceFactorPrev);
					float rotNext = (valueNext - value) / (distance * distanceFactorNext);
					bool rotPrevNaN = std::isnan(rotPrev) || valuePrev == 0;
					bool rotNextNaN = std::isnan(rotNext) || valueNext == 0;
					float val = 0;
					if(rotPrevNaN && rotNextNaN){
						continue;
					}else if(rotPrevNaN){
						val = rotNext;
					}else if(rotNextNaN){
						val = rotPrev;
					}else if(fabs(rotPrev) > fabs(rotNext)){
						val = rotPrev;
					}else{
						val = rotNext;
					}
					val *= 500;
					ray[radius] = val;
					maxValue = std::max(maxValue, val);
					minValue = std::min(minValue, val);
				}
			}
		}
	}
	radarData->stats.minValue = minValue;
	radarData->stats.maxValue = std::max(maxValue, 0.0001f);
	if(inputProducts[RadarData::VOLUME_VELOCITY_DEALIASED]->verbose || true){
		printf("Normalized Rotation: Min %f, Max %f\n", minValue, maxValue);
	}
	radarData->stats.volumeType = volumeType;
	radarData->Interpolate();
	return radarData;
}