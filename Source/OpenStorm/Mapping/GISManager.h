#pragma once

#include "../Radar/SimpleVector3.h"

#include <vector>

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "GISManager.generated.h"

class AMapMesh;
class Globe;
class TileProvider;

UCLASS()
class AGISManager : public AActor{
	GENERATED_BODY()
	
public:
	// location of camera/pawn
	SimpleVector3<> cameraLocation = {};
	// globe used for generating meshes
	Globe* globe;
	// if the map is enabled
	bool enabled = false;
	// the brightness of the map
	float mapBrightness = 0.2;
	// callback ids for global state
	std::vector<uint64_t> callbackIds = {};
	
	// number of ticks to wait after game start to start map
	int preTicks = 10;
	
	// update position of all meshes after globe update
	void UpdatePositionsFromGlobe();
	
	AGISManager();
	~AGISManager();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	void EnableMap();
	void DisableMap();
	
};