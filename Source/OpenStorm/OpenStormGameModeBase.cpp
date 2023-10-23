// Copyright Epic Games, Inc. All Rights Reserved.


#include "OpenStormGameModeBase.h"


void AOpenStormGameModeBase::InitGameState()
{
	Super::InitGameState();
	#ifdef _WIN32
		// set file apis to binary on windows
		_set_fmode(0x8000);
	#endif // _WIN32
}

AOpenStormGameModeBase::AOpenStormGameModeBase()
{
	DefaultPawnClass = ARadarViewPawn::StaticClass();
	GameStateClass = ARadarGameStateBase::StaticClass();
}
