#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Materials/Material.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "MaterialRenderTarget.generated.h"

UCLASS()
class UMaterialRenderTarget : public UCanvasRenderTarget2D {

	GENERATED_BODY()

public:
	UMaterialInterface* renderMaterial;
	
	UMaterialRenderTarget();
	
	

	static UMaterialRenderTarget* Create(int width, int height, EPixelFormat pixelFormat, UMaterialInterface* material, UObject* parent);

	void Initialize(int width, int height, EPixelFormat pixelFormat);

	void Update();

	UFUNCTION()
	void DrawMaterial(UCanvas* canvas, int32 width, int32 height);
	//virtual void ReceiveUpdate(UCanvas* Canvas, int32 Width, int32 Height);
};