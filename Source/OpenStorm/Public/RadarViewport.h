// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "RadarVolumeRender.h"

#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "RadarViewport.generated.h"

UCLASS()
class OPENSTORM_API ARadarViewport : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARadarViewport();
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* meshComponent;

	UPROPERTY(EditAnywhere)
		ARadarVolumeRender* mainVolumeRender = NULL;

	UMaterialInstanceDynamic* radarMaterialInstance = NULL;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
