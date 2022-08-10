// Copyright Epic Games, Inc. All Rights Reserved.


#include "OpenStormGameModeBase.h"


void AOpenStormGameModeBase::InitGameState()
{
	Super::InitGameState();
	
}

AOpenStormGameModeBase::AOpenStormGameModeBase()
{
	DefaultPawnClass = ARadarViewPawn::StaticClass();
	GameStateClass = ARadarGameStateBase::StaticClass();
}
