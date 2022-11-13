#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"

#include "AudioCaptureCore.h"
//#include "AudioCapture.h"

#include "Joke.generated.h"

UCLASS()
class AJoke : public AActor {
	GENERATED_BODY()

public:
	AJoke();

	bool capturing = false;
	float totalVolume = 0;
	float totalNumSamples = 0;
	float totalPeak = 0;

	Audio::FAudioCapture audioCapture;

	void AudioHandler(const float* AudioData, int32 NumFrames, int32 NumChannels, int32 SampleRate, double StreamTime, bool bOverFlow);
	void StartAudioCapture();
	void StopAudioCapture();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
};