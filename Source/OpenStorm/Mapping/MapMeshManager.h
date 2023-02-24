#pragma once

#include "../Radar/SimpleVector3.h"

#include <vector>

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "MapMeshManager.generated.h"

class AMapMesh;
class Globe;
class TileProvider;

UCLASS()
class AMapMeshManager : public AActor{
	GENERATED_BODY()
	
public:
	// max layers of zoom
	int maxLayer = 15;
	// location of camera/pawn
	SimpleVector3<> cameraLocation = {};
	// globe used for generating meshes
	Globe* globe;
	// if the map is enabled
	bool enabled = false;
	// callback ids for global state
	std::vector<uint64_t> callbackIds = {};
	// root of map mesh tree
	UPROPERTY(EditAnywhere);
	AMapMesh* rootMapMesh = NULL;
	
	TileProvider* tileProvider = NULL;
	
	
	AMapMeshManager();
	~AMapMeshManager();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	void EnableMap();
	void DisableMap();
	
};