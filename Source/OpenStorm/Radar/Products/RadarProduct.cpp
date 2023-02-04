#include "RadarProduct.h"


RadarProduct* RadarProduct::GetProduct(RadarData::VolumeType type){
	// if(productsMap.find(type) != productsMap.end()){
	// 	return productsMap[type];
	// }else{
	// 	return NULL;
	// }
	for(RadarProduct* product : RadarProduct::products){
		if(product->volumeType == type){
			return product;
		}
	}
	return NULL;
}

int RadarProduct::dynamicVolumeTypeId = 1000;

RadarData::VolumeType RadarProduct::CreateDynamicVolumeType(){
	return (RadarData::VolumeType)dynamicVolumeTypeId++;
}

RadarData *RadarProduct::deriveVolume(std::map<RadarData::VolumeType, RadarData *> inputProducts) {
	return NULL;
}

RadarProduct::~RadarProduct(){
	
}

RadarProductBase::RadarProductBase(RadarData::VolumeType type, std::string productName, std::string shortProductName){
	volumeType = type;
	productType = PRODUCT_BASE;
	name = productName;
	shortName = shortProductName;
}