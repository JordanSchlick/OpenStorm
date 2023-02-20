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

UCLASS()
class AMapMesh : public AActor{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	int divisions = 8;
	UPROPERTY(EditAnywhere)
	int childId = -1;
	UPROPERTY(EditAnywhere)
	int layer = 0;
	UPROPERTY(EditAnywhere)
	int maxLayer = 15;
	UPROPERTY(EditAnywhere)
	bool loaded = true;
	UPROPERTY(EditAnywhere)
	double latitudeRadians = 0;
	UPROPERTY(EditAnywhere)
	double longitudeRadians = 0;
	UPROPERTY(EditAnywhere)
	double latitudeHeightRadians = 1;
	UPROPERTY(EditAnywhere)
	double longitudeWidthRadians = 1;
	// when closer than this to the camera subdivide into next layer
	UPROPERTY(EditAnywhere)
	float subdivideDistance = 0;
	SimpleVector3<> centerPosition = {};
	

	Globe* globe = NULL;
	
	UPROPERTY(EditAnywhere)
	UMaterialInterface* material;
	
	UPROPERTY(EditAnywhere);
	UProceduralMeshComponent* proceduralMesh;
	
	UPROPERTY(EditAnywhere);
	AMapMeshManager* manager = NULL;
	
	
	UPROPERTY(EditAnywhere);
	AMapMesh* mapParent = NULL;
	
	UPROPERTY(EditAnywhere);
	AMapMesh* mapChildren[4] = {};
	
	AMapMesh();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	
	void GenerateMesh();
	
	// set the bounds of for the section
	void SetBounds(double latitudeRadians, double longitudeRadians, double latitudeHeightRadians, double longitudeWidthRadians);
	
	// duality of man
	void MakeChildren();
	void DestroyChildren();
	
	// update and check for changes
	void Update();
	
	// set center and rotation of globe
	void UpdatePosition(SimpleVector3<> position, SimpleVector3<> rotation);
	
	
	void ProjectionOntoGlobeTest();
};