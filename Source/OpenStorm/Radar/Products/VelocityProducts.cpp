#include "VelocityProducts.h"

RadarProductVelocityDealiased::RadarProductVelocityDealiased(){
	volumeType = RadarData::VOLUME_VELOCITY_DEALIASED;
	productType = PRODUCT_DERIVED_VOLUME;
	name = "Radial Velocity Dealiased";
	shortName = "RVD";
	dependencies = {RadarData::VOLUME_VELOCITY};
}

RadarData* RadarProductVelocityDealiased::deriveVolume(std::map<RadarData::VolumeType, RadarData *> inputProducts){
	RadarData* radarData = new RadarData();
	radarData->CopyFrom(inputProducts[RadarData::VOLUME_VELOCITY]);
	//fprintf(stderr, "---- %p %i %i\n", radarData->buffer, radarData->fullBufferSize, inputProducts[RadarData::VOLUME_VELOCITY]->fullBufferSize);
	int len = radarData->fullBufferSize;
	for(int i = 0; i < len; i++){
		radarData->buffer[i] *= -1;
	}
	return radarData;
}
