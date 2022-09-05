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
#include "uiwindow.h"
#include "../Radar/SystemApi.h"
#include "RadarGameStateBase.h"
#include "RadarViewPawn.h"
#include "portable-file-dialogs.h"

#include <vector>

//#include "imgui_internal.h"
#include "imgui.h"
#include "ImGuiModule.h"
#include "Widgets/SWindow.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Console.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "HAL/FileManager.h"
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

bool ToggleButton(const char* label, bool active, const ImVec2 &size = ImVec2(0, 0)) {
	if (active) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	}
	bool pressed = ImGui::Button(label, size);
	if (active) {
		ImGui::PopStyleColor(2);
	}
	return pressed;
}


// Sets default values
AImGuiUI::AImGuiUI()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}
AImGuiUI::~AImGuiUI(){
	delete uiWindow;
}

// Called when the game starts or when spawned
void AImGuiUI::BeginPlay()
{
	Super::BeginPlay();
	
	LoadFonts();
	unsafeFrames = 10;
	//FImGuiModule::Get().RebuildFontAtlas();
	
	ImGuiStyle &style = ImGui::GetStyle();
	
	InitializeConsole();
	
	
	
	UnlockMouse();
}

//const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());

// Called every frame
void AImGuiUI::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	GlobalState &globalState = GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	if (unsafeFrames > 0) {
		unsafeFrames--;
		globalState.devShowImGui = false;
		return;
	}
	globalState.devShowImGui = true;

	ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>();

	FViewport* veiwport = GetWorld()->GetGameViewport()->Viewport;
	FIntPoint viewportSize = veiwport->GetSizeXY();

	SWindow* swindow = GetWorld()->GetGameViewport()->GetWindow().Get();
	float nativeScale = ((uiWindow != NULL && uiWindow->isOpen) ? uiWindow->window.Get() : swindow)->GetDPIScaleFactor();
	if(nativeScale != globalState.defaults->guiScale){
		if(globalState.defaults->guiScale == globalState.guiScale){
			globalState.guiScale = nativeScale;
		}
		globalState.defaults->guiScale = nativeScale;
	}

	float fontScale = globalState.guiScale;
	ImGui::SetNextWindowBgAlpha(0.3);
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), 0, ImVec2(0.0f, 0.0f));
	//ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), 0);
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = fontScale;
	ImGui::SetNextWindowSizeConstraints(ImVec2(100.0f, 100.0f), ImVec2(viewportSize.X / 2, viewportSize.Y));
	if(ImGui::Begin("OpenStorm", NULL, ImGuiWindowFlags_NoMove | /*ImGuiWindowFlags_NoBackground |*/ ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar)){
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1)){
			//auto rect = ImGui::GetCurrentWindow()->TitleBarRect();
			//if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false)){
				//ImGui::OpenPopup("FOO");
			//}
		}
		if(scalabilityTest){
			//ImGui::SetWindowFontScale((cos(SystemAPI::CurrentTime() / 1) / 2 + 1.5) * fontScale);
			ImGui::SetWindowFontScale((cos(SystemAPI::CurrentTime() / 1) / 2 + 1.5));
		}else{
			ImGui::SetWindowFontScale(1);
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
			if(ToggleButton(ICON_FA_PLAY, globalState.animate, squareButtonSize)){
				globalState.animate = !globalState.animate;
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
				
				bool spatialInterpolationOldValue = globalState.spatialInterpolation;
				ImGui::Checkbox("Spatial Interpolation", &globalState.spatialInterpolation);
				if(spatialInterpolationOldValue != globalState.spatialInterpolation){
					globalState.EmitEvent("UpdateVolumeParameters");
				}
				
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
				ImGui::Checkbox("Temporal Interpolation", &GS->globalState.temporalInterpolation);
				//ImGui::Text("Animation Speed:");
				//ImGui::SliderFloat("##1", &GS->globalState.animateSpeed, 0.0f, 5.0f);
				CustomFloatInput("Time Animation Speed", 1, 15, &globalState.animateSpeed, &globalState.defaults->animateSpeed);
				CustomFloatInput("Cutoff Animation Time", 0.5, 10, &globalState.animateCutoffTime, &globalState.defaults->animateCutoffTime);
				ImGui::TreePop();
			}
			ImGui::Separator();
			if (ImGui::TreeNodeEx("Data", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Poll Data", &globalState.pollData);
				if (ImGui::Button("Load Files")) {
					ChooseFiles();
				}
				ImGui::TreePop();
			}
			//ImGui::Separator();

		}	
		if (ImGui::CollapsingHeader("Filter")) {
			ImGui::Text("TODO");
		}
		
		if (ImGui::CollapsingHeader("Settings")) {
			if (ImGui::TreeNodeEx("Display", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				CustomFloatInput("Max FPS", 20, 120, &globalState.maxFPS, &globalState.defaults->maxFPS);
				if (ImGui::Button("External Window")) {
					if(uiWindow != NULL){
						InternalWindow();
					}else{
						ExternalWindow();
					}
				}
				ImGui::SameLine();
				if (ToggleButton("VR " ICON_FA_VR_CARDBOARD, globalState.vrMode)) {
					globalState.vrMode = !globalState.vrMode;
					if(globalState.vrMode){
						GEngine->Exec(GetWorld(), TEXT("vr.bEnableStereo 1"));
					}else{
						GEngine->Exec(GetWorld(), TEXT("vr.bEnableStereo 0"));
					}
					//ImGui::SetWindowCollapsed(true);
					ExternalWindow();
				}
				ImGui::TreePop();
			}
		}
		
		if (ImGui::CollapsingHeader("Ligma")) {
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			if (ImGui::Button("Demo Window")) {
				showDemoWindow = !showDemoWindow;
			}
			ImGui::SameLine();
			if (ImGui::Button("Load Font")) {
				LoadFonts();
				unsafeFrames = 10;
				return;
			}
			if (ImGui::Button("External win")) {
				ExternalWindow();
			}
			ImGui::SameLine();
			if (ImGui::Button("Internal win")) {
				// this was thought to be impossible
				InternalWindow();
			}
			ImGui::SameLine();
			if (ImGui::Button("Unlock")) {
				UnlockMouse();
			}
			ImGui::SameLine();
			if (ImGui::Button("Lock")) {
				LockMouse();
			}
			if (ImGui::Button("Reload File")) {
				globalState.EmitEvent("DevReloadFile");
			}
			if (ImGui::Button("Cache State")) {
				globalState.devShowCacheState = !globalState.devShowCacheState;
			}
			ImGui::Text("Custom float input:");
			CustomFloatInput("test float", 0, 3, &globalState.testFloat);
			CustomFloatInput("test float##2", 0, 2, &globalState.testFloat, &globalState.defaults->testFloat);
			
			CustomFloatInput("gui scale", 1, 2, &globalState.guiScale, &globalState.defaults->guiScale);
			
			
			ImGui::Checkbox("Scalability Test", &scalabilityTest);
			
		}
		
		if (ImGui::Button("Test")) {
			ligma(GS->globalState.inputToggle);
		}
	}
	ImGui::End();
	
	
	if(showDemoWindow){
		ImGui:: ShowDemoWindow();
	}
	
	
	if(!io.WantCaptureMouse){
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
			// mouse release will not be received
			//io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
			//io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
			//fprintf(stderr, "Capture mouse here\n");
			globalState.isMouseCaptured = true;
			LockMouse();
		}
	}
	
	if (uiWindow != NULL) {
		uiWindow->Tick();
	}
	
	if(fileChooser != NULL){
		// check if user has taken action on file dialog
		if(fileChooser->ready()){
			std::vector<std::string> files = fileChooser->result();
			if(files.size() > 0){
				globalState.EmitEvent("LoadDirectory", files[0], NULL);
			}
			delete fileChooser;
			fileChooser = NULL;
		}
	}
	
}





