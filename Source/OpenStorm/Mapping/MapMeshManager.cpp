#define _USE_MATH_DEFINES
#include "MapMeshManager.h"
#include "MapMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "Data/ElevationData.h"
#include "Data/TileProvider.h"
#include "../Objects/RadarGameStateBase.h"
// #include "../Objects/RadarViewPawn.h"
#include "../Application/GlobalState.h"
#include "../Radar/Globe.h"
#include "../EngineHelpers/StringUtils.h"

#include <cmath>

#define M_PI       3.14159265358979323846

AMapMeshManager::AMapMeshManager(){
	globe = new Globe();
}

AMapMeshManager::~AMapMeshManager(){
	delete globe;
}

void UpdateMapMeshPositionFromGlobe(AMapMesh* mapMesh, Globe* globe){
	SimpleVector3 center = SimpleVector3<>(globe->center);
	center.Multiply(globe->scale);
	mapMesh->UpdatePosition(center, SimpleVector3<>(globe->rotationAroundX, 0, globe->rotationAroundPolls));
}

void AMapMeshManager::BeginPlay(){
	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		callbackIds.push_back(globalState->RegisterEvent("GlobeUpdate", [this, globalState](std::string stringData, void* extraData){
			if(!enabled){
				return;
			}
			UpdateMapMeshPositionFromGlobe(rootMapMesh, globalState->globe);
		}));
		callbackIds.push_back(globalState->RegisterEvent("CameraMove", [this, globalState](std::string stringData, void* extraData){
			cameraLocation = SimpleVector3<double>(*(SimpleVector3<float>*)extraData);
		}));
	}else{
		EnableMap();
	}
	
}

void AMapMeshManager::EndPlay(const EEndPlayReason::Type endPlayReason){
	Super::EndPlay(endPlayReason);
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		// unregister all events
		for(auto id : callbackIds){
			globalState->UnregisterEvent(id);
		}
		callbackIds.clear();
	}
}

void AMapMeshManager::Tick(float DeltaTime){
	if(preTicks > 0){
		preTicks--;
		return;
	}
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		if(globalState->enableMap && globalState->enableMapTiles && !enabled){
			EnableMap();
		}
		if(!(globalState->enableMap && globalState->enableMapTiles) && enabled){
			DisableMap();
		}
		if(enabled){
			if(globalState->mapBrightness != mapBrightness){
				mapBrightness = globalState->mapBrightness;
				rootMapMesh->UpdateParameters();
			}
		}
	}
	if(enabled){
		// APawn* pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
		// if(pawn != NULL){
		// 	FVector actorLocTmp = pawn->GetActorLocation();
		// 	ARadarViewPawn* radarPawn = dynamic_cast<ARadarViewPawn*>(pawn);
		// 	if(radarPawn){
		// 		actorLocTmp = radarPawn->camera->GetComponentLocation();
		// 	}
		// 	cameraLocation = SimpleVector3<double>(actorLocTmp.X, actorLocTmp.Y, actorLocTmp.Z);
		// 	//fprintf(stderr, "loc %f %f %f\n", cameraLocation.x, cameraLocation.y, cameraLocation.z);
		// }
		rootMapMesh->Update();
	}
}


void AMapMeshManager::EnableMap(){
	if(enabled){
		return;
	}
	enabled = true;
	
	ElevationData::StartUsing();
	
	std::string staticCacheLocation = StringUtils::GetRelativePath(TEXT("Content/Data/Map/Tiles/ImageryOnly/"));
	std::string staticCacheTarLocation = StringUtils::GetRelativePath(TEXT("Content/Data/Map/Tiles/ImageryOnly.tar"));
	std::string dynamicCacheLocation = StringUtils::GetUserPath(TEXT("Map/Tiles/USGSImageryOnly/"));
	fprintf(stderr, "path %s\n", staticCacheLocation.c_str());
	fprintf(stderr, "path %s\n", dynamicCacheLocation.c_str());
	tileProvider = new TileProvider("USGSImageryOnly", "https://basemap.nationalmap.gov/arcgis/rest/services/USGSImageryOnly/MapServer/tile/{z}/{y}/{x}", "image/jpeg", 10);
	tileProvider->SetCache(staticCacheLocation, dynamicCacheLocation);
	tileProvider->SetTarCache(staticCacheTarLocation);
	
	rootMapMesh = GetWorld()->SpawnActor<AMapMesh>(AMapMesh::StaticClass());
	rootMapMesh->SetBounds(
		0,
		0,
		2.968844, //M_PI,
		M_PI * 2
	);
	rootMapMesh->manager = this;
	rootMapMesh->globe = globe;
	rootMapMesh->UpdateParameters();
	rootMapMesh->LoadTile();
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		globe->scale = globalState->globe->scale;
		UpdateMapMeshPositionFromGlobe(rootMapMesh, globalState->globe);
	}
}

void AMapMeshManager::DisableMap(){
	if(!enabled){
		return;
	}
	enabled = false;
	ElevationData::StopUsing();
	if(rootMapMesh != NULL){
		rootMapMesh->Destroy();
		rootMapMesh = NULL;
	}
	if(tileProvider != NULL){
		delete tileProvider;
		tileProvider = NULL;
	}
}
