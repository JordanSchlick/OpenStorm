#include "SettingsSaver.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "../Objects/RadarGameStateBase.h"
#include "../Radar/Globe.h"


ASettingsSaver::ASettingsSaver() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ASettingsSaver::BeginPlay(){
	Super::BeginPlay();
	settingsFile = FPaths::Combine(FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("OpenStorm")), TEXT("settings.json"));
	locationMarkersFile = FPaths::Combine(FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("OpenStorm")), TEXT("location-markers.json"));

	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		callbackIds.push_back(globalState->RegisterEvent("LocationMarkersUpdate", [this](std::string stringData, void* extraData) {
			saveLocationMarkersCountdown = 5;
		}));
		LoadLocationMarkers();
	}
}

void ASettingsSaver::EndPlay(const EEndPlayReason::Type endPlayReason) {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		// unregister all events
		for(auto id : callbackIds){
			globalState->UnregisterEvent(id);
		}
		SaveLocationMarkers();
	}
	Super::EndPlay(endPlayReason);
}

void ASettingsSaver::Tick(float deltaTime) {
	if(saveLocationMarkersCountdown > 0){
		saveLocationMarkersCountdown -= deltaTime;
		if(saveLocationMarkersCountdown <= 0){
			// save when countdown hits zero
			SaveLocationMarkers();
		}
	}
}

TSharedPtr<FJsonObject> ASettingsSaver::LoadJson(FString filename){
	bool success = false;
	FString dataString;
	success = FFileHelper::LoadFileToString(dataString, *filename);
	TSharedPtr<FJsonObject> jsonObject;
	if(success){
		success = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(dataString), jsonObject);
		if(success){
			return jsonObject;
		}
	}
	success = FFileHelper::LoadFileToString(dataString, *(filename + TEXT(".old")));
	if(success){
		success = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(dataString), jsonObject);
		if(success){
			return jsonObject;
		}
	}
	jsonObject = MakeShared<FJsonObject>();
	return jsonObject;
}

bool ASettingsSaver::SaveJson(FString filename, TSharedPtr<FJsonObject> jsonObject){
	bool success = false;
	FString dataString;
	success = FJsonSerializer::Serialize(jsonObject.ToSharedRef(), TJsonWriterFactory<>::Create(&dataString, 0));
	if(success){
		//IFileManager::Get().Move(*(filename + TEXT(".old")), *filename, true);
		success = FFileHelper::SaveStringToFile(dataString, *filename);
		return success;
	}
	// success = FFileHelper::LoadFileToString(dataString, *filename);
	// if(success){
	// 	success = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(dataString), jsonObject);
	// 	return jsonObject;
	// }
	return false;
}

void ASettingsSaver::LoadLocationMarkers() {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		fprintf(stderr, "Loading waypoints\n");
		TSharedPtr<FJsonObject> jsonObject = LoadJson(locationMarkersFile);
		// get existing markers
		const TArray<TSharedPtr<FJsonValue>>* markersOldPtr;
		if (jsonObject->TryGetArrayField(TEXT("markers"), markersOldPtr)) {
			fprintf(stderr, "Loading waypoints %i\n", markersOldPtr->Num());
			for (int id = 0; id < markersOldPtr->Num(); id++) {
				const TSharedPtr<FJsonObject>* markerObjectPtr;
				(*markersOldPtr)[id]->TryGetObject(markerObjectPtr);
				FJsonObject* markerObject = markerObjectPtr->Get();
				GlobalState::Waypoint waypoint = {};
				FString name;
				if (markerObject->TryGetStringField(TEXT("name"), name)) {
					waypoint.name = std::string(StringCast<ANSICHAR>(*name).Get());
				}
				markerObject->TryGetNumberField(TEXT("latitude"), waypoint.latitude);
				markerObject->TryGetNumberField(TEXT("longitude"), waypoint.longitude);
				markerObject->TryGetNumberField(TEXT("altitude"), waypoint.altitude);
				markerObject->TryGetBoolField(TEXT("enabled"), waypoint.enabled);
				if (id < globalState->locationMarkers.size()) {
					// overite existing entry
					globalState->locationMarkers[id] = waypoint;
				} else {
					// create new entry
					globalState->locationMarkers.push_back(waypoint);
				}
			}
		}
	}
}

void ASettingsSaver::SaveLocationMarkers() {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		fprintf(stderr, "Saving waypoints\n");
		TSharedPtr<FJsonObject> jsonObject = LoadJson(locationMarkersFile);
		TArray<TSharedPtr<FJsonValue>> markers;
		// get existing markers to avoid accidently clearing everything
		const TArray<TSharedPtr<FJsonValue>>* markersOldPtr;
		if(jsonObject->TryGetArrayField(TEXT("markers"), markersOldPtr)){
			markers.Append(*markersOldPtr);
		}
		for (int id = 0; id < globalState->locationMarkers.size(); id++) {
			GlobalState::Waypoint& waypoint = globalState->locationMarkers[id];
			TSharedPtr<FJsonObject> markerObject = MakeShared<FJsonObject>();
			markerObject->SetStringField(TEXT("name"), FString(waypoint.name.c_str()));
			markerObject->SetNumberField(TEXT("latitude"), waypoint.latitude);
			markerObject->SetNumberField(TEXT("longitude"), waypoint.longitude);
			markerObject->SetNumberField(TEXT("altitude"), waypoint.altitude);
			markerObject->SetBoolField(TEXT("enabled"), waypoint.enabled);
			if (id < markers.Num()) {
				// overite existing entry
				markers[id] = MakeShared<FJsonValueObject>(markerObject);
			} else {
				// create new entry
				markers.Emplace(MakeShared<FJsonValueObject>(markerObject));
			}
			
		}
		jsonObject->SetArrayField(TEXT("markers"), markers);
		SaveJson(locationMarkersFile, jsonObject);
	}
}
