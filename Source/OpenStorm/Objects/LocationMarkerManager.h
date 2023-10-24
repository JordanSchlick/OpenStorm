#pragma once

#include <map>
#include <vector>
#include <string>

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "LocationMarkerManager.generated.h"

class ALocationMarker;

UCLASS()
class ALocationMarkerManager : public AActor{
	GENERATED_BODY()
public:
	std::map<std::string, ALocationMarker*> locationMarkerObjects = {};
	std::vector<uint64_t> callbackIds = {};
	
	float maxSiteMarkerDistance = 7500;
	
	ALocationMarkerManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	
	// update all waypoint location markers
	void UpdateWaypointMarkers();
	
	// add any site markers that need to be added
	void AddSiteMarkers();
	
	// update all location markers based on globalState
	void UpdateMarkerLocations();
};