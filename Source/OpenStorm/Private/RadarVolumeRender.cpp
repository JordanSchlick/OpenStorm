// Fill out your copyright notice in the Description page of Project Settings.




#include "RadarVolumeRender.h"
#include "StdioConsole.h"
#include "RadarGameStateBase.h"
#include "../EngineHelpers/MaterialRenderTarget.h"


//#define VERBOSE 1
#include "../Radar/RadarData.h"
#include "../Radar/RadarCollection.h"
#include "../Radar/RadarColorIndex.h"
#include "../Radar/SystemAPI.h"


#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "Math/UnrealMathUtility.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceDynamic.h"

#include <algorithm>
#include <cmath>

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

#define PI2F 6.283185307179586f

ARadarVolumeRender *ARadarVolumeRender::instance = NULL;
// Sets default values
ARadarVolumeRender::ARadarVolumeRender()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a static mesh component
	cubeMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube"));

	// Set the component's mesh

	cubeMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'")).Object;

	storedMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/RadarVolumeMaterial.RadarVolumeMaterial'")).Object;

	//storedInterpolationMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/InterpolationMaterial.InterpolationMaterial'")).Object;
	storedInterpolationMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/InterpolationAdvancedMaterial.InterpolationAdvancedMaterial'")).Object;



	cubeMeshComponent->SetStaticMesh(cubeMesh);

	cubeMeshComponent->SetMaterial(0, storedMaterial);

	// Set as root component
	RootComponent = cubeMeshComponent;

	SetActorEnableCollision(false);

	instance = this;
}



