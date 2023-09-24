#pragma once

#include "./Data/ShapeFile.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "GISPolyline.generated.h"

class Globe;
class GISObject;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class AGISPolyline : public AActor{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere)
	UMaterialInterface* material;
	
	UPROPERTY(VisibleAnywhere)
	UMaterialInstanceDynamic* materialInstance = NULL;
	
	UPROPERTY(EditAnywhere);
	UProceduralMeshComponent* proceduralMesh;
	
	// update position of all meshes after globe update
	void DisplayObject(GISObject* object, GISGroup* group);
	
	// position object onto a globe
	void PositionObject(Globe* globe);
	
	AGISPolyline();
	~AGISPolyline();
	
	virtual void BeginPlay() override;
	//virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	//virtual void Tick(float DeltaTime) override;
	
};