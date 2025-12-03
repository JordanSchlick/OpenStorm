#include "RadarProduct.h"

class RadarProductNormalizedRotation : public RadarProduct {
public:
	RadarProductNormalizedRotation();
	virtual RadarData* deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts) override;
};