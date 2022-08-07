// Fill out your copyright notice in the Description page of Project Settings.


#include "RadarViewPawn.h"

// Sets default values
ARadarViewPawn::ARadarViewPawn()
{
	//Setup Rootcomponent
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ViewportMesh"));
	
	UStaticMesh * playerMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Game/Meshes/inverted_cube.inverted_cube'")).Object;

	UMaterial * material = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/RadarVolumeMaterial.RadarVolumeMaterial'")).Object;
	
	camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));


	meshComponent->SetStaticMesh(playerMesh);

	meshComponent->SetMaterial(0, material);
	
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
	camera->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	meshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	
	SetActorEnableCollision(false);
}

// Called when the game starts or when spawned
void ARadarViewPawn::BeginPlay()
{
	Super::BeginPlay();
	meshComponent->SetRelativeScale3D(FVector3d(0.25, 2, 2));
	mainVolumeRender = ARadarVolumeRender::instance;
}

// Called every frame
void ARadarViewPawn::Tick(float DeltaTime)
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

// Called to bind functionality to input
void ARadarViewPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveFB", this, &ARadarViewPawn::MoveFB);
	PlayerInputComponent->BindAxis("MoveLR", this, &ARadarViewPawn::MoveLR);
	PlayerInputComponent->BindAxis("MoveUD", this, &ARadarViewPawn::MoveUD);
	PlayerInputComponent->BindAxis("RotateLR", this, &ARadarViewPawn::RotateLR);
	PlayerInputComponent->BindAxis("RotateUD", this, &ARadarViewPawn::RotateUD);
	
}

void ARadarViewPawn::MoveFB(float value)
{
	auto Location = GetActorLocation();
	Location += GetActorForwardVector() * value * MoveSpeed;
	SetActorLocation(Location);
}

void ARadarViewPawn::MoveLR(float value)
{
	auto Location = GetActorLocation();
	Location += GetActorRightVector() * value * MoveSpeed;
	SetActorLocation(Location);
}

void ARadarViewPawn::MoveUD(float value)
{
	auto Location = GetActorLocation();
	Location.Z += value * MoveSpeed;
	SetActorLocation(Location);
}

void ARadarViewPawn::RotateUD(float Value)
{
	auto Rotation = GetActorRotation();
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch + Value * RotateSpeed, -89.0f, 89.0f);
	Rotation.Roll = 0;
	SetActorRotation(Rotation);
}

void ARadarViewPawn::RotateLR(float Value)
{
	auto Rotation = GetActorRotation();
	Rotation.Yaw += Value * RotateSpeed;
	SetActorRotation(Rotation);
}






