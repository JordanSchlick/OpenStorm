// Fill out your copyright notice in the Description page of Project Settings.


#include "RadarViewport.h"


// Sets default values
ARadarViewport::ARadarViewport()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ViewportMesh"));

	// Set the component's mesh

	UStaticMesh* cubeMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'")).Object;

	UMaterial* material = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/RadarVolumeMaterial.RadarVolumeMaterial'")).Object;



	meshComponent->SetStaticMesh(cubeMesh);

	meshComponent->SetMaterial(0, material);

	// Set as root component
	RootComponent = meshComponent;
}

// Called when the game starts or when spawned
void ARadarViewport::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARadarViewport::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (radarMaterialInstance == NULL) {
		if (mainVolumeRender != NULL) {
			if (mainVolumeRender->radarMaterialInstance != NULL) {
				radarMaterialInstance = mainVolumeRender->radarMaterialInstance;
				meshComponent->SetMaterial(0, radarMaterialInstance);
			}
		}
	}
}

