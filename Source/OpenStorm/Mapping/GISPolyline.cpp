
#include "GISPolyline.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "../Objects/RadarGameStateBase.h"
#include "../Application/GlobalState.h"
#include "../Radar/Globe.h"
#include "./Data/ShapeFile.h"
#include "./Data/ElevationData.h"

AGISPolyline::AGISPolyline(){
	material = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/Script/Engine.Material'/Game/Materials/GISLine.GISLine'")).Object;
	materialInstance = UMaterialInstanceDynamic::Create(material, this, "GISLineMaterialInstance");
	proceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	proceduralMesh->bUseAsyncCooking = true;
	proceduralMesh->SetMaterial(0, materialInstance);
	RootComponent = proceduralMesh;
}

AGISPolyline::~AGISPolyline(){
	
}

void AGISPolyline::BeginPlay(){
	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();
}

inline FVector SimpleVector3ToFVector(SimpleVector3<float> vec){
	return FVector(vec.x, vec.y, vec.z);
}

void AGISPolyline::DisplayObject(GISObject* object, GISGroup* group){
	static Globe globe = {};
	
	TArray<FVector> vertices = {};
	TArray<FVector> normals = {};
	TArray<int> triangles = {};
	TArray<FVector2D> uv0 = {};
	vertices.Empty(object->geometryCount * 1.5);
	normals.Empty(object->geometryCount * 1.5);
	triangles.Empty(object->geometryCount * 3 * 1.5);
	uv0.Empty(object->geometryCount  * 1.5);
	
	float width = group->width / 2.0f;
	bool hasDoneFirstPoint = false;
	int64_t initialPointIndex = 0;
	int64_t currentPointIndex = 0;
	while(currentPointIndex < object->geometryCount - 1){
		float pointPart1 = object->geometry[currentPointIndex];
		if(!std::isfinite(pointPart1)){
			// NaN detected, stop current line and begin next
			currentPointIndex++;
			initialPointIndex = currentPointIndex;
			hasDoneFirstPoint = false;
			continue;
		}
		int64_t nextIndex = std::min(currentPointIndex + 2, (int64_t)object->geometryCount - 2);
		float nextPart1 = object->geometry[nextIndex];
		if(!std::isfinite(nextPart1)){
			// next part is a NaN separator so use current point
			nextIndex = currentPointIndex;
		}
		SimpleVector3<float> nextPoint = globe.GetPointScaledDegrees(object->geometry[nextIndex],object->geometry[nextIndex + 1],0);
		int64_t previousIndex = std::max(currentPointIndex - 2, initialPointIndex);
		SimpleVector3<float> previousPoint = globe.GetPointScaledDegrees(object->geometry[previousIndex],object->geometry[previousIndex + 1],0);
		//float elevation = ElevationData::GetDataAtPointRadians(object->geometry[currentPointIndex] / 180.0f * PI, object->geometry[currentPointIndex + 1] / 180.0f * PI);
		SimpleVector3<float> point = globe.GetPointScaledDegrees(object->geometry[currentPointIndex], object->geometry[currentPointIndex + 1], 1000 /*+ elevation*/);
		currentPointIndex += 2;
		
		SimpleVector3<float> direction = point;
		direction.Subtract(previousPoint);
		SimpleVector3<float> direction2 = nextPoint;
		direction2.Subtract(point);
		if(!std::isfinite(direction.x)){
			direction = direction2;
		}
		if(!std::isfinite(direction2.x)){
			direction2 = direction;
		}
		
		// outwards for making line thick
		SimpleVector3<float> outwards = direction;
		outwards.Normalize();
		direction2.Normalize();
		outwards.Add(direction2);
		outwards.Cross(point);
		// outwards.ProjectOntoPlane(point);
		outwards.Normalize();
		outwards.Multiply(width);
		
		// segment very long single lines so they do not clip
		float distance = direction.Magnitude();
		int subsectionCount = std::max((int)(distance / 100), 1);
		for(int i = 1; i <= subsectionCount; i++){
			SimpleVector3 subPoint = previousPoint;
			SimpleVector3 pointOffset = direction;
			pointOffset.Multiply(i / (float)subsectionCount);
			subPoint.Add(pointOffset);
			subPoint = globe.GetLocationScaled(subPoint);
			float elevation = ElevationData::GetDataAtPointRadians(-subPoint.z, subPoint.y + PI);
			subPoint.radius() = 500 + elevation;
			subPoint = globe.GetPointScaled(subPoint);
			vertices.Add(SimpleVector3ToFVector(subPoint + outwards));
			vertices.Add(SimpleVector3ToFVector(subPoint - outwards));
			uv0.Add(FVector2D(0,0));
			uv0.Add(FVector2D(0,1));
			point.Normalize();
			normals.Add(SimpleVector3ToFVector(point));
			normals.Add(SimpleVector3ToFVector(point));
			
			if(hasDoneFirstPoint){
				int vertexCount = vertices.Num();
				// triangle 1
				triangles.Add(vertexCount - 4);
				triangles.Add(vertexCount - 3);
				triangles.Add(vertexCount - 2);
				// triangle 2
				triangles.Add(vertexCount - 2);
				triangles.Add(vertexCount - 3);
				triangles.Add(vertexCount - 1);
			}
			hasDoneFirstPoint = true;
		}
	}
	
	if(triangles.Num() > 0){
		proceduralMesh->CreateMeshSection(0, vertices, triangles, normals, uv0, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
		// proceduralMesh->SetMaterial(0, materialInstance);
		materialInstance->SetVectorParameterValue(TEXT("Color"), FVector4(group->colorR,group->colorG,group->colorB,1));
	}
}

void AGISPolyline::PositionObject(Globe* globe){
	SimpleVector3 center = SimpleVector3<>(globe->center);
	center.Multiply(globe->scale);
	SetActorLocationAndRotation(SimpleVector3ToFVector(center), FQuat(FVector(1, 0, 0), -globe->rotationAroundX) * FQuat(FVector(0, 0, 1), -globe->rotationAroundPolls));
}

void AGISPolyline::SetBrightness(float brightness){
	materialInstance->SetScalarParameterValue(TEXT("Brightness"), brightness);
}