#pragma once

#include <string>

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/Object.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "../UI/ClickableInterface.h"
#include "LocationMarker.generated.h"

class UTextRenderComponent;

UCLASS()
class ALocationMarker : public AActor, public IClickableInterface{
	GENERATED_BODY()
	
public:

	enum MarkerType {
		MarkerTypeWaypoint,
		MarkerTypeRadarSite
	};
	
	MarkerType markerType = MarkerTypeWaypoint;
	
	double latitude = 0;
	double longitude = 0;
	double altitude = 0;
	
	// maximum distance from camera before the marker is destroyed
	float maxDistance = 0;
	
	// used for storing site id
	std::string data;

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
	
	virtual void OnClick() override;
};