#include "LocationMarkerManager.h"
#include "LocationMarker.h"
#include "Engine/World.h"
#include "../Objects/RadarGameStateBase.h"
#include "Camera/CameraComponent.h"
#include "../Radar/Globe.h"
#include "../Radar/NexradSites/NexradSites.h"
#include "../Application/GlobalState.h"
#include <cmath>

ALocationMarkerManager::ALocationMarkerManager() {
	
}

void ALocationMarkerManager::BeginPlay() {
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		
		callbackIds.push_back(globalState->RegisterEvent("GlobeUpdate",[this](std::string stringData, void* extraData){
			UpdateMarkerLocations();
		}));
		callbackIds.push_back(globalState->RegisterEvent("LocationMarkersUpdate",[this](std::string stringData, void* extraData){
			UpdateWaypointMarkers();
		}));

		UpdateWaypointMarkers();
		UpdateMarkerLocations();
	}
	PrimaryActorTick.bCanEverTick = gameMode != NULL;
	Super::BeginPlay();
}

void ALocationMarkerManager::EndPlay(const EEndPlayReason::Type endPlayReason) {
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		// unregister all events
		for(auto id : callbackIds){
			globalState->UnregisterEvent(id);
		}
	}
	Super::EndPlay(endPlayReason);
}

void ALocationMarkerManager::Tick(float DeltaTime) {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	auto camera = Cast<UCameraComponent>(GetWorld()->GetFirstPlayerController()->GetPawn()->GetComponentByClass(UCameraComponent::StaticClass()));
	if (camera != NULL) {
		FRotator rotation = camera->GetComponentRotation();
		FVector camPos = camera->GetComponentLocation();
		for (auto marker : locationMarkerObjects) {
			
		}
		for (auto iterator = locationMarkerObjects.cbegin(); iterator != locationMarkerObjects.cend();){
			bool doDelete = false;
			// always face camera
			iterator->second->SetActorRotation(rotation);
			
			float maxDistance = iterator->second->maxDistance;
			if(maxDistance > 0.0f){
				// delete if too far away
				if(maxDistance < FVector::Distance(iterator->second->GetActorLocation(), camPos)){
					doDelete = true;
				}
			}
			
			if(!globalState->enableSiteMarkers && iterator->second->markerType == ALocationMarker::MarkerTypeRadarSite){
				doDelete = true;
			}
			
			if(doDelete){
				iterator->second->Destroy();
				locationMarkerObjects.erase(iterator++);
			}else{
				iterator++;
			}
		}
		if(globalState->enableSiteMarkers){
			AddSiteMarkers();
		}
	}
}

void ALocationMarkerManager::UpdateWaypointMarkers() {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	// loop over map and remove waypoints
	for (auto iterator = locationMarkerObjects.cbegin(); iterator != locationMarkerObjects.cend();){
		if(iterator->second->markerType == ALocationMarker::MarkerTypeWaypoint || ALocationMarker::MarkerTypeRadarSite){
			iterator->second->Destroy();
			locationMarkerObjects.erase(iterator++);
		}else{
			iterator++;
		}
	}
	// add all location markers
	for (size_t i = 0; i < globalState->locationMarkers.size(); i ++) {
		GlobalState::Waypoint* waypoint = &globalState->locationMarkers[i];
		if (waypoint->enabled) {
			ALocationMarker* marker = GetWorld()->SpawnActor<ALocationMarker>(ALocationMarker::StaticClass());
			marker->markerType = ALocationMarker::MarkerTypeWaypoint;
			marker->latitude = waypoint->latitude;
			marker->longitude = waypoint->longitude;
			marker->altitude = waypoint->altitude;
			auto vector = globalState->globe->GetPointScaledDegrees(marker->latitude, marker->longitude, marker->altitude);
			marker->SetActorLocation(FVector(vector.x, vector.y, vector.z));
			marker->SetText(waypoint->name);
			// set color while converting to linear color space
			marker->SetColor(FVector(std::pow(waypoint->colorR / 255.0f, 2.2f), std::pow(waypoint->colorG / 255.0f, 2.2f), std::pow(waypoint->colorB / 255.0f, 2.2f)));
			locationMarkerObjects["waypoint-" + std::to_string(i)] = marker;
		}
	}
	if(globalState->enableSiteMarkers){
		AddSiteMarkers();
	}
}

void ALocationMarkerManager::AddSiteMarkers() {
	auto camera = Cast<UCameraComponent>(GetWorld()->GetFirstPlayerController()->GetPawn()->GetComponentByClass(UCameraComponent::StaticClass()));
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	if (camera != NULL) {
		FVector camPos = camera->GetComponentLocation();
		// this is O(n) on the main thread every tick but there are not enough radars in the world to justify otherwise
		for(size_t i = 0; i < NexradSites::numberOfSites; i++){
			NexradSites::Site* site = &NexradSites::sites[i];
			auto vector = globalState->globe->GetPointScaledDegrees(site->latitude, site->longitude, site->altitude);
			FVector vectorF = FVector(vector.x, vector.y, vector.z);
			std::string name = "site-" + std::string(site->name);
			// if close enough to camera add to world
			if(maxSiteMarkerDistance > FVector::Distance(vectorF, camPos) && locationMarkerObjects.count(name) == 0){
				ALocationMarker* marker = GetWorld()->SpawnActor<ALocationMarker>(ALocationMarker::StaticClass());
				marker->markerType = ALocationMarker::MarkerTypeRadarSite;
				marker->data = site->name;
				marker->maxDistance = maxSiteMarkerDistance;
				marker->latitude = site->latitude;
				marker->longitude = site->longitude;
				marker->altitude = site->altitude;
				marker->SetActorLocation(vectorF);
				marker->SetText(site->name);
				marker->SetColor(FVector(std::pow(globalState->siteMarkerColorR / 255.0f, 2.2f), std::pow(globalState->siteMarkerColorG / 255.0f, 2.2f), std::pow(globalState->siteMarkerColorB / 255.0f, 2.2f)));
				marker->EnableCollision();
				locationMarkerObjects[name] = marker;
			}
		}
	}
}

void ALocationMarkerManager::UpdateMarkerLocations() {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	// set locations of all markers
	for (auto marker : locationMarkerObjects) {
		auto vector = globalState->globe->GetPointScaledDegrees(marker.second->latitude, marker.second->longitude, marker.second->altitude);
		marker.second->SetActorLocation(FVector(vector.x, vector.y, vector.z));
	}
}
