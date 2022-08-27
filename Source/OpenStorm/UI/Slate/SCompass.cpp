#include "SCompass.h"
#include <cmath>


#define PIF 3.14159265358979323846f
#define PIF2 6.283185307179586f


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
	float radians = -(rotation + 0.25) * PIF2;
	float radius = size / 3.0;
	northText->SetRenderTransform(FVector2D(std::cos(radians) * radius, std::sin(radians) * radius));
	eastText-> SetRenderTransform(FVector2D(std::cos(radians + PIF2 * 0.25f) * radius, std::sin(radians + PIF2 * 0.25f) * radius));
	southText->SetRenderTransform(FVector2D(std::cos(radians + PIF2 * 0.50f) * radius, std::sin(radians + PIF2 * 0.50f) * radius));
	westText-> SetRenderTransform(FVector2D(std::cos(radians + PIF2 * 0.75f) * radius, std::sin(radians + PIF2 * 0.75f) * radius));
}






