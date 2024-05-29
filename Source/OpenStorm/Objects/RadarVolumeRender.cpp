// Fill out your copyright notice in the Description page of Project Settings.




#include "RadarVolumeRender.h"
#include "../UI/Native.h"
#include "RadarGameStateBase.h"
#include "../EngineHelpers/MaterialRenderTarget.h"

#include "imgui.h"

//#define VERBOSE 1
#include "../Radar/RadarData.h"
#include "../Radar/RadarCollection.h"
#include "../Radar/RadarColorIndex.h"
#include "../Radar/Products/RadarProduct.h"
#include "../Radar/SystemAPI.h"
#include "../Radar/Globe.h"
#include "../Application/GlobalState.h"
#include "../EngineHelpers/StringUtils.h"


#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "Math/UnrealMathUtility.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"

#include <algorithm>
#include <cmath>
#include <functional>

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

#define PI2F 6.283185307179586f

void UpdateTexture(UTexture2D* texture, uint8_t* buffer, size_t bufferSizeBytes, int pixelSizeBytes, std::function<void(uint8_t* buffer)> callback = [](uint8_t* buffer){}){
	int width = texture->GetSizeX();
	FUpdateTextureRegion2D* regions = new FUpdateTextureRegion2D[1]();
	regions[0].DestX = 0;
	regions[0].DestY = 0;
	regions[0].SrcX = 0;
	regions[0].SrcY = 0;
	regions[0].Width = width;
	regions[0].Height = std::min(bufferSizeBytes / pixelSizeBytes / width, (size_t)texture->GetSizeY());
	
	texture->UpdateTextureRegions(0, 1, regions, width * pixelSizeBytes, pixelSizeBytes, (uint8*)buffer, [callback](uint8* dataPtr, const FUpdateTextureRegion2D* regionsPtr) {
		delete regionsPtr;
		callback(dataPtr);
	});
}


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

	static bool doAllocatedConsole = true;
	if (doAllocatedConsole) {
		NativeAPI::AllocateConsole();
		doAllocatedConsole = false;
	}

#if WITH_EDITOR
	NativeAPI::ShowConsole();