void AImGuiUI::LockMouse() {
	fprintf(stderr, "Locking mouse\n");
	FImGuiModule::Get().GetProperties().SetInputEnabled(false);
	//GetWorld()->GetGameViewport()->Viewport->;
	GetWorld()->GetGameViewport()->Viewport->CaptureMouse(true);
	GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeGameOnly());
	//ImGuiIO& io = ImGui::GetIO();
}

void AImGuiUI::UnlockMouse() {
	fprintf(stderr, "Unlocking mouse\n");
	FImGuiModule::Get().GetProperties().SetInputEnabled(true);
	FImGuiModule::Get().GetProperties().SetGamepadNavigationEnabled(false);
	FImGuiModule::Get().GetProperties().SetMouseInputShared(false);
	FImGuiModule::Get().GetProperties().SetGamepadInputShared(true);
	ImGui::SetWindowFocus();
	//GetWorld()->GetGameViewport()->Viewport->SetUserFocus(true);
	//FImGuiModule::Get().SetInputMode
	//ImGuiIO& io = ImGui::GetIO();
	//io.ClearInputKeys();
	//io.ClearInputCharacters();
	//io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
	//io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
	//GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeGameOnly());
	GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeGameAndUI());
}

void AImGuiUI::ligma(bool Value)
{
	FImGuiModule::Get().GetProperties().SetInputEnabled(Value);
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	globalState->EmitEvent("Test");
	globalState->EmitEvent("TestUnregistered");
}

