#include "MaterialRenderTarget.h"
#include "Engine/Canvas.h"

UMaterialRenderTarget* UMaterialRenderTarget::Create(int width, int height, EPixelFormat pixelFormat, UMaterialInterface* material, UObject* parent){
	UMaterialRenderTarget* renderTarget = (UMaterialRenderTarget*)NewObject<class UMaterialRenderTarget>(parent);
	renderTarget->renderMaterial = material;
	renderTarget->Initialize(width, height, pixelFormat);
	return renderTarget;
}

UMaterialRenderTarget::UMaterialRenderTarget(){
	OnCanvasRenderTargetUpdate.AddDynamic(this, &UMaterialRenderTarget::DrawMaterial);
}

void UMaterialRenderTarget::Initialize(int width, int height, EPixelFormat pixelFormat){
	//InitCustomFormat(width, height, pixelFormat, true);
	
	// overide without arbitrary pixel format restrictions
	OverrideFormat = pixelFormat;
	bForceLinearGamma = true;
	InitAutoFormat(width, height);
	UpdateResource();
}

void UMaterialRenderTarget::Update(){
	//fprintf(stderr,"Update2 %i\n",OnCanvasRenderTargetUpdate.IsBound());
	RepaintCanvas();
	//UpdateResource();
	//ReceiveUpdate(NULL,1,1);
}

//void UMaterialRenderTarget::ReceiveUpdate(UCanvas* canvas, int32 width, int32 height){
	//DrawMaterial(canvas, width, height);
//}


void UMaterialRenderTarget::DrawMaterial(UCanvas* canvas, int32 width, int32 height){
	//fprintf(stderr,"Update %p %i %i\n",canvas, width, height);
	//return;
	//canvas->K2_DrawMaterial(renderMaterial, FVector2D(0, 0), FVector2D(1, 1), FVector2D(0, 0), FVector2D(width, height), 0, FVector2D(0, 0));
	canvas->K2_DrawMaterial(renderMaterial, FVector2D(0, 0), FVector2D(width, height), FVector2D(0, 0));
}