// Called when the game starts or when spawned
void ARadarVolumeRender::BeginPlay()
{
	Super::BeginPlay();

	// Load the Cube mesh
	cubeMeshComponent->SetStaticMesh(cubeMesh);

	AStdioConsole::ShowConsole();

	/*
	//IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ShowFlag.PostProcessing"));
	//CVar->Set(0);
	//GEngine->Exec(GetWorld(), TEXT("ShowFlag.PostProcessing 0"));
	GEngine->Exec(GetWorld(), TEXT("ShowFlag.ToneMapper 0"));

//#if UE_BUILD_SHIPPING
	GEngine->Exec(GetWorld(), TEXT("r.TonemapperGamma 0"));
	GEngine->Exec(GetWorld(), TEXT("r.TonemapperFilm 0"));
	GEngine->Exec(GetWorld(), TEXT("r.Tonemapper.Quality 0"));
	fprintf(stderr, "Disabled tonemapper in shipping\n");
//#endif
	*/

	//UE_LOG(LogTemp, Display, TEXT("%s"), StoredMaterial->GetName());

	interpolationMaterialInstance = UMaterialInstanceDynamic::Create(storedInterpolationMaterial, this);

	radarMaterialInstance = UMaterialInstanceDynamic::Create(cubeMeshComponent->GetMaterial(0), this);
	cubeMeshComponent->SetMaterial(0, radarMaterialInstance);



	radarMaterialInstance->SetVectorParameterValue(TEXT("Center"), this->GetActorLocation());


	FString radarFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Data/Demo/KMKX_20220723_235820"));
	//FString radarFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Data/Demo/KTLX20130531_231434_V06"));
	FString fullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*radarFile);
	const char* fileLocaition = StringCast<ANSICHAR>(*fullPath).Get();
	radarData = new RadarData();
	radarData->ReadNexrad(fileLocaition);
	//radarData->Compress();
	
	if (radarData->radiusBufferCount == 0) {
		fprintf(stderr, "Could not open %s\n", fileLocaition);
		return;
	}

	// PF_B8G8R8A8 - 4 uint8 values per pixel
	// PF_R32_FLOAT - single float value per pixel
	// PF_A32B32G32R32F - 4 float values per pixel

	InitializeTextures();
	

	radarMaterialInstance->SetScalarParameterValue(TEXT("InnerDistance"), radarData->innerDistance);

	RadarData::TextureBuffer imageBuffer = radarData->CreateTextureBufferReflectivity2();
	float* RawImageData = (float*)volumeImageData->Lock(LOCK_READ_WRITE);
	memcpy(RawImageData, imageBuffer.data, imageBuffer.byteSize);
	volumeImageData->Unlock();
	volumeTexture->UpdateResource();
	imageBuffer.Delete();


	imageBuffer = radarData->CreateAngleIndexBuffer();
	float* rawAngleIndexImageData = (float*)angleIndexImageData->Lock(LOCK_READ_WRITE);
	memcpy(rawAngleIndexImageData, imageBuffer.data, imageBuffer.byteSize);
	angleIndexImageData->Unlock();
	angleIndexTexture->UpdateResource();
	imageBuffer.Delete();
	

	RadarColorIndex::Params colorParams = {};
	colorParams.fromRadarData(radarData);
	//colorParams.minValue = 0; colorParams.maxValue = 1;
	//RadarColorIndex::Result valueIndex = RadarColorIndex::relativeHue(colorParams);
	radarColorResult = RadarColorIndex::reflectivityColors(colorParams, &radarColorResult);
	float* rawValueIndexImageData = (float*)valueIndexImageData->Lock(LOCK_READ_WRITE);
	//memcpy(rawValueIndexImageData, valueIndex.data, 16384);
	memcpy(rawValueIndexImageData, radarColorResult.data, radarColorResult.byteSize);
	valueIndexImageData->Unlock();
	valueIndexTexture->UpdateResource();
	// set value bounds
	radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexLower"), radarColorResult.lower);
	radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexUpper"), radarColorResult.upper);

	

	
	RadarCollection::Testing();
	
	FString radarDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("../files/dir/"));
	FString fullradarDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*radarDir);
	const char* radarDirLocaition = StringCast<ANSICHAR>(*fullradarDir).Get();
	fprintf(stderr, "path %s\n", radarDirLocaition);
	
	radarCollection = new RadarCollection();
	radarCollection->filePath = std::string(radarDirLocaition);

	radarCollection->RegisterListener([this](RadarCollection::RadarUpdateEvent event) {
		if (event.data != NULL) {
			//RadarData::TextureBuffer buffer = data->CreateTextureBufferReflectivity2();
			//float* RawImageData = (float*)volumeImageData->Lock(LOCK_READ_WRITE);
			//memcpy(RawImageData, buffer.data, buffer.byteSize);
			//volumeImageData->Unlock();
			//volumeTexture->UpdateResource();
			//if(buffer.data != NULL){
			//	delete[] buffer.data;
			//}
			
			// RadarData::TextureBuffer buffer = data->CreateTextureBufferReflectivity2();
			// FUpdateTextureRegion2D* regions = new FUpdateTextureRegion2D[1]();
			// regions[0].DestX = 0;
			// regions[0].DestY = 0;
			// regions[0].SrcX = 0;
			// regions[0].SrcY = 0;
			// regions[0].Width = data->radiusBufferCount;
			// regions[0].Height = data->fullBufferSize / data->thetaBufferSize;

			// volumeTexture->UpdateTextureRegions(0, 1, regions, data->radiusBufferCount * 4, 4, (uint8*)buffer.data, [](uint8* dataPtr, const FUpdateTextureRegion2D* regionsPtr) {
			// 	delete regionsPtr;
			// });
			
			radarData->CopyFrom(event.data);
			RadarData::TextureBuffer buffer = radarData->CreateTextureBufferReflectivity2();
			FUpdateTextureRegion2D* regions = new FUpdateTextureRegion2D[1]();
			regions[0].DestX = 0;
			regions[0].DestY = 0;
			regions[0].SrcX = 0;
			regions[0].SrcY = 0;
			regions[0].Width = radarData->radiusBufferCount;
			regions[0].Height = radarData->fullBufferSize / radarData->thetaBufferSize;
			
			
			UTexture2D* textureToUpdate = volumeTexture;
			if(doTimeInterpolation){
				double now = SystemAPI::CurrentTime();
				if(usePrimaryTexture){
					interpolationStartValue = 1;
					interpolationEndValue = 0;
				}else{
					textureToUpdate = volumeTexture2;
					interpolationStartValue = 0;
					interpolationEndValue = 1;
				}
				interpolationAnimating = true;
				interpolationStartTime = now;
				interpolationEndTime = now + (double)event.minTimeTillNext;
				interpolationMaterialInstance->SetScalarParameterValue(TEXT("Amount"), interpolationStartValue);
				//interpolationMaterialInstance->SetScalarParameterValue(TEXT("Minimum"), radarColorResult.lower);
				usePrimaryTexture = !usePrimaryTexture;
				
			}
			
			textureToUpdate->UpdateTextureRegions(0, 1, regions, radarData->radiusBufferCount * 4, 4, (uint8*)buffer.data, [](uint8* dataPtr, const FUpdateTextureRegion2D* regionsPtr) {
				delete regionsPtr;
			});
			
		}
	});

	radarCollection->Allocate(75);
	radarCollection->ReadFiles();
	radarCollection->LoadNewData();


	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	
	callbackIds.push_back(globalState->RegisterEvent("Test",[this](std::string stringData, void* extraData){
		fprintf(stderr, "Test event received in RadarVolumeRender\n");
	}));
	callbackIds.push_back(globalState->RegisterEvent("ForwardStep",[this](std::string stringData, void* extraData){
		radarCollection->MoveManual(1);
	}));
	callbackIds.push_back(globalState->RegisterEvent("BackwardStep",[this](std::string stringData, void* extraData){
		radarCollection->MoveManual(-1);
	}));
	
	//RandomizeTexture();
}

