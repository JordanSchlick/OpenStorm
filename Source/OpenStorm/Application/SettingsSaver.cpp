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
#include "../EngineHelpers/StringUtils.h"
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
		callbackIds.push_back(globalState->RegisterEvent("ResetBasicSettings", [this](std::string stringData, void* extraData) {
			ResetBasicSettings();
		}));
		callbackIds.push_back(globalState->RegisterEvent("ResetAllSettings", [this](std::string stringData, void* extraData) {
			ResetAllSettings();
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
		// test if existing json file is valid
		bool readSuccess = false;
		FString oldDataString;
		readSuccess = FFileHelper::LoadFileToString(oldDataString, *filename);
		TSharedPtr<FJsonObject> jsonObjectOld;
		if(readSuccess){
			readSuccess = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(oldDataString), jsonObjectOld);
		}
		if(readSuccess && jsonObjectOld->Values.Num() > 0){
			// move existing valid json file
			IFileManager::Get().Move(*(filename + TEXT(".old")), *filename, true);
		}
		
		success = FFileHelper::SaveStringToFile(dataString, *filename);
		return success;
	}
	return false;
}


#define QUOTE(seq) ""#seq""

#define LOAD_MACRO_FLOAT(NAME) if(jsonObject->TryGetNumberField(TEXT(QUOTE(NAME)), tmpFloat)){ \
	globalState->NAME = (decltype(globalState->NAME))tmpFloat; \
}

#define LOAD_MACRO_BOOL(NAME) if(jsonObject->TryGetBoolField(TEXT(QUOTE(NAME)), tmpBool)){ \
	globalState->NAME = tmpBool; \
}

#define LOAD_MACRO_STRING(NAME) if(jsonObject->TryGetStringField(TEXT(QUOTE(NAME)), tmpString)){ \
	globalState->NAME = StringUtils::FStringToSTDString(tmpString); \
}

void ASettingsSaver::LoadSettings() {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		// fprintf(stderr, "Loading waypoints\n");
		TSharedPtr<FJsonObject> jsonObject = LoadJson(settingsFile);
		float tmpFloat;
		bool tmpBool;
		FString tmpString;
		// Main
		// --Radar--
		LOAD_MACRO_FLOAT(cutoff);
		LOAD_MACRO_FLOAT(opacityMultiplier);
		LOAD_MACRO_FLOAT(verticalScale);
		LOAD_MACRO_BOOL(spatialInterpolation);
		LOAD_MACRO_BOOL(temporalInterpolation);
		LOAD_MACRO_FLOAT(viewMode);
		LOAD_MACRO_FLOAT(sliceMode);
		LOAD_MACRO_FLOAT(sliceAltitude);
		LOAD_MACRO_FLOAT(sliceAngle);
		LOAD_MACRO_FLOAT(sliceVerticalLocationX);
		LOAD_MACRO_FLOAT(sliceVerticalLocationY);
		LOAD_MACRO_FLOAT(sliceVerticalRotation);
		LOAD_MACRO_BOOL(sliceVolumetric);
		// --Movement--
		LOAD_MACRO_FLOAT(moveSpeed);
		LOAD_MACRO_FLOAT(rotateSpeed);
		// --Animation--
		LOAD_MACRO_FLOAT(animateSpeed);
		LOAD_MACRO_FLOAT(animateLoopMode);
		LOAD_MACRO_FLOAT(animateCutoffSpeed);
		// --Data--
		LOAD_MACRO_BOOL(pollData);
		LOAD_MACRO_FLOAT(downloadPollInterval);
		LOAD_MACRO_FLOAT(downloadPreviousCount);
		LOAD_MACRO_FLOAT(downloadDeleteAfter);
		LOAD_MACRO_STRING(downloadSiteId);
		LOAD_MACRO_STRING(downloadUrl);
		// --Map--
		LOAD_MACRO_BOOL(enableMap);
		LOAD_MACRO_BOOL(enableMapTiles);
		LOAD_MACRO_BOOL(enableMapGIS);
		LOAD_MACRO_FLOAT(mapBrightness);
		LOAD_MACRO_FLOAT(mapBrightnessGIS);
		LOAD_MACRO_BOOL(enableSiteMarkers);
		// Settings
		LOAD_MACRO_FLOAT(maxFPS);
		LOAD_MACRO_BOOL(vsync);
		LOAD_MACRO_FLOAT(quality);
		LOAD_MACRO_FLOAT(qualityCustomStepSize);
		LOAD_MACRO_BOOL(enableFuzz);
		LOAD_MACRO_BOOL(temporalAntiAliasing);
		LOAD_MACRO_BOOL(discordPresence);
		
		globalState->EmitEvent("UpdateVolumeParameters");
	}
}

