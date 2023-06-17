#pragma once

#include "../Radar/SimpleVector3.h"

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "MapMesh.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class Globe;
class AMapMeshManager;
class UMaterialInstanceDynamic;
class UTexture2D;
class UTexture;
class AsyncTaskRunner;

UCLASS()
class AMapMesh : public AActor{
	GENERATED_BODY()
	
public:
	// number of sections to divide each axis of the mesh into
	UPROPERTY(EditAnywhere)
	int divisions = 8;
	// id of the child for the parent
	UPROPERTY(EditAnywhere)
	int childId = -1;
	// what zoom layer this is a part of
	UPROPERTY(EditAnywhere)
	int layer = 0;
	// tile for given zoom layer
	UPROPERTY(EditAnywhere)
	int tileX = 0;
	// tile for given zoom layer
	UPROPERTY(EditAnywhere)
	int tileY = 0;
	// max layers of zoom
	UPROPERTY(EditAnywhere)
	int maxLayer = 15;
	// if this mesh is loaded
	UPROPERTY(EditAnywhere)
	bool loaded = true;
	// if this mesh has a texture loaded
	UPROPERTY(EditAnywhere)
	bool fullyLoaded = false;
	// location
	UPROPERTY(EditAnywhere)
	double latitudeRadians = 0;
	UPROPERTY(EditAnywhere)
	// location
	double longitudeRadians = 0;
	// height
	UPROPERTY(EditAnywhere)
	double latitudeHeightRadians = 1;
	// width
	UPROPERTY(EditAnywhere)
	double longitudeWidthRadians = 1;
	// when closer than this to the camera subdivide into next layer
	UPROPERTY(EditAnywhere)
	float subdivideDistance = 0;
	// center of the tile in game units
	SimpleVector3<> centerPosition = {};
	// last rotation from UpdateRotation to pass to children
	SimpleVector3<> appliedRotation = {};
	

	Globe* globe = NULL;
	
	UPROPERTY(EditAnywhere)
	UMaterialInterface* material;
	
	UPROPERTY(VisibleAnywhere)
	UMaterialInstanceDynamic* materialInstance = NULL;
	
	// texture generated asynchronously
	UPROPERTY(VisibleAnywhere)
	UTexture2D* texture = NULL;
	// is a new texture ready
	bool pendingTexture = false;
	
	UPROPERTY(EditAnywhere);
	UProceduralMeshComponent* proceduralMesh;
	
	UPROPERTY(EditAnywhere);
	AMapMeshManager* manager = NULL;
	
	AsyncTaskRunner* tileLoader = NULL;
	AsyncTaskRunner* textureLoader = NULL;
	
	UPROPERTY(EditAnywhere);
	AMapMesh* mapParent = NULL;
	
	UPROPERTY(EditAnywhere);
	AMapMesh* mapChildren[4] = {};
	
	AMapMesh();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	
	void GenerateMesh();
	
	// attempt to load the tile texture
	void LoadTile();
	
	// set the bounds of for the section
	void SetBounds(double latitudeRadians, double longitudeRadians, double latitudeHeightRadians, double longitudeWidthRadians);
	
	// duality of man
	void MakeChildren();
	void DestroyChildren();
	
	// update and check for changes, is run every tick
	void Update();
	
	// pull updated data from AMapMeshManager
	void UpdateParameters();
	
	// set center and rotation of globe
	void UpdatePosition(SimpleVector3<> position, SimpleVector3<> rotation);
	
	// propagate loaded texture
	void UpdateTexture(UTexture* texture, bool fromParent);
	
	void ProjectionOntoGlobeTest();
};