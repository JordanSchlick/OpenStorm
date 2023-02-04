#include "RotationProduct.h"
#include <algorithm>

RadarProductRotation::RadarProductRotation(){
	volumeType = RadarData::VOLUME_ROTATION;
	productType = PRODUCT_DERIVED_VOLUME;
	name = "Rotation";
	shortName = "ROT";
	dependencies = {RadarData::VOLUME_VELOCITY_DEALIASED,  RadarData::VOLUME_SPECTRUM_WIDTH};
	// this currently does not work very well so it is hidden by default
	development = true;
}


RadarData* RadarProductRotation::deriveVolume(std::map<RadarData::VolumeType, RadarData *> inputProducts){
	RadarData* radarData = new RadarData();
	radarData->CopyFrom(inputProducts[RadarData::VOLUME_VELOCITY_DEALIASED]);
	std::fill(radarData->buffer, radarData->buffer + radarData->usedBufferSize, 0.0f);
	float maxValue = 0;
	RadarData* vol = radarData;
	RadarData* src = inputProducts[RadarData::VOLUME_VELOCITY_DEALIASED];
	src->Decompress();
	RadarData* srcSW = inputProducts[RadarData::VOLUME_SPECTRUM_WIDTH];
	srcSW->Decompress();
	// when the spectrum width is below this the outout will be decreased
	float spectrumWidthThreshold = 20;
	for(int sweep = 0; sweep < vol->sweepBufferCount; sweep++){
		for(int theta = 0; theta < vol->thetaBufferCount; theta++){
			RadarData::RayInfo* rayInfo = &vol->rayInfo[sweep * (vol->thetaBufferCount + 2) + theta + 1];
			if(!rayInfo->interpolated){
				int location = vol->thetaBufferSize * (theta + 1) + vol->sweepBufferSize * sweep;
				float* ray = vol->buffer + location;
				float* raySrc = src->buffer + location;
				float* raySrcSW = srcSW->buffer + location;
				float* raySrcPrev = raySrc + rayInfo->previousTheta * vol->thetaBufferSize;
				float* raySrcNext = raySrc + rayInfo->nextTheta * vol->thetaBufferSize;
				for(int radius = 2; radius < vol->radiusBufferCount - 1; radius++){
					float minVal = 1000;
					float maxVal = -1000;
					float val = 0;
					// val = raySrcPrev[radius - 1];
					// if(val != 0){
					// 	minVal = std::min(minVal, val);
					// 	maxVal = std::max(maxVal, val);
					// }
					val = raySrcPrev[radius];
					if(val != 0){
						minVal = std::min(minVal, val);
						maxVal = std::max(maxVal, val);
					}
					// val = raySrcPrev[radius + 1];
					// if(val != 0){
					// 	minVal = std::min(minVal, val);
					// 	maxVal = std::max(maxVal, val);
					// }
					// val = raySrc[radius - 1];
					// if(val != 0){
					// 	minVal = std::min(minVal, val);
					// 	maxVal = std::max(maxVal, val);
					// }
					val = raySrc[radius];
					if(val != 0){
						minVal = std::min(minVal, val);
						maxVal = std::max(maxVal, val);
					}
					// val = raySrc[radius + 1];
					// if(val != 0){
					// 	minVal = std::min(minVal, val);
					// 	maxVal = std::max(maxVal, val);
					// }
					// val = raySrcNext[radius - 1];
					// if(val != 0){
					// 	minVal = std::min(minVal, val);
					// 	maxVal = std::max(maxVal, val);
					// }
					val = raySrcNext[radius];
					if(val != 0){
						minVal = std::min(minVal, val);
						maxVal = std::max(maxVal, val);
					}
					val = raySrcNext[radius + 1];
					// if(val != 0){
					// 	minVal = std::min(minVal, val);
					// 	maxVal = std::max(maxVal, val);
					// }
					val = std::max(maxVal - minVal, 0.0f) * std::min(raySrcSW[radius] , spectrumWidthThreshold) / spectrumWidthThreshold;
					ray[radius] = val;
					maxValue = std::max(maxValue, val);
				}
			}
		}
	}
	radarData->stats.minValue = 0;
	radarData->stats.maxValue = std::max(maxValue, 0.0001f);
	radarData->stats.volumeType = volumeType;
	radarData->Interpolate();
	return radarData;
}