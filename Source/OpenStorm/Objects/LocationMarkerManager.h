#pragma once

#include <vector>

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "LocationMarkerManager.generated.h"

class ALocationMarker;

UCLASS()
class ALocationMarkerManager : public AActor{
	GENERATED_BODY()
public:
	std::vector<ALocationMarker*> locationMarkerObjects = {};
	std::vector<uint64_t> callbackIds = {};
	
	ALocationMarkerManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	
	// update all location markers based on globalState
	void UpdateLocationMarkers();
};