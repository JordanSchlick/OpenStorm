#include "RadarProduct.h"
#include "VelocityProducts.h"

std::map<RadarData::VolumeType, RadarProduct*> RadarProduct::products = {
	{RadarData::VOLUME_REFLECTIVITY, new RadarProductBase(RadarData::VOLUME_REFLECTIVITY, "Reflectivity")},
	{RadarData::VOLUME_VELOCITY, new RadarProductBase(RadarData::VOLUME_VELOCITY, "Radial Velocity")},
	{RadarData::VOLUME_SPECTRUM_WIDTH, new RadarProductBase(RadarData::VOLUME_SPECTRUM_WIDTH, "Spectrum Width")},
	{RadarData::VOLUME_VELOCITY_ANTIALIASED, new RadarProductVelocityAntialiased()},
};
