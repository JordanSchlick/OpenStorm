// Copyright Epic Games, Inc. All Rights Reserved.


#include "OpenStormGameModeBase.h"


void AOpenStormGameModeBase::InitGameState()
{
	Super::InitGameState();
	
	//set default pawn
	//if (DefaultPawnClass == ADefaultPawn::StaticClass())
	//{
	//	DefaultPawnClass = CustomRadarPawnClass;
	//}
	
	//DefaultPawnClass = CustomRadarPawnClass;
}

AOpenStormGameModeBase::AOpenStormGameModeBase()
{
	DefaultPawnClass = ARadarViewPawn::StaticClass();
}
