#define _USE_MATH_DEFINES
#include "MapMeshManager.h"
#include "MapMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Data/ElevationData.h"


#include <cmath>

#define M_PI       3.14159265358979323846

AMapMeshManager::AMapMeshManager(){
	
}

void AMapMeshManager::BeginPlay(){
	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();
	rootMapMesh = GetWorld()->SpawnActor<AMapMesh>(AMapMesh::StaticClass());
	rootMapMesh->SetBounds(
		0,
		0,
		M_PI,
		M_PI * 2
	);
	rootMapMesh->manager = this;
	
	FString elevationFile =  FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Data/elevation.bin"));
	FString fullElevationFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*elevationFile);
	UE_LOG(LogTemp, Display, TEXT("Elevation data file should be located at %s"), *fullElevationFilePath);
	const char* fullElevationFilePathCstr = StringCast<ANSICHAR>(*fullElevationFilePath).Get();
	fprintf(stderr, "path %s\n", fullElevationFilePathCstr);
	ElevationData::LoadData(std::string(fullElevationFilePathCstr));
}

void AMapMeshManager::Tick(float DeltaTime){
	APawn* pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	if(pawn != NULL){
		FVector actorLocTmp = pawn->GetActorLocation();
		cameraLocation = SimpleVector3<double>(actorLocTmp.X, actorLocTmp.Y, actorLocTmp.Z);
		//fprintf(stderr, "loc %f %f %f\n", cameraLocation.x, cameraLocation.y, cameraLocation.z);
	}
	rootMapMesh->Update();
}
