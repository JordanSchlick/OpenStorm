// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RadarViewPawn.h"
#include "GameFramework/DefaultPawn.h"
#include "OpenStormGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class OPENSTORM_API AOpenStormGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	void InitGameState() override;

	/*UPROPERTY(EditAnywhere, NoClear)
		TSubclassOf<ARadarViewPawn> CustomRadarPawnClass = ARadarViewPawn::StaticClass();*/

	AOpenStormGameModeBase();
};
