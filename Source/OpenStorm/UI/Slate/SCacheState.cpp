#include "SCacheState.h"

#include "Brushes/SlateImageBrush.h"
#include "Rendering/SlateRenderTransform.h"
#include "Misc/Paths.h"
#include "SlateUIResources.h"

#include "../../Radar/RadarCollection.h"

SCacheState::SCacheState(){
	
}

void SCacheState::Construct(const FArguments& inArgs) {
	SAssignNew(selector, SImage);
	selector->SetImage(&selectorBrush);
}

FVector2D SCacheState::ComputeDesiredSize(float layoutScaleMultiplier) const {
	return FVector2D(width, height);
	
	
}

// void SCacheState::Tick(float deltaTime) {
// 	UpdateState();
// }

void SCacheState::UpdateState() {
	if(USlateUIResources::Instance != NULL && USlateUIResources::Instance->globalState != NULL  && USlateUIResources::Instance->globalState->refRadarCollection != NULL){
		RadarCollection* radarCollection = USlateUIResources::Instance->globalState->refRadarCollection;
		std::vector<RadarCollection::RadarDataHolder::State> stateVector = radarCollection->StateVector();
		
		if(stateVector.size() != cellCount){
			cellCount = stateVector.size();
			ClearChildren();
			cells.clear();
			for(int i = 0; i < cellCount; i++){
				TSharedPtr<class SImage> cell;
				SAssignNew(cell, SImage);
				AddSlot().Offset(FMargin(width / (float)cellCount * ((float)i + 0.5f), height / 2.0f)).AutoSize(true).Anchors(FAnchors(0.0f, 0.0f))[cell.ToSharedRef()];
				cell->SetImage(&unloadedBrush);
				cell->SetDesiredSizeOverride(FVector2D(width / (float)cellCount - boarder * 2.0f, height - boarder * 2.0f));
				cells.push_back(cell);
				fprintf(stderr,"test %i ", i);
			}
			AddSlot().Offset(FMargin(0, height / 2.0f)).AutoSize(true).Anchors(FAnchors(0.0f, 0.0f))[selector.ToSharedRef()];
			selector->SetDesiredSizeOverride(FVector2D(width / (float)cellCount, height / 2.0f));
			// TSharedPtr<class SImage> background;
			// SAssignNew(background, SImage);
			// AddSlot(-1)[background.ToSharedRef()];
			// background->SetImage(backgroundBrush);
		}
		
		selector->SetRenderTransform(FVector2D(width / (float)cellCount * ((float)radarCollection->GetCurrentPosition() + 0.5f),0));
		
		for(int i = 0; i < cellCount; i++){
			switch(stateVector[i]){
				case RadarCollection::RadarDataHolder::State::DataStateUnloaded:
					cells[i]->SetImage(&unloadedBrush);
				break;
				case RadarCollection::RadarDataHolder::State::DataStateLoaded:
					cells[i]->SetImage(&loadedBrush);
				break;
				case RadarCollection::RadarDataHolder::State::DataStateLoading:
					cells[i]->SetImage(&loadingBrush);
				break;
				case RadarCollection::RadarDataHolder::State::DataStateFailed:
					cells[i]->SetImage(&errorBrush);
				break;
			}
		}
	}
}
