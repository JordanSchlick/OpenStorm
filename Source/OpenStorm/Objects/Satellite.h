#pragma once


#include "../Radar/Globe.h"

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "Satellite.generated.h"

// Basic object to test the Glob class by orbiting around it
UCLASS()
class ASatellite : public AActor{
	GENERATED_BODY()
	Globe globe;
	
	double latitude = 0;
	double longitude = 0;
	
	UPROPERTY(EditAnywhere);
		double globeUpLatitude = 0;
	UPROPERTY(EditAnywhere);
		double globeUpLongitude = 0;
	UPROPERTY(EditAnywhere);
		double globeCenterX = 0;
	UPROPERTY(EditAnywhere);
		double globeCenterY = 0;
	UPROPERTY(EditAnywhere);
		double globeCenterZ = 0;
	
	
	UPROPERTY(EditAnywhere);
		double startLatitude = 0;
	UPROPERTY(EditAnywhere);
		double startLongitude = 0;
	UPROPERTY(EditAnywhere);
		double altitude = 0;
	UPROPERTY(EditAnywhere);
		double speedLatitude = 0;
	UPROPERTY(EditAnywhere);
		double speedLongitude = 100;
	UPROPERTY(EditAnywhere);
		bool doReset = false;
	UPROPERTY(EditAnywhere);
		bool doSetGlobe = true;
	
	ASatellite();
	void Reset();
	void SetGlobe();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
};