#include "SettingsSaver.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "../Objects/RadarGameStateBase.h"
#include "../Radar/Globe.h"
#include <algorithm>

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
		LoadSettings();
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
		SaveSettings();
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
		IFileManager::Get().Move(*(filename + TEXT(".old")), *filename, true);
		success = FFileHelper::SaveStringToFile(dataString, *filename);
		return success;
	}
	return false;
}


#define QUOTE(seq) ""#seq""

#define LOAD_MACRO_FLOAT(NAME) if(jsonObject->TryGetNumberField(TEXT(QUOTE(NAME)), tmpFloat)){ \
	globalState->NAME = tmpFloat; \
}

#define LOAD_MACRO_BOOL(NAME) if(jsonObject->TryGetBoolField(TEXT(QUOTE(NAME)), tmpBool)){ \
	globalState->NAME = tmpBool; \
}

void ASettingsSaver::LoadSettings() {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		// fprintf(stderr, "Loading waypoints\n");
		TSharedPtr<FJsonObject> jsonObject = LoadJson(settingsFile);
		float tmpFloat;
		bool tmpBool;
		// Main
		// --Radar--
		if (jsonObject->TryGetNumberField(TEXT(""), tmpFloat)) {
			globalState->animateSpeed = tmpFloat;
		}
		LOAD_MACRO_FLOAT(cutoff);
		LOAD_MACRO_FLOAT(opacityMultiplier);
		LOAD_MACRO_FLOAT(verticalScale);
		LOAD_MACRO_BOOL(spatialInterpolation);
		LOAD_MACRO_BOOL(temporalInterpolation);
		// --Movement--
		LOAD_MACRO_FLOAT(moveSpeed);
		LOAD_MACRO_FLOAT(rotateSpeed);
		// --Animation--
		LOAD_MACRO_FLOAT(animateSpeed);
		LOAD_MACRO_FLOAT(animateCutoffSpeed);
		// --Data--
		LOAD_MACRO_BOOL(pollData);
		// --Map--
		LOAD_MACRO_BOOL(enableMap);
		LOAD_MACRO_FLOAT(mapBrightness);
		// Settings
		LOAD_MACRO_FLOAT(maxFPS);
		LOAD_MACRO_BOOL(vsync);
		LOAD_MACRO_FLOAT(quality);
		LOAD_MACRO_FLOAT(qualityCustomStepSize);
		LOAD_MACRO_BOOL(enableFuzz);
		LOAD_MACRO_BOOL(temporalAntiAliasing);
		
		globalState->EmitEvent("UpdateVolumeParameters");
	}
}

#define SAVE_MACRO_FLOAT(NAME) if(globalState->NAME != globalState->defaults->NAME){ \
	jsonObject->SetNumberField(TEXT(QUOTE(NAME)), globalState->NAME); \
} else { \
	jsonObject->RemoveField(TEXT(QUOTE(NAME))); \
}

#define SAVE_MACRO_BOOL(NAME) if(globalState->NAME != globalState->defaults->NAME){ \
	jsonObject->SetBoolField(TEXT(QUOTE(NAME)), globalState->NAME); \
} else { \
	jsonObject->RemoveField(TEXT(QUOTE(NAME))); \
}

void ASettingsSaver::SaveSettings() {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		TSharedPtr<FJsonObject> jsonObject = LoadJson(settingsFile);
		// Main
		// --Radar--
		if(globalState->cutoff != globalState->defaults->cutoff){
			// prevent users from accidentally hiding everything across restarts
			jsonObject->SetNumberField(TEXT("cutoff"), std::min(globalState->cutoff, 0.5f));
		} else {
			jsonObject->RemoveField(TEXT("cutoff"));
		}
		SAVE_MACRO_FLOAT(opacityMultiplier);
		SAVE_MACRO_FLOAT(verticalScale);
		SAVE_MACRO_BOOL(spatialInterpolation);
		SAVE_MACRO_BOOL(temporalInterpolation);
		// --Movement--
		SAVE_MACRO_FLOAT(moveSpeed);
		SAVE_MACRO_FLOAT(rotateSpeed);
		// --Animation--
		SAVE_MACRO_FLOAT(animateSpeed);
		SAVE_MACRO_FLOAT(animateCutoffSpeed);
		// --Data--
		SAVE_MACRO_BOOL(pollData);
		// --Map--
		SAVE_MACRO_BOOL(enableMap);
		SAVE_MACRO_FLOAT(mapBrightness);
		// Settings
		SAVE_MACRO_FLOAT(maxFPS);
		SAVE_MACRO_BOOL(vsync);
		SAVE_MACRO_FLOAT(quality);
		SAVE_MACRO_FLOAT(qualityCustomStepSize);
		SAVE_MACRO_BOOL(enableFuzz);
		SAVE_MACRO_BOOL(temporalAntiAliasing);



		SaveJson(settingsFile, jsonObject);
	}
}

void ASettingsSaver::LoadLocationMarkers() {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		// fprintf(stderr, "Loading waypoints\n");
		TSharedPtr<FJsonObject> jsonObject = LoadJson(locationMarkersFile);
		// get existing markers
		const TArray<TSharedPtr<FJsonValue>>* markersOldPtr;
		if (jsonObject->TryGetArrayField(TEXT("markers"), markersOldPtr)) {
			//fprintf(stderr, "Loading waypoints %i\n", markersOldPtr->Num());
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
		// fprintf(stderr, "Saving waypoints\n");
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
