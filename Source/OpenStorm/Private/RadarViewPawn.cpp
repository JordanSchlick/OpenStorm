// Fill out your copyright notice in the Description page of Project Settings.


#include "RadarViewPawn.h"
#include "RadarGameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h" 
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"




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
	//mainVolumeRender = ARadarVolumeRender::instance;
	mainVolumeRender = FindActor<ARadarVolumeRender>();
	gui = FindActor<AImGuiUI>();
	hud = new HUD(GetWorld()->GetGameViewport());
}

// Called every frame
void ARadarViewPawn::Tick(float deltaTime)
{
	Super::Tick(deltaTime);
	if (radarMaterialInstance == NULL) {
		if (mainVolumeRender != NULL) {
			if (mainVolumeRender->radarMaterialInstance != NULL) {
				radarMaterialInstance = mainVolumeRender->radarMaterialInstance;
				meshComponent->SetMaterial(0, radarMaterialInstance);
			}
		}
	}
	if (ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>()){
		moveSpeed = GS->globalState.moveSpeed;
		FVector location = GetActorLocation();
		location += camera->GetForwardVector() * forwardMovement * deltaTime * moveSpeed;
		location += camera->GetRightVector() * sidewaysMovement * deltaTime * moveSpeed;
		location.Z += verticalMovement * deltaTime * moveSpeed;
		SetActorLocation(location);

		FRotator rotation = GetActorRotation();
		if(GS->globalState.vrMode){
			rotation.Pitch = 0;
		}else{
			rotation.Pitch = FMath::Clamp(rotation.Pitch + (verticalRotation * deltaTime + verticalRotationAmount / 60) * rotateSpeed, -89.0f, 89.0f);
			camera->SetRelativeLocation(FVector(0, 0, 0));
			camera->SetRelativeRotation(FRotator(0, 0, 0));
		}
		verticalRotationAmount = 0;
		rotation.Yaw += (horizontalRotation * deltaTime + horizontalRotationAmount / 60) * rotateSpeed;
		horizontalRotationAmount = 0;
		rotation.Roll = 0;
		SetActorRotation(rotation);
		hud->SetCompassRotation(rotation.Yaw / 360.0f);
		//GS->globalState.testFloat = forwardMovement;
	}
	meshComponent->SetRelativeLocation(camera->GetRelativeLocation());
	meshComponent->SetRelativeRotation(camera->GetRelativeRotation());
	
}

ARadarViewPawn::~ARadarViewPawn(){
	if(hud != NULL){
		delete hud;
		hud = NULL;
	}
}

template <class T>
T* ARadarViewPawn::FindActor() {
	TArray<AActor*> FoundActors = {};
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), T::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0) {
		return (T*)FoundActors[0];
	} else {
		return NULL;
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
	PlayerInputComponent->BindAxis("RotateMouseLR", this, &ARadarViewPawn::RotateMouseLR);
	PlayerInputComponent->BindAxis("RotateMouseUD", this, &ARadarViewPawn::RotateMouseUD);
	PlayerInputComponent->BindAction("MouseButton", IE_Released, this, &ARadarViewPawn::ReleaseMouse);
	PlayerInputComponent->BindAction("MouseButton", IE_Pressed, this, &ARadarViewPawn::PressMouse);
	
}

void ARadarViewPawn::MoveFB(float value)
{
	forwardMovement = value;
	// auto Location = GetActorLocation();
	// Location += camera->GetForwardVector() * value * MoveSpeed;
	// SetActorLocation(Location);
}

void ARadarViewPawn::MoveLR(float value)
{
	sidewaysMovement = value;
	// auto Location = GetActorLocation();
	// Location += camera->GetRightVector() * value * MoveSpeed;
	// SetActorLocation(Location);
}

void ARadarViewPawn::MoveUD(float value)
{
	verticalMovement = value;
	// auto Location = GetActorLocation();
	// Location.Z += value * MoveSpeed;
	// SetActorLocation(Location);
}

void ARadarViewPawn::RotateUD(float value)
{
	verticalRotation = value;
	// auto Rotation = GetActorRotation();
	// Rotation.Pitch = FMath::Clamp(Rotation.Pitch + value * RotateSpeed, -89.0f, 89.0f);
	// Rotation.Roll = 0;
	// SetActorRotation(Rotation);
}

void ARadarViewPawn::RotateLR(float value)
{
	horizontalRotation = value;
	// auto Rotation = GetActorRotation();
	// Rotation.Yaw += Value * RotateSpeed;
	// SetActorRotation(Rotation);
}

void ARadarViewPawn::RotateMouseUD(float value)
{
	verticalRotationAmount += value;
}

void ARadarViewPawn::RotateMouseLR(float value)
{
	horizontalRotationAmount += value;
}

void ARadarViewPawn::ReleaseMouse() {
	// return mouse to gui
	fprintf(stderr, "Relesed mouse button\n");
	if (gui != NULL) {
		gui->UnlockMouse();
	}
}

void ARadarViewPawn::PressMouse() {
	// take mouse from gui
	fprintf(stderr, "Pressed mouse button\n");
	if (gui != NULL) {
		gui->LockMouse();
	}
}




