// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h" 
#include "Components/StaticMeshComponent.h"
#include "TestTexture.generated.h"

UCLASS()
class OPENSTORM_API ATestTexture : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestTexture();

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* cubeMeshComponent;
	UPROPERTY(VisibleAnywhere)
		UMaterialInstanceDynamic* DynamicMaterialInst;
	UStaticMesh* cubeMesh;
	UMaterial* StoredMaterial;
	FByteBulkData* ImageData;
	UTexture2D* CustomTexture;
	void RandomizeTexture();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