void ARadarVolumeRender::EndPlay(const EEndPlayReason::Type endPlayReason) {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	for(auto id : callbackIds){
		globalState->UnregisterEvent(id);
	}
}

//=======================================================================================================
void ARadarVolumeRender::InitializeTextures() {
	int textureWidth = radarData->radiusBufferCount;
	int textureHeight = (radarData->thetaBufferCount + 2) * radarData->sweepBufferCount;
	EPixelFormat pixelFormat = PF_R32_FLOAT;
	
	// the volume texture contains an array that is interperted as a three dimensional volume of values
	// it corresponds to the real values of the radar
	volumeTexture = UTexture2D::CreateTransient(textureWidth, textureHeight, pixelFormat);
	FTexture2DMipMap* MipMap = &(volumeTexture->PlatformData->Mips[0]);
	volumeImageData = &(MipMap->BulkData);
	volumeTexture->UpdateResource();
	radarMaterialInstance->SetTextureParameterValue(TEXT("Volume"), volumeTexture);
	usePrimaryTexture = true;
	
	if (doTimeInterpolation) {
		volumeMaterialRenderTarget = UMaterialRenderTarget::Create(textureWidth, textureHeight, pixelFormat, interpolationMaterialInstance, this);
		radarMaterialInstance->SetTextureParameterValue(TEXT("Volume"), volumeMaterialRenderTarget);
		volumeMaterialRenderTarget->Update();
		
		volumeTexture2 = UTexture2D::CreateTransient(textureWidth, textureHeight, pixelFormat);
		MipMap = &(volumeTexture2->PlatformData->Mips[0]);
		volumeImageData2 = &(MipMap->BulkData);
		volumeTexture2->UpdateResource();

		interpolationMaterialInstance->SetScalarParameterValue(TEXT("Amount"), 0);
		interpolationMaterialInstance->SetTextureParameterValue(TEXT("Texture1"), volumeTexture);
		interpolationMaterialInstance->SetTextureParameterValue(TEXT("Texture2"), volumeTexture2);
	}else{
		volumeMaterialRenderTarget = NULL;
		volumeImageData2 = NULL;
	}
	interpolationAnimating = false;
	
	
	// the angle index texture maps the angle from the ground to a sweep
	// pixel 65536 is strait up, pixel 32768 is parallel with the ground, pixel 0 is strait down
	// values less than zero mean no sweep, value 0.0-255.0 specify the index of the sweep, value 0.0 specifies first sweep, values in-between intagers will interpolate sweeps
	angleIndexTexture = UTexture2D::CreateTransient(256, 256, PF_R32_FLOAT);
	MipMap = &(angleIndexTexture->PlatformData->Mips[0]);
	angleIndexImageData = &(MipMap->BulkData);
	radarMaterialInstance->SetTextureParameterValue(TEXT("AngleIndex"), angleIndexTexture);
	
	// th value index texture maps the real values of the radar to color and alpha values
	valueIndexTexture = UTexture2D::CreateTransient(128, 128, PF_A32B32G32R32F);
	MipMap = &(valueIndexTexture->PlatformData->Mips[0]);
	valueIndexImageData = &(MipMap->BulkData);
	radarMaterialInstance->SetTextureParameterValue(TEXT("ValueIndex"), valueIndexTexture);
}



static uint32_t deadbeef_seed;
static uint32_t deadbeef_beef = 0xdeadbeef;

