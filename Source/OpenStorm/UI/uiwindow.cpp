#include "uiwindow.h"

#include "RHIDefinitions.h" // including this prevents some wierd compilation errors when pre compiles headers are disabled
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
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateColorBrush.h"
#include "Math/Color.h"

#include "ImGuiModule.h"
#include "ImGuiDelegates.h"
#include "imgui.h"
//#include "../../../Plugins/UnrealImGui/Source/ImGui/Private/Widgets/SImGuiLayout.h"


class SInputProxy : public SOverlay {
public:
	
	TSharedPtr<SViewport> viewport;
	virtual FReply OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& InAnalogInputEvent) {
		if (viewport == NULL) {
			return FReply::Unhandled();
		}
		// pass on controller input to main viewport
		return viewport->OnAnalogValueChanged(MyGeometry, InAnalogInputEvent);
	}
	void SetViewport(UGameViewportClient* viewportClient) {
		viewport = viewportClient->GetGameViewportWidget();
	}
};




UIWindow::UIWindow(UGameViewportClient* gameViewport){
	window = SNew(SWindow)
		.Title(FText::FromString("OpenStorm Settings"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(300, 500))
		.UseOSWindowBorder(true);
	//SWindow* swindow = window.Get();
	FSlateApplication::Get().AddWindow(window.ToSharedRef());

	window->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &UIWindow::CloseDelegate));
	//FImGuiModule* newModule = new FImGuiModule();
	


	// set window content
	SAssignNew(windowContent, SInputProxy);
	window->SetContent(windowContent.ToSharedRef());
	windowContent->SetViewport(gameViewport);


	// create background
	TSharedPtr<class SImage> background;
	SAssignNew(background, SImage);
	windowContent->AddSlot(-1)[background.ToSharedRef()];
	backgroundBrush = new FSlateColorBrush(FLinearColor(0.01, 0.01, 0.01, 1));
	background->SetImage(backgroundBrush);

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
			fprintf(stderr, "I found you imgui, you are going to UIWindow.\n");
			imGuiWidget = child;
			// remove from main viewport
			mainViewportOverlayWidget->RemoveSlot(child);
			// place in window
			windowContent->AddSlot(10)[child];
		}
	}


	window->BringToFront();
	isOpen = true;
}

UIWindow::~UIWindow(){
	// avoid moving ImGui during cleanup to prevent crashing
	imGuiWidget = NULL;
	window->SetOnWindowClosed(NULL);
	Close();
	delete backgroundBrush;
}



void UIWindow::ReturnImGui() {
	if (imGuiWidget.IsValid() && mainViewportOverlayWidget.IsValid()) {
		// remove from window
		windowContent->RemoveSlot(imGuiWidget.ToSharedRef());
		//window->RemoveOverlaySlot(imGuiWidget.ToSharedRef());
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
		isOpen = false;
		FSlateApplication::Get().RequestDestroyWindow(window.ToSharedRef());//.(window)
	}
}

void UIWindow::Tick(){
	/*ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), 0);
	if (ImGui::Begin("Sugma", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar)){
		if (ImGui::Button("Sugma")) {
			fprintf(stderr, "Sugma\n");
		}
	}
	ImGui::End();*/
}

void UIWindow::CloseDelegate(const TSharedRef<SWindow>& windowRef){
	Close();
}
