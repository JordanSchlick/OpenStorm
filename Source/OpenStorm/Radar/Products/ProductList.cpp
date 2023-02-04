#include "RadarProduct.h"
#include "VelocityDealiasProduct.h"
#include "VelocityRelativeProduct.h"
#include "RotationProduct.h"

std::vector<RadarProduct*> RadarProduct::products = {
	{new RadarProductBase(RadarData::VOLUME_REFLECTIVITY, "Reflectivity", "REF")},
	{new RadarProductBase(RadarData::VOLUME_VELOCITY, "Radial Velocity","RV")},
	{new RadarProductBase(RadarData::VOLUME_SPECTRUM_WIDTH, "Spectrum Width","SW")},
	{new RadarProductVelocityDealiased()},
	{new RadarProductStormRelativeVelocity()},
	{new RadarProductRotation()},
	{new RadarProductVelocityDealiasedGroupTest(RadarProduct::CreateDynamicVolumeType())},
};

//std::map<RadarData::VolumeType, RadarProduct*> RadarProduct::productsMap = {};

