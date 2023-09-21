#define _USE_MATH_DEFINES
#include "GISManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "Data/ElevationData.h"
#include "../Objects/RadarGameStateBase.h"
// #include "../Objects/RadarViewPawn.h"
#include "../Application/GlobalState.h"
#include "../Radar/Globe.h"
#include "../EngineHelpers/StringUtils.h"

#include <cmath>

#define M_PI       3.14159265358979323846

AGISManager::AGISManager(){
	globe = new Globe();
}

AGISManager::~AGISManager(){
	delete globe;
}

void AGISManager::BeginPlay(){
	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
	}else{
		EnableMap();
	}
	
}

void AGISManager::Tick(float DeltaTime){
	if(preTicks > 0){
		preTicks--;
		return;
	}
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		if(globalState->enableMap && !enabled){
			EnableMap();
		}
		if(!globalState->enableMap && enabled){
			DisableMap();
		}
		if(enabled){
			if(globalState->mapBrightness != mapBrightness){
				mapBrightness = globalState->mapBrightness;
			}
		}
	}
	if(enabled){
		
	}
}


void AGISManager::UpdatePositionsFromGlobe(){
	SimpleVector3 center = SimpleVector3<>(globe->center);
	center.Multiply(globe->scale);
	//mapMesh->UpdatePosition(center, SimpleVector3<>(globe->rotationAroundX, 0, globe->rotationAroundPolls));
}



void AGISManager::EnableMap(){
	if(enabled){
		return;
	}
	enabled = true;
	
	
	
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		globe->scale = globalState->globe->scale;
		callbackIds.push_back(globalState->RegisterEvent("GlobeUpdate", [this, globalState](std::string stringData, void* extraData){
			UpdatePositionsFromGlobe();
		}));
		callbackIds.push_back(globalState->RegisterEvent("CameraMove", [this, globalState](std::string stringData, void* extraData){
			cameraLocation = SimpleVector3<double>(*(SimpleVector3<float>*)extraData);
		}));
		UpdatePositionsFromGlobe();
	}
}

void AGISManager::DisableMap(){
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
}
