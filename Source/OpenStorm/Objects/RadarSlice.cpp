#include "RadarSlice.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Modules/ModuleManager.h"
#include "Engine/World.h"

#include "../Radar/SimpleVector3.h"
#include "../Radar/AsyncTask.h"
#include "../Radar/Globe.h"
#include "../Application/GlobalState.h"
#include "./RadarVolumeRender.h"
#include "./RadarGameStateBase.h"

#include <cmath>
#include <algorithm>

#define M_PI       3.14159265358979323846
#define M_PIF       3.14159265358979323846f


template <class T>
T* ARadarSlice::FindActor() {
	TArray<AActor*> FoundActors = {};
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), T::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0) {
		return (T*)FoundActors[0];
	} else {
		return NULL;
	}
}


ARadarSlice::ARadarSlice(){
	proceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	proceduralMesh->bUseAsyncCooking = true;
	proceduralMesh->TranslucencySortPriority = 1;
	RootComponent = proceduralMesh;
}

template <class T>
inline FVector SimpleVectorToFVector(SimpleVector3<T> vec){
	return FVector(vec.x, vec.y, vec.z);
}

void ARadarSlice::BeginPlay(){
	static Globe defaultGlobe = {};
	//defaultGlobe.scale = 1.0 / 10000.0 * 100.0;
	//defaultGlobe.SetCenter(0, 0, -defaultGlobe.surfaceRadius);
	//defaultGlobe.SetTopCoordinates(45, 0);
	//defaultGlobe.SetTopCoordinates(27.9879493, 86.9172718); //mt everest
	//defaultGlobe.SetTopCoordinates(63.06630592, -151.00722251); //mt Denali
	//defaultGlobe.SetTopCoordinates(45.8991878, -115.0331426); //mountains in america
	//defaultGlobe.SetTopCoordinates(42.967900, -88.550667);// KMKX
	if(globe == NULL){
		globe = &defaultGlobe;
	}
	PrimaryActorTick.bCanEverTick = true;
	mainVolumeRender = FindActor<ARadarVolumeRender>();
	Super::BeginPlay();
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode == NULL){
		return;
	}
	GlobalState* globalState = &gameMode->globalState;
	
	callbackIds.push_back(globalState->RegisterEvent("GlobeUpdate",[this](std::string stringData, void* extraData){
		GenerateMesh();
	}));
	callbackIds.push_back(globalState->RegisterEvent("VolumeUpdate",[this](std::string stringData, void* extraData){
		RadarData* radarData = (RadarData*)extraData;
		radius = radarData->stats.boundRadius * radarData->stats.pixelSize / 10000.0f * 100.0f * 1.1f;
		radiusMax = (radarData->radiusBufferCount + radarData->stats.innerDistance) * radarData->stats.pixelSize / 10000.0f * 100.0f * 1.1f;
		topHeight = radarData->stats.boundUpper * radarData->stats.pixelSize / 10000.0f * 100.0f + 100;
		bottomHeight = radarData->stats.boundLower * radarData->stats.pixelSize / 10000.0f * 100.0f - 50;
		GenerateMesh();
	}));
	callbackIds.push_back(globalState->RegisterEvent("CameraMove",[this](std::string stringData, void* extraData){
		cameraLocation = *(SimpleVector3<float>*)extraData;
		SimpleVector3<float> cameraSpherical = cameraLocation;
		cameraSpherical.RectangularToSpherical();
		float cameraAngle = cameraSpherical.theta();
		if(sliceMode == GlobalState::SLICE_MODE_SWEEP_ANGLE && std::abs(cameraAngle - lastCameraAngle) > M_PI * 2 / sectors){
			lastCameraAngle = cameraAngle;
			GenerateMesh();
		}
	}));
	
	GenerateMesh();
}

void ARadarSlice::EndPlay(const EEndPlayReason::Type endPlayReason) {
	Super::EndPlay(endPlayReason);
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode == NULL){
		return;
	}
	GlobalState* globalState = &gameMode->globalState;
	for(auto id : callbackIds){
		globalState->UnregisterEvent(id);
	}
	callbackIds.clear();
}