// these macros only save a value if it is not the default to allow changing default values for all installations

#define SAVE_MACRO_FLOAT(NAME) if(globalState->NAME != globalState->defaults->NAME){ \
	jsonObject->SetNumberField(TEXT(QUOTE(NAME)), (double)globalState->NAME); \
} else { \
	jsonObject->RemoveField(TEXT(QUOTE(NAME))); \
}

#define SAVE_MACRO_BOOL(NAME) if(globalState->NAME != globalState->defaults->NAME){ \
	jsonObject->SetBoolField(TEXT(QUOTE(NAME)), globalState->NAME); \
} else { \
	jsonObject->RemoveField(TEXT(QUOTE(NAME))); \
}

#define SAVE_MACRO_STRING(NAME) if(globalState->NAME != globalState->defaults->NAME){ \
	jsonObject->SetStringField(TEXT(QUOTE(NAME)), StringUtils::STDStringToFString(globalState->NAME)); \
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
		if(globalState->opacityMultiplier != globalState->defaults->opacityMultiplier){
			// prevent users from accidentally hiding everything across restarts
			jsonObject->SetNumberField(TEXT("opacityMultiplier"), std::max(globalState->opacityMultiplier, 0.01f));
		} else {
			jsonObject->RemoveField(TEXT("opacityMultiplier"));
		}
		SAVE_MACRO_FLOAT(verticalScale);
		SAVE_MACRO_BOOL(spatialInterpolation);
		SAVE_MACRO_BOOL(temporalInterpolation);
		SAVE_MACRO_FLOAT(viewMode);
		SAVE_MACRO_FLOAT(sliceMode);
		SAVE_MACRO_FLOAT(sliceAltitude);
		SAVE_MACRO_FLOAT(sliceAngle);
		SAVE_MACRO_FLOAT(sliceVerticalLocationX);
		SAVE_MACRO_FLOAT(sliceVerticalLocationY);
		SAVE_MACRO_FLOAT(sliceVerticalRotation);
		SAVE_MACRO_BOOL(sliceVolumetric);
		// --Movement--
		SAVE_MACRO_FLOAT(moveSpeed);
		SAVE_MACRO_FLOAT(rotateSpeed);
		// --Animation--
		SAVE_MACRO_FLOAT(animateSpeed);
		SAVE_MACRO_FLOAT(animateLoopMode);
		SAVE_MACRO_FLOAT(animateCutoffSpeed);
		// --Data--
		SAVE_MACRO_BOOL(pollData);
		SAVE_MACRO_FLOAT(downloadPollInterval);
		SAVE_MACRO_FLOAT(downloadPreviousCount);
		SAVE_MACRO_FLOAT(downloadDeleteAfter);
		SAVE_MACRO_STRING(downloadSiteId);
		SAVE_MACRO_STRING(downloadUrl);
		// --Map--
		SAVE_MACRO_BOOL(enableMap);
		SAVE_MACRO_BOOL(enableMapTiles);
		SAVE_MACRO_BOOL(enableMapGIS);
		SAVE_MACRO_FLOAT(mapBrightness);
		SAVE_MACRO_FLOAT(mapBrightnessGIS);
		SAVE_MACRO_BOOL(enableSiteMarkers);
		// Settings
		SAVE_MACRO_FLOAT(maxFPS);
		SAVE_MACRO_BOOL(vsync);
		if(globalState->quality != 3){
			// only save if not on gpu melter
			SAVE_MACRO_FLOAT(quality);
		}
		if(globalState->qualityCustomStepSize >= 0.3){
			// don't alow saving the equivelent of gpu melter
			SAVE_MACRO_FLOAT(qualityCustomStepSize);
		}
		SAVE_MACRO_BOOL(enableFuzz);
		SAVE_MACRO_BOOL(temporalAntiAliasing);
		SAVE_MACRO_BOOL(discordPresence);



		SaveJson(settingsFile, jsonObject);
	}
}

