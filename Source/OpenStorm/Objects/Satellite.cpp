#include "Satellite.h"

#include "../Application/GlobalState.h"

#include "Engine/Texture2D.h"
#include "UObject/Object.h"
#include "Materials/Material.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"

ASatellite::ASatellite()
{
	PrimaryActorTick.bCanEverTick = true;
	UStaticMeshComponent* meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	UStaticMesh* mesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'")).Object;
	UMaterial* material = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/TestImageMaterial.TestImageMaterial'")).Object;
	
	meshComponent->SetStaticMesh(mesh);
	meshComponent->SetMaterial(0, material);
	RootComponent = meshComponent;
}

void ASatellite::Reset() {
	latitude = startLatitude;
	longitude = startLongitude;
}

void ASatellite::SetGlobe() {
	globe.SetCenter(globeCenterX, globeCenterY, globeCenterZ);
	globe.SetRotationDegrees(90 - globeUpLatitude, -globeUpLongitude);
}

void ASatellite::BeginPlay() {
	Super::BeginPlay();
}

void ASatellite::Tick(float DeltaTime){
	latitude += speedLatitude * DeltaTime;
	longitude += speedLongitude * DeltaTime;
	if(doSetGlobe){
		SetGlobe();
	}
	if(doReset){
		Reset();
	}
	if(doUseGlobalGlobe){
		GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
		globe = *globalState->globe;
	}
	SimpleVector3 vector = globe.GetPointScaledDegrees(latitude, longitude, altitude);
	fprintf(stderr, "vec %f %f %f \n", vector.x,vector.y,vector.z);
	fprintf(stderr, "lng %f\n", longitude);
	SetActorLocation(FVector(vector.x,vector.y,vector.z));
	//fprintf(stderr,"tick2 ");
}