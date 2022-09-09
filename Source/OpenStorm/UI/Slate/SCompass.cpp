#include "SCompass.h"

#include "UObject/ConstructorHelpers.h"
#include "Brushes/SlateImageBrush.h"
#include "Rendering/SlateRenderTransform.h"
#include "Misc/Paths.h"
#include "SlateUIResources.h"

#include <cmath>

#define PIF 3.14159265358979323846f
#define PIF2 6.283185307179586f


SCompass::SCompass() {
	//roseTexture = MakeShareable(ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("Texture2D'/Game/Textures/crosshair.crosshair'")).Object);
}


void SCompass::Construct(const FArguments& inArgs) {
	SAssignNew(northText, STextBlock);
	SAssignNew(southText, STextBlock);
	SAssignNew(eastText, STextBlock);
	SAssignNew(westText, STextBlock);
	//northSlot = AddSlot();
	//northSlot.AttachWidget(northText.ToSharedRef());

	AddSlot().Anchors(FAnchors(0.5f, 0.5f)).AutoSize(true)[northText.ToSharedRef()];
	AddSlot().Anchors(FAnchors(0.5f, 0.5f)).AutoSize(true)[southText.ToSharedRef()];
	AddSlot().Anchors(FAnchors(0.5f, 0.5f)).AutoSize(true)[eastText.ToSharedRef()];
	AddSlot().Anchors(FAnchors(0.5f, 0.5f)).AutoSize(true)[westText.ToSharedRef()];
	northText->SetText(FText::FromString(TEXT("N")));
	southText->SetText(FText::FromString(TEXT("S")));
	eastText->SetText(FText::FromString(TEXT("E")));
	westText->SetText(FText::FromString(TEXT("W")));
	
	FSlateFontInfo textStyle = FCoreStyle::Get().GetFontStyle("EmbossedText");
	textStyle.Size = size / 6.6f;
	textStyle.OutlineSettings.OutlineSize = size / 50.0f;
	northText->SetFont(textStyle);
	southText->SetFont(textStyle);
	eastText->SetFont(textStyle);
	westText->SetFont(textStyle);


	SAssignNew(rose, SImage);

	//if (roseTexture.IsValid()) {
	//	roseBrush.SetResourceObject(roseTexture.Get());
	///	roseBrush.ImageSize.X = roseTexture->GetSurfaceWidth();
	//	roseBrush.ImageSize.Y = roseTexture->GetSurfaceHeight();
	//	roseBrush.DrawAs = ESlateBrushDrawType::Image;
	//}
	//FVector2D(512, 512)
	//static const FName roseTexture(TEXT("Texture2D'/Game/Textures/crosshair.crosshair'"));
	//FString imagePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Textures/crosshair.png"));
	//roseBrush = FSlateImageBrush(imagePath, FVector2D(size * 0.8, size * 0.8));
	//roseBrush = FSlateDynamicImageBrush(roseTexture, FVector2D(size * 0.8, size * 0.8));
	roseBrush = FSlateImageBrush(USlateUIResources::Instance->crosshairTexture, FVector2D(size * 0.5, size * 0.5));
	rose->SetImage(&roseBrush);
	rose->SetRenderOpacity(0.25);
	AddSlot().Anchors(FAnchors(0.5f, 0.5f)).AutoSize(true)[rose.ToSharedRef()];
	
	Rotate(0);
	//SOverlay::ComputeDesiredSize
	//slot->SetPosition
	//slot.Position(FVector2D(size / 2.0f, size / 2.0f));
}

FVector2D SCompass::ComputeDesiredSize(float layoutScaleMultiplier) const{
	return FVector2D(size, size);
}

void SCompass::Rotate(float rotation) {
	FVector2D center = FVector2D(size / 2.0f, size / 2.0f);
	southText->SetRenderTransform(FVector2D(size * rotation, 0));
	float radians = -(rotation + 0.5) * PIF2;
	float radius = size / 3.0;
	northText->SetRenderTransform(FVector2D(std::cos(radians) * radius, std::sin(radians) * radius));
	eastText-> SetRenderTransform(FVector2D(std::cos(radians + PIF2 * 0.25f) * radius, std::sin(radians + PIF2 * 0.25f) * radius));
	southText->SetRenderTransform(FVector2D(std::cos(radians + PIF2 * 0.50f) * radius, std::sin(radians + PIF2 * 0.50f) * radius));
	westText-> SetRenderTransform(FVector2D(std::cos(radians + PIF2 * 0.75f) * radius, std::sin(radians + PIF2 * 0.75f) * radius));
	rose->SetRenderTransformPivot(FVector2D(0.5,0.5));
	FSlateRenderTransform roseTransform = FSlateRenderTransform(FQuat2D(radians));
	rose->SetRenderTransform(roseTransform);
}