inline uint32_t deadbeefRand() {
	deadbeef_seed = (deadbeef_seed << 7) ^ ((deadbeef_seed >> 25) + deadbeef_beef);
	deadbeef_beef = (deadbeef_beef << 7) ^ ((deadbeef_beef >> 25) + 0xdeadbeef);
	return deadbeef_seed;
}

// Called every frame
void ARadarVolumeRender::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	double now = SystemAPI::CurrentTime();
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	radarCollection->automaticallyAdvance = globalState->animate;
	radarCollection->autoAdvanceInterval = 1.0f / globalState->animateSpeed;
	
	if(volumeMaterialRenderTarget != NULL){
		volumeMaterialRenderTarget->Update();
	}
	
	if(interpolationAnimating){
		if(interpolationEndTime == interpolationStartTime){
			interpolationMaterialInstance->SetScalarParameterValue(TEXT("Amount"), interpolationEndValue);
			interpolationAnimating = false;
		}else{
			float animationProgress = std::clamp((now - interpolationStartTime) / (interpolationEndTime - interpolationStartTime), 0.0, 1.0);
			float animationValue = interpolationStartValue * (1.0f - animationProgress) + interpolationEndValue * animationProgress;
			interpolationMaterialInstance->SetScalarParameterValue(TEXT("Amount"), animationValue);
			//fprintf(stderr, "%f\n", animationValue);
			if(animationProgress == 1){
				interpolationAnimating = false;
			}
		}
	}
	//return;
	//*
	RadarColorIndex::Params colorParams = {};
	colorParams.fromRadarData(radarData);
	radarColorResult = RadarColorIndex::reflectivityColors(colorParams, &radarColorResult);
	float cutoff = globalState->cutoff;
	if(globalState->animateCutoff){
		cutoff = (sin(fmod(now, globalState->animateCutoffTime) / globalState->animateCutoffTime * PI2F) + 1) / 2 * cutoff;
		//cutoff = abs(fmod(now, globalState->animateCutoffTime) / globalState->animateCutoffTime * -2 + 1) * cutoff;
	}
	RadarColorIndex::Cutoff(cutoff, &radarColorResult);
	float* rawValueIndexImageData = (float*)valueIndexImageData->Lock(LOCK_READ_WRITE);
	//memcpy(rawValueIndexImageData, valueIndex.data, 16384);
	
	memcpy(rawValueIndexImageData, radarColorResult.data, radarColorResult.byteSize);
	valueIndexImageData->Unlock();
	valueIndexTexture->UpdateResource();

	// set value bounds
	radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexLower"), radarColorResult.lower);
	radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexUpper"), radarColorResult.upper);
	
	if (doTimeInterpolation) {
		interpolationMaterialInstance->SetScalarParameterValue(TEXT("Minimum"), radarColorResult.lower);
	}

	radarCollection->EventLoop();
}


void ARadarVolumeRender::RandomizeTexture() {

	uint8* RawImageData = (uint8*)volumeImageData->Lock(LOCK_READ_WRITE);
	float* RawImageDataFloat = (float*)RawImageData;

	uint8_t intensity = deadbeefRand();
	//RawImageData[0] = FMath::RandRange(0, 255);
	//*
	for (int x = 1; x < 128 * 128 * 8 * 4; x++) {
		//RawImageData[x] = FMath::RandRange(0, 255);
		RawImageData[x] = deadbeefRand();
		//RawImageData[x] = intensity;
	}
	for (int x = 0; x < 128 * 128 * 8 * 4; x+=4) {
		//RawImageData[x] = FMath::RandRange(0, 255);
		RawImageData[x+3] = 255;
		//RawImageData[x] = intensity;
	}
	//*/

	/*
	for (int x = 1; x < 128 * 128 * 8; x++) {
		//RawImageData[x] = FMath::RandRange(0, 255);
		RawImageDataFloat[x] = deadbeefRand() > 0xEFFFFFFF ? (float)deadbeefRand() / (float)0xFFFFFFFF : 0;
	}
	//*/

	volumeImageData->Unlock();
	volumeTexture->UpdateResource();
}

ARadarVolumeRender::~ARadarVolumeRender()
{
	if(radarCollection != NULL){
		delete radarCollection;
	}
	if(radarData != NULL){
		delete radarData;
	}
	if(instance == this){
		instance = NULL;
	}
	radarColorResult.Delete();
}



