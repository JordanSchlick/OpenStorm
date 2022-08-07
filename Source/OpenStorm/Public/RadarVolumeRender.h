// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h" 
#include "Components/StaticMeshComponent.h"
#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#include "RadarVolumeRender.generated.h"

UCLASS()
class OPENSTORM_API ARadarVolumeRender : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARadarVolumeRender();

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* cubeMeshComponent;
	//UPROPERTY(VisibleAnywhere)
	UMaterialInstanceDynamic* radarMaterialInstance = NULL;
	UStaticMesh* cubeMesh;
	UMaterial* storedMaterial;
	UTexture2D* volumeTexture;
	FByteBulkData* volumeImageData;
	UTexture2D* angleIndexTexture;
	FByteBulkData* angleIndexImageData;
	UTexture2D* valueIndexTexture;
	FByteBulkData* valueIndexImageData;
	static ARadarVolumeRender* instance;
	void RandomizeTexture();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
