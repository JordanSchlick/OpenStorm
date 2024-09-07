#include "SCacheState.h"

#include "Brushes/SlateImageBrush.h"
#include "Rendering/SlateRenderTransform.h"
#include "Misc/Paths.h"
#include "Widgets/Text/STextBlock.h"
#include "SlateUIResources.h"

#include "../../Radar/RadarCollection.h"

SCacheState::SCacheState(){
	
}

void SCacheState::Construct(const FArguments& inArgs) {
	SAssignNew(selector, SImage);
	SAssignNew(fileName, STextBlock);

	selector->SetImage(&selectorBrush);

	FSlateFontInfo textStyle = FCoreStyle::Get().GetFontStyle("EmbossedText");
	textStyle.Size = height / 1.5f;
	textStyle.OutlineSettings.OutlineSize = 0;
	fileName->SetFont(textStyle);
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
		std::vector<RadarDataHolder::State> stateVector = radarCollection->StateVector();
		
		
		width = stateVector.size() <= 300 ? 400 : 1000;
		if(stateVector.size() >= 1000){
			// expand to parent
			TSharedPtr<SWidget> parent = this->GetParentWidget();
			// if(parent != NULL){
			// 	parent = parent->GetParentWidget();
			// }
			if(parent != NULL){
				float parentWidth = parent->GetTickSpaceGeometry().GetLocalSize().X;
				if(width != parentWidth){
					width = parentWidth;
					// force an update
					cellCount = 0;
				}
			}
		}
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
			}
			AddSlot().Offset(FMargin(0, height / 2.0f)).AutoSize(true).Anchors(FAnchors(0.0f, 0.0f))[selector.ToSharedRef()];
			selector->SetDesiredSizeOverride(FVector2D(width / (float)cellCount, height / 2.0f));
			if(fileName.IsValid()){
				AddSlot().Offset(FMargin(0, -2)).AutoSize(true).Anchors(FAnchors(0.0f, 0.0f)).Alignment(FVector2D(0.0f, 1.0f))[fileName.ToSharedRef()];
				fileName->SetJustification(ETextJustify::Left);
			}
			// TSharedPtr<class SImage> background;
			// SAssignNew(background, SImage);
			// AddSlot(-1)[background.ToSharedRef()];
			// background->SetImage(backgroundBrush);
		}
		
		selector->SetRenderTransform(FVector2D(width / (float)cellCount * ((float)radarCollection->GetCurrentCachePosition() + 0.5f),0));
		if(fileName.IsValid()){
			//int lastSlash = std::max(std::max((int)path.find_last_of('/'), (int)path.find_last_of('\\')), 0);
			//filePath = path.substr(0, lastSlash + 1);
			RadarDataHolder* holder = radarCollection->GetCurrentRadarData();
			
			if(holder->state == RadarDataHolder::DataStateFailed){
				fileName->SetColorAndOpacity(FSlateColor(FLinearColor(0.6, 0.1, 0.1, 1)));
			}else if(holder->state == RadarDataHolder::DataStateLoading){
				fileName->SetColorAndOpacity(FSlateColor(FLinearColor(0.1, 0.1, 1.0, 1)));
			}else{
				fileName->SetColorAndOpacity(FSlateColor(FLinearColor(1.0, 1.0, 1.0, 1)));
			}
			fileName->SetText(FText::FromString(holder->fileInfo.name.c_str()));
		}
		
		for(int i = 0; i < cellCount; i++){
			switch(stateVector[i]){
				case RadarDataHolder::State::DataStateUnloaded:
					cells[i]->SetImage(&unloadedBrush);
				break;
				case RadarDataHolder::State::DataStateLoaded:
					cells[i]->SetImage(&loadedBrush);
				break;
				case RadarDataHolder::State::DataStateLoading:
					cells[i]->SetImage(&loadingBrush);
				break;
				case RadarDataHolder::State::DataStateFailed:
					cells[i]->SetImage(&errorBrush);
				break;
			}
		}
		
	}
}
