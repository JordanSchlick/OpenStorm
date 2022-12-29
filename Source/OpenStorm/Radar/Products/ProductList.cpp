#include "RadarProduct.h"
#include "VelocityProducts.h"
#include "RotationProduct.h"

std::map<RadarData::VolumeType, RadarProduct*> RadarProduct::products = {
	{RadarData::VOLUME_REFLECTIVITY, new RadarProductBase(RadarData::VOLUME_REFLECTIVITY, "Reflectivity", "REF")},
	{RadarData::VOLUME_VELOCITY, new RadarProductBase(RadarData::VOLUME_VELOCITY, "Radial Velocity","RV")},
	{RadarData::VOLUME_SPECTRUM_WIDTH, new RadarProductBase(RadarData::VOLUME_SPECTRUM_WIDTH, "Spectrum Width","SW")},
	{RadarData::VOLUME_VELOCITY_DEALIASED, new RadarProductVelocityDealiased()},
	{(RadarData::VolumeType)RadarProduct::dynamicVolumeTypeId, new RadarProductVelocityDealiasedGroupTest(RadarProduct::CreateDynamicVolumeType())},
	{RadarData::VOLUME_ROTATION, new RadarProductRotation()},
};