#endif

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

	interpolationMaterialInstance = UMaterialInstanceDynamic::Create(storedInterpolationMaterial, this, "InterpolationMaterialInstance");

	radarMaterialInstance = UMaterialInstanceDynamic::Create(cubeMeshComponent->GetMaterial(0), this, "RadarMaterialInstance");
	cubeMeshComponent->SetMaterial(0, radarMaterialInstance);



	radarMaterialInstance->SetVectorParameterValue(TEXT("Center"), this->GetActorLocation());


	//FString radarFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Data/Demo/KMKX_20220723_235820"));
	//FString radarFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Data/Demo/KTLX20130531_231434_V06"));
	//FString fullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*radarFile);
	//const char* fileLocaition = StringCast<ANSICHAR>(*fullPath).Get();
	radarData = new RadarData();
	//radarData->ReadNexrad(fileLocaition);
	//radarData->Compress();
	
	//if (radarData->radiusBufferCount == 0) {
	//	fprintf(stderr, "Could not open %s\n", fileLocaition);
	//	return;
	//}

	// PF_B8G8R8A8 - 4 uint8 values per pixel
	// PF_R32_FLOAT - single float value per pixel
	// PF_A32B32G32R32F - 4 float values per pixel

	//InitializeTextures();
	

	radarMaterialInstance->SetScalarParameterValue(TEXT("InnerDistance"), 0);
	radarMaterialInstance->SetScalarParameterValue(TEXT("StepSize"), 5);
	
	// RadarData::TextureBuffer imageBuffer = radarData->CreateTextureBufferReflectivity2();
	// float* RawImageData = (float*)volumeImageData->Lock(LOCK_READ_WRITE);
	// memcpy(RawImageData, imageBuffer.data, imageBuffer.byteSize);
	// volumeImageData->Unlock();
	// volumeTexture->UpdateResource();
	// imageBuffer.Delete();


	// imageBuffer = radarData->CreateAngleIndexBuffer();
	// float* rawAngleIndexImageData = (float*)angleIndexImageData->Lock(LOCK_READ_WRITE);
	// memcpy(rawAngleIndexImageData, imageBuffer.data, imageBuffer.byteSize);
	// angleIndexImageData->Unlock();
	// angleIndexTexture->UpdateResource();
	// imageBuffer.Delete();
	

	// RadarColorIndex::Params colorParams = {};
	// colorParams.fromRadarData(radarData);
	// //colorParams.minValue = 0; colorParams.maxValue = 1;
	// //RadarColorIndex::Result valueIndex = RadarColorIndex::relativeHue(colorParams);
	// radarColorResult = RadarColorIndex::reflectivityColors(colorParams, &radarColorResult);
	// float* rawValueIndexImageData = (float*)valueIndexImageData->Lock(LOCK_READ_WRITE);
	// memcpy(rawValueIndexImageData, radarColorResult.data, radarColorResult.byteSize);
	// valueIndexImageData->Unlock();
	// valueIndexTexture->UpdateResource();
	// // set value bounds
	// radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexLower"), radarColorResult.lower);
	// radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexUpper"), radarColorResult.upper);

	

	
	//RadarCollection::Testing();
	
	
	
	radarCollection = new RadarCollection();

	radarCollection->RegisterListener([this](RadarCollection::RadarUpdateEvent event) {
		HandleRadarDataEvent(event);
	});

	radarCollection->Allocate(75);
	
	
	FString radarDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("../files/dir/"));
	if(!FPaths::DirectoryExists(radarDir)){
		radarDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("../../files/dir/"));
	}
	if(!FPaths::DirectoryExists(radarDir)){
		radarDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Data/Demo/"));
	}
	
	FString fullradarDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*radarDir);
	// UE_LOG(LogTemp, Display, TEXT("Default radar file dir is %s"), *fullradarDir);
	// const char* radarDirLocaition = StringCast<ANSICHAR>(*fullradarDir).Get();
	// fprintf(stderr, "path %s\n", radarDirLocaition);
	std::string radarDirLocaition = StringUtils::FStringToSTDString(fullradarDir);
	radarCollection->ReadFiles(radarDirLocaition);
	
	
	//radarCollection->LoadNewData();


	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	
	globalState->refRadarCollection = radarCollection;
	
	// register global events
	callbackIds.push_back(globalState->RegisterEvent("Test",[this](std::string stringData, void* extraData){
		fprintf(stderr, "Test event received in RadarVolumeRender\n");
	}));
	callbackIds.push_back(globalState->RegisterEvent("ForwardStep",[this](std::string stringData, void* extraData){
		radarCollection->MoveManual(1);
	}));
	callbackIds.push_back(globalState->RegisterEvent("BackwardStep",[this](std::string stringData, void* extraData){
		radarCollection->MoveManual(-1);
	}));
	callbackIds.push_back(globalState->RegisterEvent("LoadDirectory",[this](std::string stringData, void* extraData){
		radarCollection->ReadFiles(stringData);
	}));
	callbackIds.push_back(globalState->RegisterEvent("DevReloadFile",[this](std::string stringData, void* extraData){
		radarCollection->ReloadFile(-1);
	}));
	callbackIds.push_back(globalState->RegisterEvent("UpdateVolumeParameters",[this, globalState](std::string stringData, void* extraData){	
		radarMaterialInstance->SetScalarParameterValue(TEXT("Mode"), globalState->spatialInterpolation ? 1 : 0);
		float stepSize = 5;
		if(globalState->quality == 10){
			stepSize = std::max(globalState->qualityCustomStepSize, 0.1f);
		}else if(globalState->quality >= 3){
			stepSize = 0.1;
		}else if(globalState->quality >= 2){
			stepSize = 1;
		}else if(globalState->quality >= 1){
			stepSize = 2;
		}else if(globalState->quality <= -10){
			stepSize = 50;
		}else if(globalState->quality <= -2){
			stepSize = 20;
		}else if(globalState->quality <= -1){
			stepSize = 10;
		}
		// fprintf(stderr,"stepSize: %f",stepSize);
		radarMaterialInstance->SetScalarParameterValue(TEXT("StepSize"), stepSize);
		radarMaterialInstance->SetScalarParameterValue(TEXT("Fuzz"), globalState->enableFuzz ? 1 : 0);
	}));
	callbackIds.push_back(globalState->RegisterEvent("ChangeProduct", [this, globalState](std::string stringData, void* extraData) {
		if (extraData == NULL) {
			return;
		}
		RadarData::VolumeType volumeType = *(RadarData::VolumeType*)extraData;
		radarCollection->ChangeProduct(volumeType);
	}));
	callbackIds.push_back(globalState->RegisterEvent("JumpToIndex", [this, globalState](std::string stringData, void* extraData) {
		if (extraData == NULL) {
			return;
		}
		radarCollection->Jump(*(size_t*)extraData);
	}));
	
	//RandomizeTexture();
}

