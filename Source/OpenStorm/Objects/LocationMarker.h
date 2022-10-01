#pragma once

#include <string>

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "LocationMarker.generated.h"

class UTextRenderComponent;

UCLASS()
class ALocationMarker : public AActor{
	GENERATED_BODY()
	
public:


	UPROPERTY(EditAnywhere);
		UTextRenderComponent* textComponent = NULL;
	UPROPERTY(EditAnywhere);
		UStaticMeshComponent* meshComponent = NULL;

	ALocationMarker();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	void SetText(std::string text);
};