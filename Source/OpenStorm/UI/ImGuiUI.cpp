// Fill out your copyright notice in the Description page of Project Settings.

/*
UI TODO

Variables to implement
GS->globalState.moveSpeed
GS->globalState.rotateSpeed
GS->globalState.fadeSpeed
GS->globalState.fade
GS->globalState.animate
GS->globalState.animateSpeed
GS->globalState.interpolation
GS->globalState.maxFPS

*/

#include "ImGuiUI.h"
#include "font.h"
#include "native.h"
#include "../Radar/SystemApi.h"
#include "imgui.h"
//#include "imgui_internal.h"
#include <ImGuiModule.h>
#include "RadarGameStateBase.h"
#include <RadarViewPawn.h>
#include "Widgets/SWindow.h"

#include "UnrealClient.h"

static bool inlineLabel = false;

typedef int CustomFloatInputFlags;
enum CustomFloatInputFlags_{
	CustomFloatInput_SliderOnly = 1, // Only show the slider. Only use this if values outside of the slider value do absolutely nothing
};

// intput for a float value
void CustomFloatInput(const char* label, float minSlider, float maxSlider, float* value, float* defaultValue = NULL, CustomFloatInputFlags flags = 0){
	float fontSize = ImGui::GetFontSize();
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::PushID(label);
	if(!inlineLabel){
		ImGui::PushItemWidth(0.01);
		ImGui::LabelText(label, "");
		ImGui::PopItemWidth();
	}
	if(!(flags & CustomFloatInput_SliderOnly)){
		ImGui::PushItemWidth(8 * fontSize);
		ImGui::InputFloat("##floatInput", value, 0.1f, 1.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();
	}
	if(defaultValue != NULL){
		float frameHeight = ImGui::GetFrameHeight();
		ImGui::PushItemWidth(12 * fontSize - frameHeight - style.ItemSpacing.x);
		ImGui::SliderFloat("##floatSlider", value, minSlider, maxSlider);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if(ImGui::Button(ICON_FA_DELETE_LEFT ,ImVec2(frameHeight, frameHeight))){
			*value = *defaultValue;
		}
	}else{
		ImGui::PushItemWidth(12 * fontSize);
		ImGui::SliderFloat("##floatSlider", value, minSlider, maxSlider);
		ImGui::PopItemWidth();
	}
	
	if(inlineLabel){
		ImGui::SameLine();
	
		/*const char* labelEnd = ImGui::FindRenderedTextEnd(label);
		if (label != labelEnd)
		{
			ImGui::TextEx(label, labelEnd);
		}*/
		//ImGui::TextUnformatted(label);
		ImGui::PushItemWidth(0.01);
		ImGui::LabelText(label, "");
		ImGui::PopItemWidth();
	}
	ImGui::PopID();
}




// Sets default values
AImGuiUI::AImGuiUI()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AImGuiUI::BeginPlay()
{
	Super::BeginPlay();
	
	LoadFonts();
	FImGuiModule::Get().RebuildFontAtlas();
	
	ImGuiStyle &style = ImGui::GetStyle();
	
	
	
	UnlockMouse();
}

//const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());

// Called every frame
void AImGuiUI::Tick(float deltaTime)
{
	Super::Tick(deltaTime);
	ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>();
	GlobalState &globalState = GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;

	FViewport* veiwport = GetWorld()->GetGameViewport()->Viewport;
	FIntPoint viewportSize = veiwport->GetSizeXY();

	SWindow* swindow = GetWorld()->GetGameViewport()->GetWindow().Get();
	float nativeScale = swindow->GetDPIScaleFactor();
	if(nativeScale != globalState.defaults->guiScale){
		if(globalState.defaults->guiScale == globalState.guiScale){
			globalState.guiScale = nativeScale;
		}
		globalState.defaults->guiScale = nativeScale;
	}

	float fontScale = 0.4;
	fontScale *= globalState.guiScale;
	ImGui::SetNextWindowBgAlpha(0.3);
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), 0, ImVec2(0.0f, 0.0f));
	//ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), 0);
	
	ImGui::SetNextWindowSizeConstraints(ImVec2(100.0f, 100.0f), ImVec2(viewportSize.X / 2, viewportSize.Y));
	if(ImGui::Begin("OpenStorm", NULL, ImGuiWindowFlags_NoMove | /*ImGuiWindowFlags_NoBackground |*/ ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar)){
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1)){
			//auto rect = ImGui::GetCurrentWindow()->TitleBarRect();
			//if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false)){
				//ImGui::OpenPopup("FOO");
			//}
		}
		if(scalabilityTest){
			ImGui::SetWindowFontScale((cos(SystemAPI::CurrentTime() / 1) / 2 + 1.5) * fontScale);
		}else{
			ImGui::SetWindowFontScale(fontScale);
		}
		
		float frameHeight = ImGui::GetFrameHeight();
		float fontSize = ImGui::GetFontSize();
		ImVec2 squareButtonSize = ImVec2(frameHeight, frameHeight);
		
		if (ImGui::CollapsingHeader("Basic", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushButtonRepeat(true);
			if(ImGui::Button(ICON_FA_BACKWARD_STEP, squareButtonSize)){
				globalState.EmitEvent("BackwardStep");
			}
			ImGui::SameLine();
			bool buttonActive = globalState.animate;
			if(buttonActive){
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			if(ImGui::Button(ICON_FA_PLAY, squareButtonSize)){
				globalState.animate = !globalState.animate;
			}
			if(buttonActive){
				ImGui::PopStyleColor(2);
			}
			ImGui::SameLine();
			if(ImGui::Button(ICON_FA_FORWARD_STEP, squareButtonSize)){
				globalState.EmitEvent("ForwardStep");
			}
			ImGui::PopButtonRepeat();
		}
		if (ImGui::CollapsingHeader("Main", ImGuiTreeNodeFlags_DefaultOpen)) {
			
			if (ImGui::TreeNodeEx("Radar", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				CustomFloatInput("Cutoff", 0, 1, &globalState.cutoff, &globalState.defaults->cutoff, CustomFloatInput_SliderOnly);
				ImGui::TreePop();
			}
			ImGui::Separator();
			
			if (ImGui::TreeNodeEx("Movement", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				CustomFloatInput("Movement Speed", 10, 1500, &globalState.moveSpeed, &globalState.defaults->moveSpeed);
				
				ImGui::Text("Rotation Speed:");
				ImGui::SliderFloat("##2", &GS->globalState.rotateSpeed, 0.0f, 1000.0f);
				ImGui::TreePop();
				//ImGui::Text("MovementSpeed: %f", GS->globalState.moveSpeed);
			}
			ImGui::Separator();
			
			if (ImGui::TreeNodeEx("Fade", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				ImGui::Checkbox("Fade", &GS->globalState.fade);
				ImGui::Text("Fade Speed:");
				ImGui::SliderFloat("##1", &GS->globalState.fadeSpeed, 0.0f, 5.0f);
				ImGui::TreePop();
			}
			ImGui::Separator();
			
			if (ImGui::TreeNodeEx("Animation", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				ImGui::Checkbox("Time", &GS->globalState.animate);
				ImGui::Checkbox("Cuttoff", &GS->globalState.animateCutoff);
				ImGui::Checkbox("Interpolation", &GS->globalState.interpolation);
				//ImGui::Text("Animation Speed:");
				//ImGui::SliderFloat("##1", &GS->globalState.animateSpeed, 0.0f, 5.0f);
				CustomFloatInput("Time Animation Speed", 1, 15, &globalState.animateSpeed, &globalState.defaults->animateSpeed);
				CustomFloatInput("Cutoff Animation Time", 0.5, 10, &globalState.animateCutoffTime, &globalState.defaults->animateCutoffTime);
				ImGui::TreePop();
			}
			//ImGui::Separator();

		}	
		if (ImGui::CollapsingHeader("Filter")) {
			ImGui::Text("TODO");
		}
		
		if (ImGui::CollapsingHeader("Settings")) {
			ImGui::Text("Max FPS:");
			ImGui::InputInt("##1", &GS->globalState.maxFPS, 10);
		}
		
		if (ImGui::CollapsingHeader("Ligma")) {
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			if (ImGui::Button("Demo Window")) {
				showDemoWindow = !showDemoWindow;
			}
			ImGui::Text("Custom float input:");
			CustomFloatInput("test float", 0, 3, &globalState.testFloat);
			CustomFloatInput("test float##2", 0, 2, &globalState.testFloat, &globalState.defaults->testFloat);
			
			CustomFloatInput("gui scale", 1, 2, &globalState.guiScale, &globalState.defaults->guiScale);
			
			
			ImGui::Checkbox("Scalability Test", &scalabilityTest);
			
		}
		if(showDemoWindow){
			ImGui:: ShowDemoWindow();
		}
		
		if (ImGui::Button("Test")) {
			ligma(GS->globalState.inputToggle);
		}
	}
	ImGui::End();
	
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse){
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
			// mouse release will not be recieved
			//io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
			//io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
			//fprintf(stderr, "Capture mouse here\n");
			globalState.isMouseCaptured = true;
			LockMouse();
		}
	}
	
}





void AImGuiUI::LockMouse() {
	FImGuiModule::Get().GetProperties().SetInputEnabled(false);
	//GetWorld()->GetGameViewport()->Viewport->;
	GetWorld()->GetGameViewport()->Viewport->CaptureMouse(true);
	GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeGameOnly());
	ImGuiIO& io = ImGui::GetIO();
}

void AImGuiUI::UnlockMouse() {
	FImGuiModule::Get().GetProperties().SetInputEnabled(true);
	ImGui::SetWindowFocus();
	FImGuiModule::Get().GetProperties().SetMouseInputShared(true);
	//GetWorld()->GetGameViewport()->Viewport->SetUserFocus(true);
	//FImGuiModule::Get().SetInputMode
	ImGuiIO& io = ImGui::GetIO();
	io.ClearInputKeys();
	io.ClearInputCharacters();
	io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
	io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
}

void AImGuiUI::ligma(bool Value)
{
	FImGuiModule::Get().GetProperties().SetInputEnabled(Value);
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	globalState->EmitEvent("Test");
	globalState->EmitEvent("TestUnregistered");
}