void ARadarVolumeRender::HandleRadarDataEvent(RadarCollection::RadarUpdateEvent event){
	fprintf(stderr, "%p\n", event.data);
	if (event.data != NULL) {
		
		// double benchTime = SystemAPI::CurrentTime();
		
		// copy to local buffer to decompress and retain data
		radarData->CopyFrom(event.data);
		
		// benchTime = SystemAPI::CurrentTime() - benchTime;
		// fprintf(stderr, "volume decompress time %fs\n", benchTime);
		// fprintf(stderr, "volume bounds %f %f %f\n", radarData->stats.boundLower, radarData->stats.boundUpper, radarData->stats.boundRadius);
		
		InitializeTextures();
		
		
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
			usePrimaryTexture = !usePrimaryTexture;
			
		}
		
		
		float boundRadius = event.data->stats.boundRadius + 1.0f;
		float boundUpper = event.data->stats.boundUpper + 5.0f;
		float boundLower = event.data->stats.boundLower - 1.0f;
		radarMaterialInstance->SetScalarParameterValue(TEXT("CutoffRadius"), boundRadius);
		radarMaterialInstance->SetVectorParameterValue(TEXT("CutoffCircles"), FVector4(
			boundUpper, 
			std::sqrt(boundRadius*boundRadius-boundUpper*boundUpper), 
			boundLower,
			std::sqrt(boundRadius*boundRadius-boundLower*boundLower)
		));
		radarMaterialInstance->SetScalarParameterValue(TEXT("InnerDistance"), event.data->stats.innerDistance);
		radarMaterialInstance->SetScalarParameterValue(TEXT("Scale"), 2.5);
		
		radarColor = RadarColorIndex::GetDefaultColorIndexForData(radarData);
		
		UpdateTexture(textureToUpdate, (uint8_t*)radarData->buffer, radarData->fullBufferSize * sizeof(float), sizeof(float));
		
		// Orient globe to match up with radar
		//fprintf(stderr, "Location lat:%f lon:%f \n", event.data->stats.latitude, event.data->stats.longitude);
		GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
		globalState->globe->SetCenter(0, 0, -globalState->globe->surfaceRadius - event.data->stats.altitude);
		globalState->globe->SetTopCoordinates(event.data->stats.latitude, event.data->stats.longitude);
		auto vector = globalState->globe->GetPointScaledDegrees(event.data->stats.latitude, event.data->stats.longitude, event.data->stats.altitude);
		radarMaterialInstance->SetVectorParameterValue(TEXT("Center"), FVector(vector.x, vector.y, vector.z));
		globalState->EmitEvent("GlobeUpdate");
		
		
		
		RadarData::TextureBuffer imageBuffer = radarData->CreateAngleIndexBuffer();
		UpdateTexture(angleIndexTexture, (uint8_t*)imageBuffer.data, imageBuffer.byteSize, sizeof(float), [](uint8_t* buffer){
			delete[] buffer;
		});
		
		globalState->EmitEvent("VolumeUpdate", "", radarData);
	}
}