void AImGuiUI::InitializeConsole()
{
    UWorld* World = GetWorld();
    if (!ensure(World != nullptr)) return;
    auto* Viewport = World->GetGameViewport();
    if (!ensure(Viewport != nullptr)) return;

    if (!Viewport->ViewportConsole)
    {
        Viewport->ViewportConsole = static_cast<UConsole*>(UGameplayStatics::SpawnObject(UConsole::StaticClass(), GetWorld()->GetGameViewport()));
    }
}

void AImGuiUI::ExternalWindow() {
	if (uiWindow != NULL) {
		if (!uiWindow->isOpen) {
			// already closed
			delete uiWindow;
			uiWindow = NULL;
		}
	}
	if(uiWindow == NULL){
		GEngine->Exec(GetWorld(), TEXT("Slate.bAllowThrottling 0"));
		uiWindow = new UIWindow(GetWorld()->GetGameViewport());
	}
}

void AImGuiUI::InternalWindow() {
	if(uiWindow != NULL){
		uiWindow->Close();
		delete uiWindow;
		uiWindow = NULL;
	}
}

void AImGuiUI::ChooseFiles() {
	/*
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	std::vector<std::string> files = {};
    TArray<FString> outFiles;
	
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		void* handle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
		uint32 SelectionFlag = 1; //A value of 0 represents single file selection while a value of 1 represents multiple file selection
		DesktopPlatform->OpenFileDialog(handle, TEXT("Open Radar Files"), FPaths::ProjectDir(), FString(""), FString(""), SelectionFlag, outFiles);
	}
	//IDesktopPlatform::OpenFileDialog(GetWorld()->GetGameViewport()->GetWindow()->GetNativeWindow()->GetOSWindowHandle(),FPaths::ProjectDir())
	
    for (FString file : outFiles)
    {
        std::string fileString = std::string(TCHAR_TO_UTF8(*file));
        files.push_back(fileString);
		fprintf(stderr, "%s\n", fileString.c_str());
    }
	if(files.size() > 0){
		globalState->EmitEvent("LoadDirectory", files[0], NULL);
	}*/
	if (fileChooser == NULL) {
		FString radarDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("../files/dir/"));
		FString fullradarDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*radarDir);
		const char* radarDirLocaition = StringCast<ANSICHAR>(*fullradarDir).Get();
		fileChooser = new pfd::public_open_file("Open Radar Files", "", { "All Files", "*" }, pfd::opt::multiselect);
	}
}