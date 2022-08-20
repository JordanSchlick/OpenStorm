#include "uiwindow.h"


#include "Engine/GameViewportClient.h"
//#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SViewport.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SViewport.h"
//#include "SToolTip.h"
#include "Widgets/SWindow.h"

#include "ImGuiModule.h"
#include "ImGuiDelegates.h"
#include "imgui.h"
//#include "../../../Plugins/UnrealImGui/Source/ImGui/Private/Widgets/SImGuiLayout.h"

UIWindow::UIWindow(UGameViewportClient* gameViewport){
	window = SNew(SWindow)
		.Title(FText::FromString("OpenStorm Settings"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(300, 500))
		.UseOSWindowBorder(true);
	//SWindow* swindow = window.Get();
	FSlateApplication::Get().AddWindow(window.ToSharedRef());

	//FImGuiModule* newModule = new FImGuiModule();
	

	// Fish the SOverlay widget out of the viewport
	TSharedPtr<class SBox> dummyWidget;
	SAssignNew(dummyWidget, SBox);
	gameViewport->AddViewportWidgetContent(dummyWidget.ToSharedRef());
	mainViewportOverlayWidget = StaticCastSharedPtr<SOverlay>(dummyWidget.Get()->GetParentWidget());
	gameViewport->RemoveViewportWidgetContent(dummyWidget.ToSharedRef());
	
	FChildren* children = mainViewportOverlayWidget.Get()->GetChildren();

	fprintf(stderr, "Children count %i\n", children->Num());

	for (int i = 0; i < children->Num(); i++) {
		TSharedRef<SWidget> child = children->GetChildAt(i);
		fprintf(stderr, "child %s\n", TCHAR_TO_ANSI(*child->GetType().ToString()));
		if (child->GetType() == "SImGuiLayout")
		{
			fprintf(stderr, "I found you imgui, you are going to brazil.\n");
			imGuiWidget = child;
			// remove from main viewport
			mainViewportOverlayWidget->RemoveSlot(child);
			// place in window
			window->SetFullWindowOverlayContent(child);
		}
	}


	//TSharedPtr<class SOverlay>

	window->BringToFront();
	isOpen = true;
}

UIWindow::~UIWindow(){
	// avoid moving ImGui during cleanup to prevent crashing
	imGuiWidget = NULL;
	Close();
}



void UIWindow::ReturnImGui() {
	if (imGuiWidget.IsValid() && mainViewportOverlayWidget.IsValid()) {
		// remove from window
		window->RemoveOverlaySlot(imGuiWidget.ToSharedRef());
		// place back into main viewport
		mainViewportOverlayWidget->AddSlot(10001)[imGuiWidget.ToSharedRef()];
		imGuiWidget = NULL;
		//slot.AttachWidget(imGuiWidget.ToSharedRef());
		//mainViewport->AddViewportWidgetContent(imGuiWidget.ToSharedRef(), 10000);
	}
}

void UIWindow::Close() {
	ReturnImGui();

	if(isOpen && window.IsValid()){
		FSlateApplication::Get().RequestDestroyWindow(window.ToSharedRef());//.(window)
		isOpen = false;
	}
}

void UIWindow::Tick(){
	ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), 0);
	if (ImGui::Begin("Sugma", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar)){
		if (ImGui::Button("Sugma")) {
			fprintf(stderr, "Sugma\n");
		}
	}
	ImGui::End();
}