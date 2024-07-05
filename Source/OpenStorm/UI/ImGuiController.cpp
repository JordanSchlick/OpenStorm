#include "ImGuiController.h"
#include "Font.h"
#include "Native.h"
#include "ImGuiUI.h"
#include "UiWindow.h"
#include "../Radar/SystemAPI.h"
#include "../Radar/Products/RadarProduct.h"
#include "../Radar/Globe.h"
#include "../Radar/NexradSites/NexradSites.h"
#include "../Objects/RadarGameStateBase.h"
#include "../Objects/RadarViewPawn.h"
#include "../EngineHelpers/StringUtils.h"
#include "./portable-file-dialogs.h"
#include "./ClickableInterface.h"

#include <vector>

//#include "imgui_internal.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "ImGuiModule.h"
#include "Widgets/SWindow.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Console.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

#if WITH_EDITOR
//#include "Toolkits/AssetEditorManager.h"
#endif


// Sets default values
AImGuiController::AImGuiController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	imGuiUI = new ImGuiUI();
	imGuiUI->imGuiController = this;
}
AImGuiController::~AImGuiController(){
	if(uiWindow != NULL){
		delete uiWindow;
		uiWindow = NULL;
	}
	delete imGuiUI;
}

// Called when the game starts or when spawned
void AImGuiController::BeginPlay()
{
	Super::BeginPlay();
	
	LoadFonts();
	unsafeFrames = 10;
	//FImGuiModule::Get().RebuildFontAtlas();
	
	ImGuiStyle &style = ImGui::GetStyle();
	style.DisplaySafeAreaPadding = ImVec2(0, 0);
	
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		callbackIds.push_back(globalState->RegisterEvent("UpdateEngineSettings", [this](std::string stringData, void* extraData) {
			UpdateEngineSettings();
		}));
	}
	
	InitializeConsole();
	
	UpdateEngineSettings();
	
	UnlockMouse();
}

void AImGuiController::EndPlay(const EEndPlayReason::Type endPlayReason) {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		// unregister all events
		for(auto id : callbackIds){
			globalState->UnregisterEvent(id);
		}
	}
	Super::EndPlay(endPlayReason);
}

//const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());

// Called every frame
void AImGuiController::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	GlobalState &globalState = GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	if (unsafeFrames > 0) {
		unsafeFrames--;
		globalState.devShowImGui = false;
		return;
	}
	globalState.devShowImGui = true;
	
	double now = SystemAPI::CurrentTime();

	ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>();

	ImGuiStyle &style = ImGui::GetStyle();
	style.DisplaySafeAreaPadding = ImVec2(0, 0);
	
	// FViewport* veiwport = GetWorld()->GetGameViewport()->Viewport;
	// FIntPoint viewportSize = veiwport->GetSizeXY();
	ImGuiIO& io = ImGui::GetIO();
	SWindow* swindow = GetWorld()->GetGameViewport()->GetWindow().Get();
	
	float nativeScale = 1;
	// ImVec2 maxSize = ImVec2(viewportSize.X / 2, viewportSize.Y);
	ImVec2 maxSize = ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y);
	if (uiWindow != NULL && uiWindow->isOpen) {
		// FVector2D viewportSize2 = uiWindow->window.Get()->GetViewportSize();
		// maxSize = ImVec2(viewportSize2.X, viewportSize2.Y);
		nativeScale = uiWindow->window.Get()->GetDPIScaleFactor();
		maxSize = ImVec2(io.DisplaySize.x, io.DisplaySize.y);
	}else{
		nativeScale = swindow->GetDPIScaleFactor();
	}
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
	
	io.FontGlobalScale = fontScale;
	ImGui::SetNextWindowSizeConstraints(ImVec2(100.0f, 100.0f), maxSize);
	
	imGuiUI->MainUI();
	imGuiUI->Tick(deltaTime);
	
	
	if(!io.WantCaptureMouse || isLeftClicking){
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || isLeftClicking){
			LeftClick();
		}
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
			// mouse release will not be received
			//io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
			//io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
			//fprintf(stderr, "Capture mouse here\n");
			LockMouse();
		}
	}
	
	if(io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L)){
		globalState.developmentMode = !globalState.developmentMode;
	}
	
	if (uiWindow != NULL) {
		uiWindow->Tick();
	}
	
	
}


void AImGuiController::LeftClick() {
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
		isLeftClicking = true;
		
		// cast ray to find out what was clicked on
		ImVec2 mousePos = ImGui::GetMousePos();
		FVector clickWorldLocation;
		FVector clickWorldDirection;
		GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(mousePos.x, mousePos.y, clickWorldLocation, clickWorldDirection);
		// fprintf(stderr, "(%f %f %f) (%f %f %f)\n",clickWorldLocation.X,clickWorldLocation.Y,clickWorldLocation.Z,clickWorldDirection.X,clickWorldDirection.Y,clickWorldDirection.Z);
		FVector traceEndLocation = clickWorldLocation + clickWorldDirection * 100000;
		FCollisionQueryParams queryParams;
		FHitResult hit;
		ECollisionChannel channel = ECC_Camera;
		GetWorld()->LineTraceSingleByChannel(hit, clickWorldLocation, traceEndLocation, channel, queryParams);
		AActor* hitActor = hit.GetActor();
		if(hitActor != NULL){
			selectedActor = hitActor;
			// fprintf(stderr, "%s\n", StringUtils::FStringToSTDString(hitActor->GetName()).c_str());
			if(dynamic_cast<IClickableInterface*>(hitActor) != NULL){
				
			}
		}else{
			selectedActor = NULL;
		}
	}
	
	// lock if the mouse is moved
	if(ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2)){
		isLeftClicking = false;
		LockMouse();
	}
	
	// the mouse was never locked before release
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)){
		isLeftClicking = false;
		if(selectedActor != NULL){
			//fprintf(stderr, "Clicking on actor %s\n", StringUtils::FStringToSTDString(selectedActor->GetName()).c_str());
			IClickableInterface* clickable = dynamic_cast<IClickableInterface*>(selectedActor);
			if(clickable != NULL){
				clickable->OnClick();
			}
		}
	}
}


void AImGuiController::LockMouse() {
	fprintf(stderr, "Locking mouse\n");
	FImGuiModule::Get().GetProperties().SetInputEnabled(false);
	//GetWorld()->GetGameViewport()->Viewport->;
	GetWorld()->GetGameViewport()->Viewport->CaptureMouse(true);
	GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeGameOnly());
	//ImGuiIO& io = ImGui::GetIO();
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		globalState->isMouseCaptured = true;
	}
}

void AImGuiController::UnlockMouse() {
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
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode != NULL){
		GlobalState* globalState = &gameMode->globalState;
		globalState->isMouseCaptured = false;
	}
}



void AImGuiController::InitializeConsole()
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

// send controls to an external window
void AImGuiController::ExternalWindow() {
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

// close external settings window and move controls to main viewport
void AImGuiController::InternalWindow() {
	if(uiWindow != NULL){
		uiWindow->Close();
		delete uiWindow;
		uiWindow = NULL;
	}
}

void AImGuiController::UpdateEngineSettings() {
	GlobalState& globalState = GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.VSync %i"), globalState.vsync && !globalState.vrMode));
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.VSyncEditor %i"), globalState.vsync && !globalState.vrMode));
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("t.MaxFPS %f"), (globalState.maxFPS == 0 || globalState.vrMode) ? 0 : std::max(5.0f,globalState.maxFPS)));
}

GlobalState* AImGuiController::GetGlobalState(){
	return &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
}
