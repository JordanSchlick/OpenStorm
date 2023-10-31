#include "DiscordPresence.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#include "../Objects/RadarGameStateBase.h"
#include "../Radar/SystemAPI.h"


#include "../Deps/discord-rpc/include/discord_rpc.h"


// Sets default values
ADiscordPresence::ADiscordPresence()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void ADiscordPresence::BeginPlay()
{
	Super::BeginPlay();
	StartDiscordPresence();
}

void ADiscordPresence::EndPlay(const EEndPlayReason::Type endPlayReason){
	Super::EndPlay(endPlayReason);
	StopDiscordPresence();
}

// Called every frame
void ADiscordPresence::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		if(globalState->discordPresence){
			StartDiscordPresence();
		}else{
			StopDiscordPresence();
		}
	}
	
	if(isPresenceActive){
		double now = SystemAPI::CurrentTime();
		if(lastPresenceTime + 20 < now){
			lastPresenceTime = now;
			DiscordRichPresence presence = {};
			presence.details = "A 3D radar viewer";
			presence.state = "Open source and free";
			presence.largeImageKey = "openstormlogo";
			presence.largeImageText = "OpenStorm";
			presence.startTimestamp = now - UGameplayStatics::GetRealTimeSeconds(GetWorld());
			presence.button1Enabled = true;
			presence.button1Label = "Get OpenStorm on GitHub";
			presence.button1Url = "https://github.com/JordanSchlick/OpenStorm";
			Discord_UpdatePresence(&presence);
		}
		
	}
	#ifdef DISCORD_DISABLE_IO_THREAD
	Discord_UpdateConnection();
	#endif
	Discord_RunCallbacks();
}



void ADiscordPresence::StartDiscordPresence(){
	if(isPresenceActive){
		return;
	}
	isPresenceActive = true;
	
	DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
	
	Discord_Initialize("1168656505059426455", &handlers, false, NULL);
	
	// set presence in 5 seconds
	lastPresenceTime = SystemAPI::CurrentTime() - 15;
}

void ADiscordPresence::StopDiscordPresence(){
	if(!isPresenceActive){
		return;
	}
	isPresenceActive = false;
	Discord_Shutdown();
}