void ARadarSlice::Tick(float DeltaTime){
	//GenerateMesh();
	//ProjectionOntoGlobeTest();
	if (radarMaterialInstance == NULL) {
		if (mainVolumeRender != NULL) {
			if (mainVolumeRender->radarMaterialInstance != NULL) {
				radarMaterialInstance = mainVolumeRender->radarMaterialInstance;
				proceduralMesh->SetMaterial(0, radarMaterialInstance);
			}
		}
	}
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode == NULL){
		return;
	}
	GlobalState* globalState = &gameMode->globalState;
	globe = globalState->globe;
	
	
	if(globalState->viewMode == GlobalState::VIEW_MODE_SLICE){
		bool regenerateMesh = false;
		
		if(globalState->sliceAltitude != sliceAltitude){
			sliceAltitude = globalState->sliceAltitude;
			regenerateMesh = true;
		}
		if(globalState->sliceAngle != sliceAngle){
			sliceAngle = globalState->sliceAngle;
			regenerateMesh = true;
		}
		if(globalState->sliceMode != sliceMode){
			sliceMode = globalState->sliceMode;
			regenerateMesh = true;
		}
		if(globalState->sliceVerticalLocationX != locationX){
			locationX = globalState->sliceVerticalLocationX;
			regenerateMesh = true;
		}
		if(globalState->sliceVerticalLocationY != locationY){
			locationY = globalState->sliceVerticalLocationY;
			regenerateMesh = true;
		}
		if(globalState->sliceVerticalRotation != rotation){
			rotation = globalState->sliceVerticalRotation;
			regenerateMesh = true;
		}
		
		
		if(regenerateMesh){
			GenerateMesh();
		}
		SetActorHiddenInGame(false);
	}else{
		SetActorHiddenInGame(true);
	}
}


