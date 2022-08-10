// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "../Application/GlobalState.h"
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "RadarGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class OPENSTORM_API ARadarGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	ARadarGameStateBase();
	
public:
	//New Global State
	GlobalState globalState = GlobalState();
	
};
