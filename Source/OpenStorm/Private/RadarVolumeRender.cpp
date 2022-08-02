// Fill out your copyright notice in the Description page of Project Settings.




#include "RadarVolumeRender.h"
#include "StdioConsole.h"


//#define VERBOSE 1
#include "../Radar/RadarData.h"


#include "TestTexture.h"
#include "Engine/Texture2D.h"
#include "UObject/Object.h"
#include "Math/UnrealMathUtility.h"

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}



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



	cubeMeshComponent->SetStaticMesh(cubeMesh);

	cubeMeshComponent->SetMaterial(0, storedMaterial);

	// Set as root component
	RootComponent = cubeMeshComponent;

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

	radarMaterialInstance = UMaterialInstanceDynamic::Create(cubeMeshComponent->GetMaterial(0), cubeMesh);
	cubeMeshComponent->SetMaterial(0, radarMaterialInstance);

	radarMaterialInstance->SetVectorParameterValue(TEXT("Center"), this->GetActorLocation());


	FString radarFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Data/Demo/KMKX_20220723_235820"));
	FString fullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*radarFile);
	RadarData radarData = {};
	const char* fileLocaition = StringCast<ANSICHAR>(*fullPath).Get();
	radarData.ReadNexrad(fileLocaition);

	if (radarData.radiusBufferCount == 0) {
		fprintf(stderr, "Could not open %s\n", fileLocaition);
		return;
	}

	// PF_B8G8R8A8 - 4 uint8 values per pixel
	// PF_R32_FLOAT - single float value per pixel
	// PF_FloatRGB - 3 float values per pixel
	// PF_FloatRGBA - 4 float values per pixel
	volumeTexture = UTexture2D::CreateTransient(radarData.radiusBufferCount, (radarData.thetaBufferCount + 2) * radarData.sweepBufferCount, PF_B8G8R8A8);
	FTexture2DMipMap* MipMap = &(volumeTexture->PlatformData->Mips[0]);
	volumeImageData = &(MipMap->BulkData);
	radarMaterialInstance->SetTextureParameterValue(TEXT("Volume"), volumeTexture);


	


	/*Radar* radar = RSL_wsr88d_to_radar("C:/Users/Admin/Desktop/stuff/projects/openstorm/files/KMKX_20220723_235820", "KMKX");
	
	fprintf(stderr, "hello stderr\n");
	UE_LOG(LogTemp, Display, TEXT("================ %i"), radar);
	if (radar) {
		Volume* volume = radar->v[DZ_INDEX];
		if (volume) {
			fprintf(stderr, "volume type_str %s\n", volume->h.type_str);
			fprintf(stderr, "volume nsweeps %i\n", volume->h.nsweeps);
			for (int sweepIndex = 0; sweepIndex < volume->h.nsweeps; sweepIndex++) {
				Sweep* sweep = volume->sweep[sweepIndex];
				fprintf(stderr, "======== %i\n", sweepIndex);
				fprintf(stderr, "sweep elev %f\n", sweep->h.elev);
				fprintf(stderr, "sweep sweep_num %i\n", sweep->h.sweep_num);
				fprintf(stderr, "sweep nrays %i\n", sweep->h.nrays);
				if (sweep->ray[0]) {
					fprintf(stderr, "sweep nbins %i\n", sweep->ray[0]->h.nbins);
				}
				fprintf(stderr, "sweep azimuth %f\n", sweep->h.azimuth);
				fprintf(stderr, "sweep beam_width %f\n", sweep->h.beam_width);
				fprintf(stderr, "sweep horz_half_bw %f\n", sweep->h.horz_half_bw);
				fprintf(stderr, "sweep vert_half_bw %f\n", sweep->h.vert_half_bw);
				if (sweep->ray[0]) {
					fprintf(stderr, "sweep bin 0 data %i\n", sweep->ray[0]->range[0]);
				}
			}

		}
	}*/


	/*UMaterialInstanceDynamic* DynamicMaterial = cubeMeshComponent->CreateDynamicMaterialInstance(0, cubeMeshComponent->GetMaterial(0));
	DynamicMaterial->SetTextureParameterValue("Texture", CustomTexture);
	cubeMeshComponent->SetMaterial(0, DynamicMaterial);*/



	// the angle index texture maps the angle from the ground to a sweep
	// pixel 65536 is strit up, pixel 32768 is paralel with the ground, pixel 0 is strait down
	// value 0 is no sweep, value 1.0-255.0 specify the index of the sweep, value 1.0 specifies first sweep, values inbetween intagers will interpolate sweeps
	angleIndexTexture = UTexture2D::CreateTransient(256, 256, PF_R32_FLOAT);
	MipMap = &(angleIndexTexture->PlatformData->Mips[0]);
	angleIndexImageData = &(MipMap->BulkData);
	radarMaterialInstance->SetTextureParameterValue(TEXT("AngleIndex"), angleIndexTexture);


	

	radarMaterialInstance->SetScalarParameterValue(TEXT("InnerDistance"), radarData.innerDistance);

	RadarData::TextureBuffer imageBuffer = radarData.CreateTextureBufferReflectivity();

	uint8* RawImageData = (uint8*)volumeImageData->Lock(LOCK_READ_WRITE);

	memcpy(RawImageData, imageBuffer.data.uint8Array, imageBuffer.byteSize);

	volumeImageData->Unlock();
	volumeTexture->UpdateResource();

	delete[] imageBuffer.data.uint8Array;



	imageBuffer = radarData.CreateAngleIndexBuffer();

	float* rawAngleIndexImageData = (float*)angleIndexImageData->Lock(LOCK_READ_WRITE);

	memcpy(rawAngleIndexImageData, imageBuffer.data.floatArray, imageBuffer.byteSize);

	angleIndexImageData->Unlock();
	angleIndexTexture->UpdateResource();

	delete[] imageBuffer.data.floatArray;

	/*
	rawAngleIndexImageData = (float*)angleIndexImageData->Lock(LOCK_READ_WRITE);
	uint8* fdsa = (uint8 *)rawAngleIndexImageData;
	for (int i = 0; i < 65536; i++) {
		if (i >= 32768) {
			float sweep = ((i - 32768) * 32) / 32768.0;
			//if (!((int)sweep - sweep < 0.2 && sweep - (int)sweep < 0.2)) {
			//	sweep = 0;
			//}
			rawAngleIndexImageData[i] = sweep;
		} else {
			rawAngleIndexImageData[i] = 0;
		}
		//rawAngleIndexImageData[i] = 0;
	}
	//for (int i = 0; i < 65536 * 4; i++) {
	//	fdsa[i] = 0.3;
	//}
	angleIndexImageData->Unlock();
	angleIndexTexture->UpdateResource();

	/ *
	//UE_LOG(LogTemp, Display, TEXT("================"));

	// maximum radius
	int radiusBufferCount = 1832;
	int radiusBufferSize = radiusBufferCount * 4;
	// number of rotations
	int thetaBufferCount = 720;
	int thetaRadiusBufferSize = thetaBufferCount * radiusBufferSize;

	
		
	
	
	uint8* RawImageData = (uint8*)volumeImageData->Lock(LOCK_READ_WRITE);
	int setBytes = 0;
	if (radar) {
		Volume* volume = radar->v[DZ_INDEX];
		if (volume) {
			for (int sweepIndex2 = 0; sweepIndex2 < 16 ; sweepIndex2++) {
				if (sweepIndex2 == 8 || sweepIndex2 == 9) {
					continue;
				}
				int sweepIndex = sweepIndex2;
				if (sweepIndex2 > 9) {
					sweepIndex = sweepIndex2 - 2;
				}
				Sweep* sweep = volume->sweep[sweepIndex2];

				if (sweep) {
					int thetaSize = sweep->h.nrays;
					int minValue = 65536;
					int maxValue = 0;
					// get stats on sweep values
					for (int theta = 0; theta < thetaSize; theta++) {
						Ray* ray = sweep->ray[theta];
						if (ray) {
							int radiusSize = ray->h.nbins;
							for (int radius = 0; radius < radiusSize; radius++) {
								int value = ray->range[radius];
								minValue = value != 0 ? (value < minValue ? value : minValue) : minValue;
								//minValue = value < minValue ? value : minValue;
								maxValue = value > maxValue ? value : maxValue;
							}
						}
					}
					bool* usedThetas = new bool[thetaBufferCount]();
					int divider = (maxValue - minValue) / 256 + 1;
					fprintf(stderr, "min: %i   max: %i   devider: %i\n", minValue, maxValue, divider);
					// fill in buffer from rays
					for (int theta = 0; theta < thetaSize; theta++) {
						Ray* ray = sweep->ray[theta];
						if (ray) {
							// get real angle of ray
							int realTheta = (int)((ray->h.azimuth * 2.0) + thetaBufferCount) % thetaBufferCount;
							usedThetas[realTheta] = true;
							int radiusSize = min(ray->h.nbins, radiusBufferCount);
							for (int radius = 0; radius < radiusSize; radius++) {
								int value = (ray->range[radius] - minValue) / divider;
								//if (theta == 0) {
								//	value = 255;
								//}
								RawImageData[3 + radius * 4 + realTheta * radiusBufferSize + sweepIndex * thetaRadiusBufferSize] = max(0, value);
								//RawImageData[3 + radius * 4 + theta * radiusSize * 4] = 0;
								setBytes++;
							}
						}
						//break;
					}
					for (int theta = 0; theta < thetaBufferCount; theta++) {
						if (!usedThetas[theta]) {
							// fill in blank by interpolating suroundings
							int previousRay = 0;
							int nextRay = 0;
							if (usedThetas[modulo(theta - 3, thetaBufferCount)]) {
								previousRay = -3;
							}
							if (usedThetas[modulo(theta - 2, thetaBufferCount)]) {
								previousRay = -2;
							}
							if (usedThetas[modulo(theta - 1, thetaBufferCount)]) {
								previousRay = -1;
							}
							if (usedThetas[modulo(theta + 3, thetaBufferCount)]) {
								nextRay = 4;
							}
							if (usedThetas[modulo(theta + 2, thetaBufferCount)]) {
								nextRay = 2;
							}
							if (usedThetas[modulo(theta + 1, thetaBufferCount)]) {
								nextRay = 1;
							}
							if (previousRay != 0 && nextRay != 0) {
								int previousRayAbs = modulo(theta + previousRay, thetaBufferCount);
								int nextRayAbs = modulo(theta + nextRay, thetaBufferCount);
								for (int radius = 0; radius < radiusBufferCount * 4; radius++) {
									int previousValue = RawImageData[radius + previousRayAbs * radiusBufferSize + sweepIndex * thetaRadiusBufferSize];
									int nextValue = RawImageData[radius + nextRayAbs * radiusBufferSize + sweepIndex * thetaRadiusBufferSize];
									for (int thetaTo = previousRay + 1; thetaTo <= nextRay - 1; thetaTo++) {
										float interLocation = (float)(thetaTo - previousRay) / (float)(nextRay - previousRay);
										// write interpolated value
										RawImageData[radius + modulo(theta + thetaTo, thetaBufferCount) * radiusBufferSize + sweepIndex * thetaRadiusBufferSize] = previousValue * (1.0 - interLocation) + nextValue * interLocation;
									}
								}
								for (int thetaTo = previousRay + 1; thetaTo <= nextRay - 1; thetaTo++) {
									// mark as filled
									usedThetas[modulo(theta + thetaTo, thetaBufferCount)] = true;
								}
							}
						}
					}
					delete usedThetas;
				}
			}
		}
	}
	fprintf(stderr, "set bytes: %i\n", setBytes);
	volumeImageData->Unlock();
	volumeTexture->UpdateResource();
	
	*/

	

	//RandomizeTexture();
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
	//return;
	
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

