#pragma once

#include "../Radar/SimpleVector3.h"
#include "../Application/GlobalState.h"

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "RadarSlice.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class Globe;
class AMapMeshManager;
class UMaterialInstanceDynamic;
class UTexture2D;
class UTexture;
class ARadarVolumeRender;

#ifndef UPROPERTY
#define UPROPERTY(a) ;
#endif


UCLASS()
class ARadarSlice : public AActor{
	GENERATED_BODY()
	
public:
	
	// height above sea level
	float sliceAltitude = 10000;
	// angle of slice in degrees
	float sliceAngle = 1;
	// x location of vertical slice
	float locationX = 0;
	// y location of vertical slice
	float locationY = 0;
	// rotation of vertical slice
	float rotation = 0;
	
	// width or radius of mesh in game units
	float radius = 100 * 50;
	// maximum radius of the volume
	float radiusMax = 100 * 50;
	// bottom of volume
	float bottomHeight = 0;
	// top of volume
	float topHeight = 0;
	// number of times to divide mesh on each side to generate divisions*divisions quads for flat slices
	float divisions = 40;
	// number of triangles to divide the mesh into for circular sweeps
	float sectors = 100;
	// location of camera to use for sorting triangles
	SimpleVector3<float> cameraLocation;
	float lastCameraAngle = 0;
	// globe to place on
	Globe* globe;
	GlobalState::SliceMode sliceMode = GlobalState::SLICE_MODE_SWEEP_ANGLE;
	
	
	std::vector<uint64_t> callbackIds = {};
	
	UPROPERTY(EditAnywhere)
	UProceduralMeshComponent* proceduralMesh;
	
	
	UPROPERTY(EditAnywhere)
	ARadarVolumeRender* mainVolumeRender = NULL;
	
	
	UPROPERTY(EditAnywhere)
	UMaterialInstanceDynamic* radarMaterialInstance = NULL;
	
	ARadarSlice();
	
	template <class T>
	T* FindActor();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	
	void GenerateMesh();
};