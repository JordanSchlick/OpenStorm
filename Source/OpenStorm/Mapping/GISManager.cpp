#define _USE_MATH_DEFINES
#include "GISManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "GISPolyline.h"
#include "Data/ElevationData.h"
#include "Data/ShapeFile.h"
#include "../Objects/RadarGameStateBase.h"
// #include "../Objects/RadarViewPawn.h"
#include "../Application/GlobalState.h"
#include "../Radar/Globe.h"
#include "../Radar/AsyncTask.h"
#include "../EngineHelpers/StringUtils.h"

#include <cmath>
#include <mutex>

#define M_PI       3.14159265358979323846




class GISLoaderTask : public AsyncTaskRunner{
public:
	
	SimpleVector3<double> cameraLocation;
	// all objects
	std::vector<GISObject> objects;
	// lock for objectsToAdd and objectsToRemove
	std::mutex changeQueueLock;
	// indexes of objects to add
	std::vector<uint64_t> objectsToAdd;
	// indexes of objects to remove
	std::vector<uint64_t> objectsToRemove;
	// if the results are ready to be consumed by the main thread
	bool ready = false;
	// if the shape files have been loaded
	bool loaded = false;
	
	virtual void Task(){
		if(!loaded){
			ReadShapeFile("J:/datasets/mapping/derived/final/non-existant.shp", &objects);
			// ReadShapeFile("J:/datasets/mapping/derived/final/motorways.shp", &objects);
			//ReadShapeFile("J:/datasets/mapping/derived/final/boundaries-counties.shp", &objects);
			ReadShapeFile("J:/datasets/mapping/derived/final/boundaries-states.shp", &objects);
			ReadShapeFile("J:/datasets/mapping/derived/final/boundaries-countries.shp", &objects);
			// ReadShapeFile("J:/datasets/mapping/derived/final/russia.shp", &objects);
			loaded = true;
		}
		Globe defaultGlobe;
		// get position on un-oriented globe in meters
		SimpleVector3<float> cameraPosition = SimpleVector3<float>(defaultGlobe.GetPoint(cameraLocation));
		for(size_t i = 0; i < objects.size(); i++){
			GISObject* object = &objects[i];
			float distance = cameraPosition.Distance(object->location);
			bool shouldShow = distance < 400000 || i < 100000;
			if(shouldShow != object->shown){
				object->shown = shouldShow;
				if(shouldShow){
					changeQueueLock.lock();
					objectsToAdd.push_back(i);
					changeQueueLock.unlock();
				}else{
					changeQueueLock.lock();
					objectsToRemove.push_back(i);
					changeQueueLock.unlock();
				}
			}
		}
		fprintf(stderr, "Loaded %i GIS objects\n", (int)objects.size());
	}
	
	~GISLoaderTask(){
		for(size_t i = 0; i < objects.size(); i++){
			objects[i].Delete();
		}
		
	}
};


AGISManager::AGISManager(){
	
}

AGISManager::~AGISManager(){
	
}

void AGISManager::BeginPlay(){
	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		callbackIds.push_back(globalState->RegisterEvent("GlobeUpdate", [this, globalState](std::string stringData, void* extraData){
			UpdatePositionsFromGlobe();
		}));
		callbackIds.push_back(globalState->RegisterEvent("CameraMove", [this, globalState](std::string stringData, void* extraData){
			cameraLocation = SimpleVector3<double>(*(SimpleVector3<float>*)extraData);
			needsRecalculation = true;
		}));
	}else{
		EnableMap();
	}
	
}


void AGISManager::EndPlay(const EEndPlayReason::Type endPlayReason){
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

void AGISManager::Tick(float DeltaTime){
	if(preTicks > 0){
		preTicks--;
		return;
	}
	UWorld* world = GetWorld();
	ARadarGameStateBase* gameMode = world->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		if(globalState->enableMap && globalState->developmentMode && !enabled){
			EnableMap();
		}
		if(!globalState->enableMap && enabled){
			DisableMap();
		}
		if(enabled){
			if(globalState->mapBrightness != mapBrightness){
				mapBrightness = globalState->mapBrightness;
			}
			if(loaderTask != NULL){
				
			}
		}
	}
	if(enabled && loaderTask != NULL){
		if(needsRecalculation && !loaderTask->running){
			loaderTask->Start();
			needsRecalculation = false;
		}
		std::vector<uint64_t> objectsToAdd;
		std::vector<uint64_t> objectsToRemove;
		loaderTask->changeQueueLock.lock();
		objectsToAdd.swap(loaderTask->objectsToAdd);
		objectsToRemove.swap(loaderTask->objectsToRemove);
		loaderTask->changeQueueLock.unlock();
		for(uint64_t id : objectsToRemove){
			polylines[id]->Destroy();
			polylines.erase(id);
		}
		fprintf(stderr, "%i ", (int)objectsToAdd.size());
		for(uint64_t id : objectsToAdd){
			AGISPolyline* polyline = world->SpawnActor<AGISPolyline>(AGISPolyline::StaticClass());
			polyline->DisplayObject(&loaderTask->objects[id]);
			polyline->PositionObject(globe);
			polylines[id] = polyline;
		}
	}
}


void AGISManager::UpdatePositionsFromGlobe(){
	if(!enabled){
		return;
	}
	for(auto polyline : polylines){
		polyline.second->PositionObject(globe);
	}
}



void AGISManager::EnableMap(){
	if(enabled){
		return;
	}
	enabled = true;
	
	ElevationData::StartUsing();
	
	if(loaderTask == NULL){
		loaderTask = new GISLoaderTask();
		needsRecalculation = true;
	}
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		globe = globalState->globe;
		
		UpdatePositionsFromGlobe();
	}else{
		static Globe defaultGlobe = {};
		globe = &defaultGlobe;
	}
}

void AGISManager::DisableMap(){
	if(!enabled){
		return;
	}
	enabled = false;
	ElevationData::StopUsing();
	for(auto polyline : polylines){
		polyline.second->Destroy();
	}
	
	if(loaderTask != NULL){
		loaderTask->Delete();
		loaderTask = NULL;
	}
}