void ARadarVolumeRender::EndPlay(const EEndPlayReason::Type endPlayReason) {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	// unregister all events
	for(auto id : callbackIds){
		globalState->UnregisterEvent(id);
	}
	callbackIds.clear();
	globalState->refRadarCollection = NULL;
	Super::EndPlay(endPlayReason);
}

//Initialize all textures or reinitialize ones that need it
void ARadarVolumeRender::InitializeTextures() {
	int textureWidth = radarData->radiusBufferCount;
	int textureHeight = (radarData->thetaBufferCount + 2) * radarData->sweepBufferCount;
	if (textureWidth == 0 || textureHeight == 0) {
		// zero size textures will not be properly allocated and can cause crashes
		textureWidth = 16;
		textureHeight = 16;
	}
	if (volumeTexture != NULL && textureWidth == volumeTexture->GetSizeX() && textureHeight == volumeTexture->GetSizeY() && (volumeMaterialRenderTarget != NULL) == doTimeInterpolation) {
		// nothing changed so no need to reallocate
		return;
	}
	EPixelFormat pixelFormat = PF_R32_FLOAT;
	
	// immediately begin destroying to free up gpu memory
	if (volumeTexture2 != NULL) {
		volumeTexture2->ConditionalBeginDestroy();
		volumeTexture2->GetPlatformData()->Mips[0].BulkData.RemoveBulkData();
	}
	if (volumeMaterialRenderTarget != NULL) {
		volumeMaterialRenderTarget->ConditionalBeginDestroy();
	}
	
	// reuse the volume texture to save gpu memory when going from non-interpolated to interpolated and back
	if (volumeTexture == NULL || textureWidth != volumeTexture->GetSizeX() || textureHeight != volumeTexture->GetSizeY()) {
		if (volumeTexture != NULL) {
			volumeTexture->ConditionalBeginDestroy();
			volumeTexture->GetPlatformData()->Mips[0].BulkData.RemoveBulkData();
		}
		// the volume texture contains an array that is interperted as a three dimensional volume of values
		// it corresponds to the real values of the radar
		volumeTexture = UTexture2D::CreateTransient(textureWidth, textureHeight, pixelFormat, TEXT("VolumeTexture1"));
		//FTexture2DMipMap* MipMap = &(volumeTexture->PlatformData->Mips[0]);
		//volumeImageData = &(MipMap->BulkData);
		volumeTexture->UpdateResource();
		radarMaterialInstance->SetTextureParameterValue(TEXT("Volume"), volumeTexture);
		usePrimaryTexture = true;
	}
	
	
	if (doTimeInterpolation) {
		volumeMaterialRenderTarget = UMaterialRenderTarget::Create(textureWidth, textureHeight, pixelFormat, interpolationMaterialInstance, this, "VolumeInterplationRenderTarget");
		volumeMaterialRenderTarget->Update();
		
		volumeTexture2 = UTexture2D::CreateTransient(textureWidth, textureHeight, pixelFormat, TEXT("VolumeTexture2"));
		//MipMap = &(volumeTexture2->PlatformData->Mips[0]);
		//volumeImageData2 = &(MipMap->BulkData);
		volumeTexture2->UpdateResource();

		interpolationMaterialInstance->SetScalarParameterValue(TEXT("Amount"), 0);
		interpolationMaterialInstance->SetScalarParameterValue(TEXT("RadiusSize"), radarData->radiusBufferCount);
		interpolationMaterialInstance->SetScalarParameterValue(TEXT("ThetaSize"), radarData->thetaBufferCount + 2);
		interpolationMaterialInstance->SetScalarParameterValue(TEXT("SweepCount"), radarData->sweepBufferCount);
		interpolationMaterialInstance->SetTextureParameterValue(TEXT("Texture1"), volumeTexture);
		interpolationMaterialInstance->SetTextureParameterValue(TEXT("Texture2"), volumeTexture2);
		radarMaterialInstance->SetTextureParameterValue(TEXT("Volume"), volumeMaterialRenderTarget);
	}else{
		volumeMaterialRenderTarget = NULL;
		volumeTexture2 = NULL;
		radarMaterialInstance->SetTextureParameterValue(TEXT("Volume"), volumeTexture);
	}
	interpolationAnimating = false;
	
	
	
	radarMaterialInstance->SetScalarParameterValue(TEXT("RadiusSize"), radarData->radiusBufferCount);
	radarMaterialInstance->SetScalarParameterValue(TEXT("ThetaSize"), radarData->thetaBufferCount);
	
	
	if(angleIndexTexture == NULL){
		// the angle index texture maps the angle from the ground to a sweep
		// pixel 65536 is strait up, pixel 32768 is parallel with the ground, pixel 0 is strait down
		// values less than zero mean no sweep, value 0.0-255.0 specify the index of the sweep, value 0.0 specifies first sweep, values in-between integers will interpolate sweeps
		angleIndexTexture = UTexture2D::CreateTransient(256, 256, PF_R32_FLOAT, TEXT("AngleIndexTexture"));
		//MipMap = &(angleIndexTexture->PlatformData->Mips[0]);
		//angleIndexImageData = &(MipMap->BulkData);
		radarMaterialInstance->SetTextureParameterValue(TEXT("AngleIndex"), angleIndexTexture);
		angleIndexTexture->UpdateResource();
	}
	
	if(valueIndexTexture == NULL){
		// the value index texture maps the real values of the radar to color and alpha values
		valueIndexTexture = UTexture2D::CreateTransient(128, 128, PF_A32B32G32R32F, TEXT("ColorIndexTexture"));
		//MipMap = &(valueIndexTexture->PlatformData->Mips[0]);
		//valueIndexImageData = &(MipMap->BulkData);
		radarMaterialInstance->SetTextureParameterValue(TEXT("ValueIndex"), valueIndexTexture);
		valueIndexTexture->UpdateResource();
	}
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
	radarCollection->poll = globalState->pollData;
	switch(globalState->animateLoopMode){
		case GlobalState::LOOP_MODE_DEFAULT:
			radarCollection->autoAdvanceEndOption = RadarCollection::AnimationEndJumpCache;
			radarCollection->autoAdvanceCacheOnly = false;
			break;
		case GlobalState::LOOP_MODE_CACHE:
			radarCollection->autoAdvanceEndOption = RadarCollection::AnimationEndJumpCache;
			radarCollection->autoAdvanceCacheOnly = true;
			break;
		case GlobalState::LOOP_MODE_ALL:
			radarCollection->autoAdvanceEndOption = RadarCollection::AnimationEndJumpAll;
			radarCollection->autoAdvanceCacheOnly = false;
			break;
		case GlobalState::LOOP_MODE_BOUNCE:
			radarCollection->autoAdvanceEndOption = RadarCollection::AnimationEndBounce;
			radarCollection->autoAdvanceCacheOnly = false;
			break;
		case GlobalState::LOOP_MODE_CACHE_BOUNCE:
			radarCollection->autoAdvanceEndOption = RadarCollection::AnimationEndBounce;
			radarCollection->autoAdvanceCacheOnly = true;
			break;
		case GlobalState::LOOP_MODE_NONE:
			radarCollection->autoAdvanceEndOption = RadarCollection::AnimationEndStop;
			radarCollection->autoAdvanceCacheOnly = false;
			break;
	}
	if(globalState->temporalInterpolation != doTimeInterpolation){
		doTimeInterpolation = globalState->temporalInterpolation;
		InitializeTextures();
	}

	radarCollection->EventLoop();
	
	
	
	if (volumeTexture != NULL) {
		

		if (interpolationAnimating) {
			if (interpolationEndTime == interpolationStartTime) {
				interpolationMaterialInstance->SetScalarParameterValue(TEXT("Amount"), interpolationEndValue);
				interpolationAnimating = false;
			} else {
				float animationProgress = std::clamp((now - interpolationStartTime) / (interpolationEndTime - interpolationStartTime), 0.0, 1.0);
				float animationValue = interpolationStartValue * (1.0f - animationProgress) + interpolationEndValue * animationProgress;
				interpolationMaterialInstance->SetScalarParameterValue(TEXT("Amount"), animationValue);
				//fprintf(stderr, "%f\n", animationValue);
				if (animationProgress == 1) {
					interpolationAnimating = false;
				}
			}
			// only update render target while animating
			if (volumeMaterialRenderTarget != NULL) {
				volumeMaterialRenderTarget->Update();
			}
		}
		//return;
		//*
		RadarColorIndex::Params colorParams = {};
		colorParams.fromRadarData(radarData);
		//radarColorResult = RadarColorIndex::reflectivityColors(colorParams, &radarColorResult);
		//radarColorResult = RadarColorIndex::velocityColors(colorParams, &radarColorResult);
		radarColorResult = radarColor->GenerateColorIndex(colorParams, &radarColorResult);
		float cutoff = globalState->cutoff;
		if (globalState->animateCutoff) {
			cutoff = (sin(fmod(now, 1 / globalState->animateCutoffSpeed) * globalState->animateCutoffSpeed * PI2F) + 1) / 2 * cutoff;
			//cutoff = abs(fmod(now, globalState->animateCutoffTime) / globalState->animateCutoffTime * -2 + 1) * cutoff;
		}
		radarColor->ModifyOpacity(globalState->opacityMultiplier, cutoff, &radarColorResult);
		//float* rawValueIndexImageData = (float*)valueIndexImageData->Lock(LOCK_READ_WRITE);
		//memcpy(rawValueIndexImageData, valueIndex.data, 16384);
		//memcpy(rawValueIndexImageData, radarColorResult.data, radarColorResult.byteSize);
		//valueIndexImageData->Unlock();
		//valueIndexTexture->UpdateResource();
		UpdateTexture(valueIndexTexture, (uint8_t*)radarColorResult.data, radarColorResult.byteSize, sizeof(float) * 4);

		// set value bounds
		radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexLower"), radarColorResult.lower);
		radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexUpper"), radarColorResult.upper);

		radarMaterialInstance->SetVectorParameterValue(TEXT("ScaleInner"), FVector(
			250.0f / radarData->stats.pixelSize,
			250.0f / radarData->stats.pixelSize,
			250.0f / radarData->stats.pixelSize / globalState->verticalScale
		));
		
		if (doTimeInterpolation) {
			interpolationMaterialInstance->SetScalarParameterValue(TEXT("Minimum"), radarColorResult.lower);
		}
		
		radarMaterialInstance->SetScalarParameterValue(TEXT("FrameIndex"), frameIndex);
		frameIndex = (frameIndex + 1) % 64;
		
		radarMaterialInstance->SetScalarParameterValue(TEXT("SliceMode"), (globalState->viewMode == GlobalState::VIEW_MODE_SLICE && !globalState->sliceVolumetric) ? 1 : 0);
		
		// swap to second buffer to prevent overwiting before the asynchronous texture upload is complete.
		RadarColorIndex::Result oldRadarColorResultAlternate = radarColorResultAlternate;
		radarColorResultAlternate = radarColorResult;
		radarColorResult = oldRadarColorResultAlternate;


		if (globalState->devShowImGui && globalState->devShowCacheState) {
			if (ImGui::Begin("Cache State", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar)) {
				ImGuiIO& io = ImGui::GetIO();
				ImGui::PushFont(io.Fonts->Fonts[1]);
				ImGui::Text("%s", radarCollection->StateString().c_str());
				RadarDataHolder* holder = radarCollection->GetCurrentRadarData();
				ImGui::Text("%s", holder->fileInfo.path.c_str());
				for (auto product : holder->products) {
					bool compressed = product->radarData ? ((product->radarData->buffer == NULL && product->radarData->bufferCompressed != NULL)) ? true : false : false;
					int memoryUsage = product->radarData ? product->radarData->MemoryUsage() : 0;
					ImGui::Text("%c%c%c%c %s    %i bytes", product->isLoaded ? 'L':'_', product->isFinal ? 'F':'_', product->isDependency ? 'D':'_', compressed ? 'C' : '_', product->product->name.c_str(), memoryUsage);
				}
				ImGui::PopFont();
			}
			ImGui::End();
		}
	}
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
	radarColorResultAlternate.Delete();
}



