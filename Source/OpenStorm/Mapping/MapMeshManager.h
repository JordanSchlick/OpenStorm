#pragma once

#include "../Radar/SimpleVector3.h"

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "MapMeshManager.generated.h"

class AMapMesh;

UCLASS()
class AMapMeshManager : public AActor{
	GENERATED_BODY()
	
public:
	int maxLayer = 15;
	SimpleVector3<> cameraLocation = {};
	
	
	UPROPERTY(EditAnywhere);
	AMapMesh* rootMapMesh = NULL;
	
	
	AMapMeshManager();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
};