void ARadarSlice::GenerateMesh(){
	TArray<FVector> vertices = {};
	TArray<FVector> normals = {};
	TArray<int> triangles = {};
	TArray<FVector2D> uv0 = {};
	if(sliceMode == GlobalState::SLICE_MODE_CONSTANT_ALTITUDE){
		// create section of a sphere concentric to earth with a difference in radius equal to sliceAltitude
		
		divisions = 30;
		vertices.SetNum((divisions + 1) * (divisions + 1));
		normals.SetNum((divisions + 1) * (divisions + 1));
		uv0.SetNum((divisions + 1) * (divisions + 1));
		triangles.Empty((divisions) * (divisions) * 6 * 2);
		
		// SimpleVector3<float> testVec = SimpleVector3<float>(1,2,3);
		// testVec.RectangularToSpherical();
		// testVec.SphericalToRectangular();
		// fprintf(stderr, "test %f %f %f \n", testVec.x, testVec.y, testVec.z);
		
		for (int x = 0; x <= divisions; x++) {
			for (int y = 0; y <= divisions; y++) {
				int loc = y * (divisions + 1) + x;
				SimpleVector3<double> vert = SimpleVector3<double>(
					(x / (float)divisions - 0.5f) * radius * 2.0f,
					(y / (float)divisions - 0.5f) * radius * 2.0f,
					0
				);
				
				vert = globe->GetLocationScaled(vert);
				vert.radius() = sliceAltitude;
				vert = globe->GetPointScaled(vert);
				
				//fprintf(stderr, "loc %f %f %f\n", vert.x, vert.y, vert.z);
				vertices[loc] = FVector(vert.x, vert.y, vert.z);
				
				vert.Subtract(globe->center);
				vert.Multiply(1.0f / vert.Magnitude());
				normals[loc] = FVector(vert.x, vert.y, vert.z);
				
				uv0[loc] = FVector2D(x / (float)divisions, y / (float)divisions);
			}
		}
		
		for (int x = 0; x < divisions; x++) {
			for (int y = 0; y < divisions; y++) {
				int loc1 = y * (divisions + 1) + x;
				int loc2 = y * (divisions + 1) + x + 1;
				int loc3 = (y + 1) * (divisions + 1) + x;
				int loc4 = (y + 1) * (divisions + 1) + x + 1;
				int triangleLoc = (y * (divisions + 1) + x) * 6;
				
				// first triangle of quad
				triangles.Add(loc1);
				triangles.Add(loc3);
				triangles.Add(loc2);
				// first triangle of quad backside
				triangles.Add(loc2);
				triangles.Add(loc3);
				triangles.Add(loc1);
				
				// second triangle of quad
				triangles.Add(loc2);
				triangles.Add(loc3);
				triangles.Add(loc4);
				// second triangle of quad backside
				triangles.Add(loc4);
				triangles.Add(loc3);
				triangles.Add(loc2);
			}
		}
	}
	if(sliceMode == GlobalState::SLICE_MODE_SWEEP_ANGLE){
		// create a cone with its tip located at the radar and an angle from flat of sliceAngle
		
		vertices.SetNum(sectors + 1);
		normals.SetNum(sectors + 1);
		uv0.SetNum(sectors + 1);
		int triangleCount = (sectors) * 3 * 2;
		triangles.Empty(triangleCount);
		TArray<int> trianglesTemp = {};
		trianglesTemp.Empty(triangleCount);
		
		vertices[0] = FVector(0, 0, 0);
		uv0[0] = FVector2D(0.5f, 0.5f);
		normals[0] = FVector(0, 0, 1);
		
		for (int i = 0; i < sectors; i++) {
			SimpleVector3<float> vert = SimpleVector3<float>(radius, 0, 0);
			vert.RotateAroundY(-sliceAngle / 180.0f * M_PIF);
			vert.RotateAroundZ(i / (float)sectors * 2.0f * M_PIF);
			
			vertices[i + 1] = FVector(vert.x, vert.y, vert.z);
			uv0[i + 1] = FVector2D(vert.x / radius + 0.5, vert.y / radius + 0.5);
			normals[i + 1] = FVector(0, 0, 1);
		}
		for (int i = 0; i < sectors - 1; i++) {
			// front triangle
			trianglesTemp.Add(i + 1);
			trianglesTemp.Add(i + 2);
			trianglesTemp.Add(0);
			// back triangle
			trianglesTemp.Add(0);
			trianglesTemp.Add(i + 2);
			trianglesTemp.Add(i + 1);
		}
		// add last triangle front
		trianglesTemp.Add(sectors + 1);
		trianglesTemp.Add(1);
		trianglesTemp.Add(0);
		// add last triangle back
		trianglesTemp.Add(0);
		trianglesTemp.Add(1);
		trianglesTemp.Add(sectors + 1);
		
		// sort triangles so that they overlap properly
		int startIndex = -lastCameraAngle / 2.0f / M_PIF * sectors + sectors * 2.5;
		int flipDirection = 1;
		int startDistance = 0;
		for(int i = 0; i < sectors; i++){
			int loc = ((startDistance * flipDirection + startIndex) * 6) % triangleCount;
			triangles.Add(trianglesTemp[loc + 0]);
			triangles.Add(trianglesTemp[loc + 1]);
			triangles.Add(trianglesTemp[loc + 2]);
			triangles.Add(trianglesTemp[loc + 3]);
			triangles.Add(trianglesTemp[loc + 4]);
			triangles.Add(trianglesTemp[loc + 5]);
			if(i % 2 == 0){
				startDistance++;
			}
			flipDirection = -flipDirection;
		}
	}
	
	if(sliceMode == GlobalState::SLICE_MODE_VERTICAL){
		SimpleVector3<float> offset = SimpleVector3<float>(radiusMax * locationX, -radiusMax * locationY, 0);
		vertices.SetNum(4);
		normals.SetNum(4);
		uv0.SetNum(4);
		triangles.Empty(3);
		// SimpleVector3<float> sideways =  SimpleVector3<float>(radiusMax * 2 + offset.Magnitude(), 0, 0);
		// sideways.RotateAroundZ(rotation / 180.0f * M_PIF);
		// SimpleVector3<float> top = SimpleVector3<float>(0, 0, topHeight);
		// SimpleVector3<float> bottom = SimpleVector3<float>(0, 0, bottomHeight);
		SimpleVector3<float> sideways =  SimpleVector3<float>(30000, 0, 0);
		sideways.RotateAroundZ(rotation / 180.0f * M_PIF);
		SimpleVector3<float> top = SimpleVector3<float>(0, 0, 10000);
		SimpleVector3<float> bottom = SimpleVector3<float>(0, 0, -10000);
		vertices[0] = SimpleVectorToFVector(offset + sideways + bottom);
		vertices[1] = SimpleVectorToFVector(offset - sideways + bottom);
		vertices[2] = SimpleVectorToFVector(offset - sideways + top);
		vertices[3] = SimpleVectorToFVector(offset + sideways + top);
		
		// front triangle 1
		triangles.Add(0);
		triangles.Add(1);
		triangles.Add(2);
		// front triangle 2
		triangles.Add(0);
		triangles.Add(2);
		triangles.Add(3);
		// back triangle 1
		triangles.Add(2);
		triangles.Add(1);
		triangles.Add(0);
		// vakc triangle 2
		triangles.Add(3);
		triangles.Add(2);
		triangles.Add(0);
	}
	proceduralMesh->CreateMeshSection(0, vertices, triangles, normals, uv0, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
}