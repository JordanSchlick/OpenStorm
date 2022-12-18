#include "VelocityProducts.h"

RadarProductVelocityAntialiased::RadarProductVelocityAntialiased(){
	volumeType = RadarData::VOLUME_VELOCITY_ANTIALIASED;
	productType = PRODUCT_DERIVED_VOLUME;
	name = "Radial Velocity Anti-aliased";
	dependencies = {RadarData::VOLUME_VELOCITY};
}

RadarData* RadarProductVelocityAntialiased::deriveVolume(std::map<RadarData::VolumeType, RadarData *> inputProducts){
	RadarData* radarData = new RadarData();
	radarData->CopyFrom(inputProducts[RadarData::VOLUME_VELOCITY]);
	//fprintf(stderr, "---- %p %i %i\n", radarData->buffer, radarData->fullBufferSize, inputProducts[RadarData::VOLUME_VELOCITY]->fullBufferSize);
	int len = radarData->fullBufferSize;
	for(int i = 0; i < len; i++){
		radarData->buffer[i] *= -1;
	}
	return radarData;
}