#define RESET_MACRO(NAME) globalState->NAME = globalState->defaults->NAME;

void ASettingsSaver::ResetBasicSettings() {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		TSharedPtr<FJsonObject> jsonObject = LoadJson(settingsFile);
		// Main
		// --Radar--
		RESET_MACRO(volumeType);
		RESET_MACRO(cutoff);
		RESET_MACRO(opacityMultiplier);
		RESET_MACRO(verticalScale);
		RESET_MACRO(spatialInterpolation);
		RESET_MACRO(temporalInterpolation);
		RESET_MACRO(viewMode);
		RESET_MACRO(sliceMode);
		RESET_MACRO(sliceAltitude);
		RESET_MACRO(sliceAngle);
		RESET_MACRO(sliceVerticalLocationX);
		RESET_MACRO(sliceVerticalLocationY);
		RESET_MACRO(sliceVerticalRotation);
		RESET_MACRO(sliceVolumetric);
		// --Movement--
		if(globalState->moveSpeed <= 10){
			RESET_MACRO(moveSpeed);
		}
		if(globalState->rotateSpeed <= 10){
			RESET_MACRO(rotateSpeed);
		}
		// --Animation--
		RESET_MACRO(animate);
		RESET_MACRO(animateCutoff);
		// RESET_MACRO(animateSpeed);
		// RESET_MACRO(animateCutoffSpeed);
		// --Data--
		// RESET_MACRO(pollData);
		// --Map--
		// RESET_MACRO(enableMap);
		// RESET_MACRO(mapBrightness);
		// Settings
		// RESET_MACRO(maxFPS);
		// RESET_MACRO(vsync);
		// RESET_MACRO(quality);
		// RESET_MACRO(qualityCustomStepSize);
		RESET_MACRO(enableFuzz);
		// RESET_MACRO(temporalAntiAliasing);
		// --Joke--
		RESET_MACRO(audioControlledCutoff);
		RESET_MACRO(audioControlledHeight);
		RESET_MACRO(audioControlledOpacity);
		RESET_MACRO(audioControlMultiplier);



		globalState->EmitEvent("UpdateVolumeParameters");
		globalState->EmitEvent("UpdateEngineSettings");
		globalState->EmitEvent("ChangeProduct", "", &globalState->volumeType);
	}
}

