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
	
	
	// width or radius of mesh in game units
	float size = 100 * 50;
	// height above sea level
	float sliceAltitude = 10000;
	// angle of slice in degrees
	float sliceAngle = 1;
	// number of times to divide mesh on each side to generate divisions*divisions quads for flat slices
	float divisions = 10;
	// number of triangles to divide the mesh into for circular sweeps
	float sectors = 100;
	// globe to place on
	Globe* globe;
	GlobalState::SliceMode sliceMode = GlobalState::SLICE_MODE_SWEEP_ANGLE;
	
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