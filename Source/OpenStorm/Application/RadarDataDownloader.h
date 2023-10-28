#pragma once

#include <vector>
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RadarDataDownloader.generated.h"

class RadarDownloaderTask;

UCLASS()
class OPENSTORM_API ARadarDataDownloader : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARadarDataDownloader();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	RadarDownloaderTask* downloaderTask = NULL;
	std::vector<uint64_t> callbackIds = {};
};
