#include "VelocityRelativeProduct.h"

#include <cmath>
#include <algorithm>

RadarProductStormRelativeVelocity::RadarProductStormRelativeVelocity(){
	volumeType = RadarData::VOLUME_STORM_RELATIVE_VELOCITY;
	productType = PRODUCT_DERIVED_VOLUME;
	name = "Storm Relative Velocity";
	shortName = "SRV";
	dependencies = {RadarData::VOLUME_VELOCITY_DEALIASED};
};

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

RadarData* RadarProductStormRelativeVelocity::deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts){
	RadarData* radarData = new RadarData();
	RadarData* radarDataSrc = inputProducts[RadarData::VOLUME_VELOCITY_DEALIASED];
	radarDataSrc->Decompress();
	radarData->CopyFrom(radarDataSrc);
	std::fill(radarData->buffer, radarData->buffer + radarData->usedBufferSize, 0.0f);
	
	// number of sections to divide like a pie chart for bins
	int thetaSections = 12;
	// length of bins radially
	int radiusSectionSize = 32;
	int radiusSections = radarData->radiusBufferCount / radiusSectionSize + 1;
	
	float* bins = new float[thetaSections * radiusSections]{};
	//std::fill(bins, bins + (thetaSections * radiusSections), 0.0f);
	float* binCounts = new float[thetaSections * radiusSections]{};
	//std::fill(binCounts, binCounts + (thetaSections * radiusSections), 0.0f);
	
	// read radar data to fill bins
	for(int sweep = 0; sweep < radarData->sweepBufferCount; sweep++){
		for(int theta = 1; theta < radarData->thetaBufferCount + 2; theta++){
			float* raySrc = radarDataSrc->buffer + (radarData->thetaBufferSize * theta + radarData->sweepBufferSize * sweep);
			int thetaBin = (theta - 1) / (float)radarData->thetaBufferCount * thetaSections;
			if (!radarData->rayInfo[sweep * (radarData->thetaBufferCount + 2) + theta].interpolated) {
				for(int radius = 5; radius < radarData->radiusBufferCount; radius++){
					int binIndex = thetaBin * radiusSections + radius / radiusSectionSize;
					float value = raySrc[radius];
					if(value != 0){
						bins[binIndex] += value;
						binCounts[binIndex]++;
					}
				}
			}
		}
	}
	
	// average bins
	for(int theta = 0; theta < thetaSections; theta++){
		for(int radius = 0; radius < radiusSections; radius++){
			int binIndex = theta * radiusSections + radius;
			if(binCounts[binIndex] > 0){
				bins[binIndex] /= binCounts[binIndex];
			}
		}
	}
	delete[] binCounts;
	
	// create output data
	for(int sweep = 0; sweep < radarData->sweepBufferCount; sweep++){
		for(int theta = 1; theta < radarData->thetaBufferCount + 2; theta++){
			float* ray = radarData->buffer + (radarData->thetaBufferSize * theta + radarData->sweepBufferSize * sweep);
			float* raySrc = radarDataSrc->buffer + (radarData->thetaBufferSize * theta + radarData->sweepBufferSize * sweep);
			float thetaBin = (theta - 1) / (float)radarData->thetaBufferCount * thetaSections;
			if (!radarData->rayInfo[sweep * (radarData->thetaBufferCount + 2) + theta].interpolated) {
				for(int radius = 0; radius < radarData->radiusBufferCount; radius++){
					// interpolate bins
					int binIndexTheta1 = modulo((int)std::floor(thetaBin - 0.5f), thetaSections) * radiusSections;
					int binIndexTheta2 = modulo((int)std::floor(thetaBin + 0.5f), thetaSections) * radiusSections;
					int binIndexRadius1 = std::max((int)(radius / (float)radiusSectionSize - 0.5f), 0);
					int binIndexRadius2 = std::min((int)(radius / (float)radiusSectionSize + 0.5f), radiusSections);
					float bin11 = bins[binIndexTheta1 + binIndexRadius1];
					float bin12 = bins[binIndexTheta1 + binIndexRadius2];
					float bin21 = bins[binIndexTheta2 + binIndexRadius1];
					float bin22 = bins[binIndexTheta2 + binIndexRadius2];
					float delta1 = std::fmod(thetaBin + 0.5f, 1.0f);
					float delta2 = std::fmod(radius / (float)radiusSectionSize + 0.5f, 1.0f);
					float bin1 = bin12 * delta2 + bin11 * (1 - delta2);
					float bin2 = bin22 * delta2 + bin21 * (1 - delta2);
					float bin = bin2 * delta1 + bin1 * (1 - delta1);
					
					float value = raySrc[radius];
					ray[radius] = value == 0 ? 0 : value - bin;
				}
			}
		}
	}
	delete[] bins;
	
	
	// float valueRange = radarData->stats.maxValue - radarData->stats.minValue;
	// float threshold = valueRange * 0.2f;
	
	// DealiasingAlgorithm algo = {};
	// algo.threashold = threshold;
	// algo.range = valueRange;
	// algo.vol = radarData;
	// algo.src = inputProducts[RadarData::VOLUME_VELOCITY];
	// algo.src->Decompress();
	// algo.used = new bool[radarData->fullBufferSize];
	// std::fill(algo.used, algo.used + radarData->fullBufferSize, false);
	// algo.groupId = 20;

	// algo.FindAllGroups();
	
	// radarData->stats.minValue = 0;
	// radarData->stats.maxValue = algo.groupId + 1;
	// radarData->stats.volumeType = RadarData::VOLUME_UNKNOWN;
	// delete algo.used;
	
	radarData->Interpolate();
	return radarData;
};