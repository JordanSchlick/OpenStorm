#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DiscordPresence.generated.h"

UCLASS()
class OPENSTORM_API ADiscordPresence : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADiscordPresence();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	bool isPresenceActive = false;
	double lastPresenceTime = 0;
	
	void StartDiscordPresence();
	void StopDiscordPresence();
};
