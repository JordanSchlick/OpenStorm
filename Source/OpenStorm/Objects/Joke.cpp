#include "Joke.h"

#include "RadarGameStateBase.h"
#include "Engine/World.h"
#include <cmath>

/*
class UCustomCapture : public UAudioCapture {

};*/


AJoke::AJoke() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	//audioCapture = UObject::CreateDefaultSubobject<FAudioCapture>(TEXT("Capture"));
}

void AJoke::BeginPlay() {
	Super::BeginPlay();
	
}



void AJoke::AudioHandler(const float* audioData, int32 numFrames, int32 numChannels, int32 sampleRate, double streamTime, bool bOverFlow){
	int32 numSamples = numFrames * numChannels;
	float volume = 0;
	float peak = 0;
	for(int32 i = 0; i < numSamples; i++){
		float abs = std::abs(audioData[i]);
		volume += abs;
		peak = std::max(peak, abs);
	}
	
	this->totalVolume += volume;
	this->totalNumSamples += numSamples;
	this->totalPeak = std::max(this->totalPeak, peak);;

	volume /= (float)numSamples;
	
	
	
	//fprintf(stderr,"samples %i volume %f ",numSamples, volume);
}

void AJoke::StartAudioCapture() {
	Audio::FOnCaptureFunction OnCapture = [this](const float* AudioData, int32 NumFrames, int32 NumChannels, int32 SampleRate, double StreamTime, bool bOverFlow){
		AudioHandler(AudioData, NumFrames, NumChannels, SampleRate, StreamTime, bOverFlow);
	};
	Audio::FAudioCaptureDeviceParams Params = Audio::FAudioCaptureDeviceParams();
	//TArray<Audio::FCaptureDeviceInfo> devices;
	//int deviceCount = audioCapture.GetCaptureDevicesAvailable(devices);
	//fprintf(stderr, "Device count %i \n", deviceCount);
	//for (int i = 0; i < deviceCount; i++) {
	//	fprintf(stderr, "Device id %i %s \n", i, std::string(TCHAR_TO_UTF8(*devices[i].DeviceName)).c_str());
	//}

	//for (int i = 0; i < 10; i++) {
	//	Audio::FCaptureDeviceInfo deviceInfo;
	//	if (audioCapture.GetCaptureDeviceInfo(deviceInfo, i)) {
	//		fprintf(stderr, "Device id %i %i %s %s \n", i, deviceInfo.InputChannels, std::string(TCHAR_TO_UTF8(*deviceInfo.DeviceId)).c_str(), std::string(TCHAR_TO_UTF8(*deviceInfo.DeviceName)).c_str());
	//	}
	//}

	//fprintf(stderr, "======================================================================= \n");


	//Params.DeviceIndex = 6;
	if (audioCapture.OpenCaptureStream(Params, MoveTemp(OnCapture), 512)){
		audioCapture.StartStream();
	}
	capturing = true;
}

void AJoke::StopAudioCapture() {
	audioCapture.AbortStream();
	capturing = false;
}

void AJoke::Tick(float DeltaTime) {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	if (totalNumSamples > 0) {
		totalVolume /= (float)totalNumSamples;
		// UWorld* world = GetWorld();
		// if (world) {
		// 	ARadarGameStateBase* GS = world->GetGameState<ARadarGameStateBase>();
		// 	if (GS) {
		// 		GlobalState* globalState = &GS->globalState;
		// 	}
		// }
		if(globalState->audioControlledHeight){
			globalState->verticalScale = 1 + totalVolume * globalState->audioControlMultiplier;
		}
		if(globalState->audioControlledOpacity){
			globalState->opacityMultiplier = 1 + totalVolume * globalState->audioControlMultiplier;
		}
		if(globalState->audioControlledCutoff){
			globalState->cutoff = 1 - totalVolume * globalState->audioControlMultiplier / 2;
		}
		totalNumSamples = 0;
		totalVolume = 0;
		totalPeak = 0;
	}
	
	bool enabled = globalState->audioControlledHeight || globalState->audioControlledOpacity || globalState->audioControlledCutoff;
	
	if(!capturing && enabled){
		StartAudioCapture();
	}
	
	if(capturing && !enabled){
		StopAudioCapture();
	}
}
