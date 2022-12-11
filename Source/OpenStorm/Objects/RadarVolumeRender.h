// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "../Radar/RadarCollection.h"
#include "../Radar/RadarColorIndex.h"
#include "../Application/GlobalState.h"

#include <vector>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/Object.h"
#include "Materials/Material.h"
#include "Engine/Texture2D.h"
//#include "Engine/TextureRenderTarget2D.h"
//#include "Engine/CanvasRenderTarget2D.h"
#include "RadarVolumeRender.generated.h"


class UMaterialRenderTarget;

UCLASS()
class OPENSTORM_API ARadarVolumeRender : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARadarVolumeRender();
	~ARadarVolumeRender();

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* cubeMeshComponent;
	//UPROPERTY(VisibleAnywhere)
	UPROPERTY(VisibleAnywhere)
	UMaterialInstanceDynamic* radarMaterialInstance = NULL;
	UPROPERTY(VisibleAnywhere)
	UMaterialInstanceDynamic* interpolationMaterialInstance = NULL;
	UPROPERTY(VisibleAnywhere)
	UStaticMesh* cubeMesh = NULL;
	UPROPERTY(VisibleAnywhere)
	UMaterial* storedMaterial = NULL;
	UPROPERTY(EditAnywhere)
	UMaterial* storedInterpolationMaterial = NULL;
	
	// true if time interpolation is enabled. call InitializeTextures after changing
	bool doTimeInterpolation = false;
	
	// if interpolation is being animated
	bool interpolationAnimating = false;
	float interpolationStartValue = 0;
	float interpolationEndValue = 0;
	double interpolationStartTime = 0;
	double interpolationEndTime = 0;
	
	// write to volume 1 if true and write to volume 2 if false
	bool usePrimaryTexture = true;
	UPROPERTY(VisibleAnywhere)
	UTexture2D* volumeTexture = NULL;
	//FByteBulkData* volumeImageData;
	UPROPERTY(VisibleAnywhere)
	UTexture2D* volumeTexture2 = NULL;
	//FByteBulkData* volumeImageData2;
	UPROPERTY(VisibleAnywhere)
	UMaterialRenderTarget* volumeMaterialRenderTarget = NULL;

	FRenderTarget* volumeRenderTarget;
	UPROPERTY(VisibleAnywhere)
	UTexture2D* angleIndexTexture = NULL;
	//FByteBulkData* angleIndexImageData;
	UPROPERTY(VisibleAnywhere)
	UTexture2D* valueIndexTexture = NULL;
	//FByteBulkData* valueIndexImageData;
	static ARadarVolumeRender* instance;
	RadarCollection* radarCollection = NULL;
	RadarData* radarData;
	
	//GlobalState* globalState = NULL;
	std::vector<uint64_t> callbackIds = {};
	RadarColorIndex* radarColor = &RadarColorIndexReflectivity::defaultInstance;
	RadarColorIndex::Result radarColorResult = {};
	RadarColorIndex::Result radarColorResultAlternate = {};
	
	// load data from RadarUpdateEvent into shader
	void HandleRadarDataEvent(RadarCollection::RadarUpdateEvent event);
	
	void RandomizeTexture();

	//Initialize all textures or reinitialize ones that need it
	void InitializeTextures();
	
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