void ASettingsSaver::ResetAllSettings() {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		TSharedPtr<FJsonObject> jsonObject = LoadJson(settingsFile);
		// Main
		// --Radar--
		RESET_MACRO(volumeType);
		RESET_MACRO(cutoff);
		RESET_MACRO(opacityMultiplier);
		RESET_MACRO(verticalScale);
		RESET_MACRO(spatialInterpolation);
		RESET_MACRO(temporalInterpolation);
		RESET_MACRO(viewMode);
		RESET_MACRO(sliceMode);
		RESET_MACRO(sliceAltitude);
		RESET_MACRO(sliceAngle);
		RESET_MACRO(sliceVerticalLocationX);
		RESET_MACRO(sliceVerticalLocationY);
		RESET_MACRO(sliceVerticalRotation);
		RESET_MACRO(sliceVolumetric);
		// --Movement--
		RESET_MACRO(moveSpeed);
		RESET_MACRO(rotateSpeed);
		// --Animation--
		RESET_MACRO(animate);
		RESET_MACRO(animateLoopMode);
		RESET_MACRO(animateCutoff);
		RESET_MACRO(animateSpeed);
		RESET_MACRO(animateCutoffSpeed);
		// --Data--
		RESET_MACRO(pollData);
		RESET_MACRO(downloadData);
		RESET_MACRO(downloadPollInterval);
		RESET_MACRO(downloadPreviousCount);
		RESET_MACRO(downloadDeleteAfter);
		RESET_MACRO(downloadSiteId);
		RESET_MACRO(downloadUrl);
		// --Map--
		RESET_MACRO(enableMap);
		RESET_MACRO(enableMapTiles);
		RESET_MACRO(enableMapGIS);
		RESET_MACRO(mapBrightness);
		RESET_MACRO(mapBrightnessGIS);
		RESET_MACRO(enableSiteMarkers);
		// Settings
		RESET_MACRO(maxFPS);
		RESET_MACRO(vsync);
		RESET_MACRO(quality);
		RESET_MACRO(qualityCustomStepSize);
		RESET_MACRO(enableFuzz);
		RESET_MACRO(temporalAntiAliasing);
		RESET_MACRO(discordPresence);
		// --Joke--
		RESET_MACRO(audioControlledCutoff);
		RESET_MACRO(audioControlledHeight);
		RESET_MACRO(audioControlledOpacity);
		RESET_MACRO(audioControlMultiplier);



		globalState->EmitEvent("UpdateVolumeParameters");
		globalState->EmitEvent("UpdateEngineSettings");
		globalState->EmitEvent("ChangeProduct", "", &globalState->volumeType);
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
					waypoint.name = StringUtils::FStringToSTDString(name);
				}
				markerObject->TryGetNumberField(TEXT("latitude"), waypoint.latitude);
				markerObject->TryGetNumberField(TEXT("longitude"), waypoint.longitude);
				markerObject->TryGetNumberField(TEXT("altitude"), waypoint.altitude);
				markerObject->TryGetNumberField(TEXT("colorR"), waypoint.colorR);
				markerObject->TryGetNumberField(TEXT("colorG"), waypoint.colorG);
				markerObject->TryGetNumberField(TEXT("colorB"), waypoint.colorB);
				markerObject->TryGetNumberField(TEXT("size"), waypoint.size);
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
		// // get existing markers to avoid accidentally clearing everything
		// const TArray<TSharedPtr<FJsonValue>>* markersOldPtr;
		// if(jsonObject->TryGetArrayField(TEXT("markers"), markersOldPtr)){
		// 	markers.Append(*markersOldPtr);
		// }
		for (int id = 0; id < globalState->locationMarkers.size(); id++) {
			GlobalState::Waypoint& waypoint = globalState->locationMarkers[id];
			TSharedPtr<FJsonObject> markerObject = MakeShared<FJsonObject>();
			markerObject->SetStringField(TEXT("name"), StringUtils::STDStringToFString(waypoint.name.c_str()));
			markerObject->SetNumberField(TEXT("latitude"), waypoint.latitude);
			markerObject->SetNumberField(TEXT("longitude"), waypoint.longitude);
			markerObject->SetNumberField(TEXT("altitude"), waypoint.altitude);
			markerObject->SetNumberField(TEXT("colorR"), waypoint.colorR);
			markerObject->SetNumberField(TEXT("colorG"), waypoint.colorG);
			markerObject->SetNumberField(TEXT("colorB"), waypoint.colorB);
			markerObject->SetNumberField(TEXT("size"), waypoint.size);
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
