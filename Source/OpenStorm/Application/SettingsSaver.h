#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "SettingsSaver.generated.h"

UCLASS()
class OPENSTORM_API ASettingsSaver : public AActor
{
	GENERATED_BODY()

public:
	ASettingsSaver();
	// ~ASettingsSaver();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;
	
	
	// load JSON from disk
	TSharedPtr<FJsonObject> LoadJson(FString filename);
	
	// save JSON to disk
	bool SaveJson(FString filename, TSharedPtr<FJsonObject> jsonObject);
	
	// load settings from disk into global state
	void LoadSettings();
	
	// save non-default settings from global state onto disk
	void SaveSettings();
	
	// load location markers from disk into global state
	void LoadLocationMarkers();
	
	// save location markers from global state onto disk
	void SaveLocationMarkers();
	
	FString settingsFile;
	FString locationMarkersFile;
	double saveLocationMarkersCountdown = -1;
	std::vector<uint64_t> callbackIds = {};
};