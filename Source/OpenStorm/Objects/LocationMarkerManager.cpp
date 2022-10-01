#include "LocationMarkerManager.h"
#include "LocationMarker.h"
#include "RadarGameStateBase.h"
#include "../Radar/Globe.h"

ALocationMarkerManager::ALocationMarkerManager() {
	
}

void ALocationMarkerManager::BeginPlay() {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	
	callbackIds.push_back(globalState->RegisterEvent("GlobeUpdate",[this](std::string stringData, void* extraData){
		UpdateLocationMarkers();
	}));
	callbackIds.push_back(globalState->RegisterEvent("LocationMarkersUpdate",[this](std::string stringData, void* extraData){
		UpdateLocationMarkers();
	}));

	UpdateLocationMarkers();
}

void ALocationMarkerManager::EndPlay(const EEndPlayReason::Type endPlayReason) {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	// unregister all events
	for(auto id : callbackIds){
		globalState->UnregisterEvent(id);
	}
}

void ALocationMarkerManager::UpdateLocationMarkers() {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	for (auto marker : locationMarkerObjects) {
		marker->Destroy();
	}
	locationMarkerObjects.clear();
	for (auto waypoint : globalState->locationMarkers) {
		if (waypoint.enabled) {
			auto vector = globalState->globe->GetPointScaledDegrees(waypoint.latitude, waypoint.longitude, waypoint.altitude);
			ALocationMarker* marker = GetWorld()->SpawnActor<ALocationMarker>(ALocationMarker::StaticClass());
			marker->SetActorLocation(FVector(vector.x, vector.y, vector.z));
			marker->SetText(waypoint.name);
			locationMarkerObjects.push_back(marker);
		}
	}
}
