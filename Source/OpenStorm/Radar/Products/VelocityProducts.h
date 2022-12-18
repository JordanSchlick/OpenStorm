#include "RadarProduct.h"

class RadarProductVelocityAntialiased : public RadarProduct {
public:
	RadarProductVelocityAntialiased();
	virtual RadarData* deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts) override;
};