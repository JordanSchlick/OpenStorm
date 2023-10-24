#pragma once

#include <string>

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/Object.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "LocationMarker.generated.h"

class UTextRenderComponent;

UCLASS()
class ALocationMarker : public AActor{
	GENERATED_BODY()
	
public:

	enum MarkerType {
		TYPE_WAYPOINT,
		TYPE_RADAR_SITE
	};
	
	MarkerType markerType = TYPE_WAYPOINT;
	
	double latitude = 0;
	double longitude = 0;
	double altitude = 0;
	
	float maxDistance = 0;

	UPROPERTY(EditAnywhere);
		UTextRenderComponent* textComponent = NULL;
	UPROPERTY(EditAnywhere);
		UStaticMeshComponent* meshComponent = NULL;
	UPROPERTY(EditAnywhere);
		UBoxComponent* collisionComponent = NULL;

	ALocationMarker();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	void SetText(std::string text);
	void EnableCollision();
};