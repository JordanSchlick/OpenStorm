#pragma once

#include "../Radar/SimpleVector3.h"

#include <vector>
#include <map>

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "GISManager.generated.h"

class GISLoaderTask;
class Globe;
class AGISPolyline;

UCLASS()
class AGISManager : public AActor{
	GENERATED_BODY()
	
public:
	// location of camera/pawn
	SimpleVector3<float> cameraLocation = {};
	// globe used for generating meshes
	Globe* globe;
	// if the map is enabled
	bool enabled = false;
	// if the camera has moved and the task needs to be re-run
	bool needsRecalculation = true;
	// the brightness of the map
	float mapBrightness = 0.2;
	// callback ids for global state
	std::vector<uint64_t> callbackIds = {};
	// all displayed polylines with indexes of the object they were created from in the object vector in the task 
	std::map<uint64_t, AGISPolyline*> polylines;
	// number of ticks to wait after game start to start map
	int preTicks = 10;
	
	GISLoaderTask* loaderTask;
	
	// update position of all meshes after globe update
	void UpdatePositionsFromGlobe();
	
	AGISManager();
	~AGISManager();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	
	void EnableMap();
	void DisableMap();
	
};