#include "Satellite.h"

#include "Engine/Texture2D.h"
#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"

ASatellite::ASatellite()
{
	PrimaryActorTick.bCanEverTick = true;
	UStaticMeshComponent* meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	UStaticMesh* mesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'")).Object;
	meshComponent->SetStaticMesh(mesh);
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
	SimpleVector3 vector = globe.GetPointScaledDegrees(latitude, longitude, altitude);
	fprintf(stderr, "vec %f %f %f \n", vector.x,vector.y,vector.z);
	fprintf(stderr, "lng %f\n", longitude);
	SetActorLocation(FVector(vector.x,vector.y,vector.z));
	//fprintf(stderr,"tick2 ");
}