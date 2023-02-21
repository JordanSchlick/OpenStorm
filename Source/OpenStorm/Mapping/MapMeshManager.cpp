#define _USE_MATH_DEFINES
#include "MapMeshManager.h"
#include "MapMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Data/ElevationData.h"
#include "../Objects/RadarGameStateBase.h"
#include "../Application/GlobalState.h"
#include "../Radar/Globe.h"


#include <cmath>

#define M_PI       3.14159265358979323846

AMapMeshManager::AMapMeshManager(){
	globe = new Globe();
}

AMapMeshManager::~AMapMeshManager(){
	delete globe;
}

void AMapMeshManager::BeginPlay(){
	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
	}else{
		EnableMap();
	}
}

void AMapMeshManager::Tick(float DeltaTime){
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		if(globalState->enableMap && !enabled){
			EnableMap();
		}
		if(!globalState->enableMap && enabled){
			DisableMap();
		}
	}
	if(enabled){
		APawn* pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
		if(pawn != NULL){
			FVector actorLocTmp = pawn->GetActorLocation();
			cameraLocation = SimpleVector3<double>(actorLocTmp.X, actorLocTmp.Y, actorLocTmp.Z);
			//fprintf(stderr, "loc %f %f %f\n", cameraLocation.x, cameraLocation.y, cameraLocation.z);
		}
		rootMapMesh->Update();
	}
}


void UpdateMapMeshPositionFromGlobe(AMapMesh* mapMesh, Globe* globe){
	SimpleVector3 center = SimpleVector3<>(globe->center);
	center.Multiply(globe->scale);
	mapMesh->UpdatePosition(center, SimpleVector3<>(globe->rotationAroundX, 0, globe->rotationAroundPolls));
}

void AMapMeshManager::EnableMap(){
	if(enabled){
		return;
	}
	enabled = true;
	
	FString elevationFile =  FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Data/elevation.bin"));
	FString fullElevationFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*elevationFile);
	UE_LOG(LogTemp, Display, TEXT("Elevation data file should be located at %s"), *fullElevationFilePath);
	const char* fullElevationFilePathCstr = StringCast<ANSICHAR>(*fullElevationFilePath).Get();
	fprintf(stderr, "path %s\n", fullElevationFilePathCstr);
	ElevationData::LoadData(std::string(fullElevationFilePathCstr));
	
	rootMapMesh = GetWorld()->SpawnActor<AMapMesh>(AMapMesh::StaticClass());
	rootMapMesh->SetBounds(
		0,
		0,
		M_PI,
		M_PI * 2
	);
	rootMapMesh->manager = this;
	rootMapMesh->globe = globe;
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		globe->scale = globalState->globe->scale;
		callbackIds.push_back(globalState->RegisterEvent("GlobeUpdate", [this, globalState](std::string stringData, void* extraData){
			UpdateMapMeshPositionFromGlobe(rootMapMesh, globalState->globe);
		}));
		UpdateMapMeshPositionFromGlobe(rootMapMesh, globalState->globe);
	}
}

void AMapMeshManager::DisableMap(){
	if(!enabled){
		return;
	}
	enabled = false;
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		// unregister all events
		for(auto id : callbackIds){
			globalState->UnregisterEvent(id);
		}
	}
	
	callbackIds.clear();
	ElevationData::UnloadData();
	if(rootMapMesh != NULL){
		rootMapMesh->Destroy();
		rootMapMesh = NULL;
	}
}
