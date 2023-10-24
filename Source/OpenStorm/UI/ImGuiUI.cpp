#include "ImGuiUI.h"
#include "Font.h"
#include "Native.h"
#include "UiWindow.h"
#include "../Radar/SystemAPI.h"
#include "../Radar/Products/RadarProduct.h"
#include "../Radar/Globe.h"
#include "../Radar/NexradSites/NexradSites.h"
#include "../Objects/RadarGameStateBase.h"
#include "../Objects/RadarViewPawn.h"
#include "portable-file-dialogs.h"

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

static bool inlineLabel = false;

typedef int CustomInputFlags;
enum CustomInputFlags_{
	CustomInput_SliderOnly = 1, // Only show the slider. Only use this if values outside of the slider value do absolutely nothing
	CustomInput_Short = 2 // make it shorter
};

// intput for a float value
bool CustomFloatInput(const char* label, float minSlider, float maxSlider, float* value, float* defaultValue = NULL, CustomInputFlags flags = 0){
	bool changed = false;
	float fontSize = ImGui::GetFontSize();
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::PushID(label);
	if(!inlineLabel){
		ImGui::PushItemWidth(0.01);
		ImGui::LabelText(label, "");
		ImGui::PopItemWidth();
	}
	if(!(flags & CustomInput_SliderOnly)){
		ImGui::PushItemWidth(8 * fontSize);
		changed |= ImGui::InputFloat("##floatInput", value, 0.1f, 1.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();
	}
	float sliderWidth = (flags & CustomInput_SliderOnly && (flags & CustomInput_Short) == 0) ? 20 * fontSize + style.ItemSpacing.x : 12 * fontSize;
	if(defaultValue != NULL){
		float frameHeight = ImGui::GetFrameHeight();
		ImGui::PushItemWidth(sliderWidth - frameHeight - style.ItemSpacing.x);
		changed |= ImGui::SliderFloat("##floatSlider", value, minSlider, maxSlider);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if(ImGui::Button(ICON_FA_DELETE_LEFT ,ImVec2(frameHeight, frameHeight))){
			*value = *defaultValue;
			changed = true;
		}
	}else{
		ImGui::PushItemWidth(sliderWidth);
		changed |= ImGui::SliderFloat("##floatSlider", value, minSlider, maxSlider);
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
	return changed;
}

bool CustomTextInput(const char* label, std::string* value, std::string* defaultValue = NULL, CustomInputFlags flags = 0){
	bool changed = false;
	float fontSize = ImGui::GetFontSize();
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::PushID(label);
	if(!inlineLabel){
		ImGui::PushItemWidth(0.01);
		ImGui::LabelText(label, "");
		ImGui::PopItemWidth();
	}
	float itemWidth = ((flags & CustomInput_Short) == 0) ? 20 * fontSize : 12 * fontSize;
	itemWidth += style.ItemSpacing.x;
	if(defaultValue != NULL){
		float frameHeight = ImGui::GetFrameHeight();
		ImGui::PushItemWidth(itemWidth - frameHeight - style.ItemSpacing.x);
		changed |= ImGui::InputText("##text", value);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if(ImGui::Button(ICON_FA_DELETE_LEFT ,ImVec2(frameHeight, frameHeight))){
			*value = *defaultValue;
			changed = true;
		}
	}else{
		ImGui::PushItemWidth(itemWidth);
		changed |= ImGui::InputText("##text", value);
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
	return changed;
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

void AImGuiUI::EndPlay(const EEndPlayReason::Type endPlayReason) {
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
				RadarData::VolumeType volumeType = (RadarData::VolumeType)globalState.volumeType;
				ImGuiComboFlags flags = 0;
				RadarProduct* currentProduct = RadarProduct::GetProduct(volumeType);
				const char* comboPreviewValue = "Unknown";  // Pass in the preview value visible before opening the combo (it could be anything)
				if(currentProduct != NULL){
					comboPreviewValue = currentProduct->name.c_str();
				}
				if (ImGui::BeginCombo("Product", comboPreviewValue, flags)){
					for (RadarProduct* product : RadarProduct::products){
						//RadarProduct* product = item.second;
						if (!product->development || globalState.developmentMode) {
							const bool isSelected = product->volumeType == volumeType;
							if (ImGui::Selectable(product->name.c_str(), isSelected)) {
								//qualityCurrentIndex = n;
								if (!isSelected) {
									globalState.volumeType = product->volumeType;
									globalState.EmitEvent("ChangeProduct", "", (void*)&product->volumeType);
								}
							}
							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (isSelected) {
								ImGui::SetItemDefaultFocus();
							}
						}
					}
					ImGui::EndCombo();
				}
				
				CustomFloatInput("Cutoff", 0, 1, &globalState.cutoff, &globalState.defaults->cutoff, CustomInput_SliderOnly);
				CustomFloatInput("Opacity", 0.2, 4, &globalState.opacityMultiplier, &globalState.defaults->opacityMultiplier);
				CustomFloatInput("Height Exaggeration", 1, 4, &globalState.verticalScale, &globalState.defaults->verticalScale);
				
				bool spatialInterpolationOldValue = globalState.spatialInterpolation;
				ImGui::Checkbox("Spatial Interpolation", &globalState.spatialInterpolation);
				if(spatialInterpolationOldValue != globalState.spatialInterpolation){
					globalState.EmitEvent("UpdateVolumeParameters");
				}
				
				ImGui::Checkbox("Temporal Interpolation", &GS->globalState.temporalInterpolation);
				
				// ImGui::PushItemWidth(10 * fontSize);
				// if (ImGui::BeginCombo("View Mode", globalState.viewMode == GlobalState::VIEW_MODE_VOLUMETRIC ? "3D Volume" : "2D Slice", 0)){
				// 	bool isSelected = globalState.viewMode == GlobalState::VIEW_MODE_VOLUMETRIC;
				// 	if (ImGui::Selectable("3D Volume", isSelected)) {
				// 		globalState.viewMode = GlobalState::VIEW_MODE_VOLUMETRIC;
				// 	}
				// 	if (isSelected) {
				// 		ImGui::SetItemDefaultFocus();
				// 	}
				// 	isSelected = globalState.viewMode == GlobalState::VIEW_MODE_SLICE;
				// 	if (ImGui::Selectable("2D Slice", isSelected)) {
				// 		globalState.viewMode = GlobalState::VIEW_MODE_SLICE;
				// 	}
				// 	if (isSelected) {
				// 		ImGui::SetItemDefaultFocus();
				// 	}
				// 	ImGui::EndCombo();
				// }
				// ImGui::PopItemWidth();
				
				
				const char* viewModes[] = {"Invalid", "3D Volume", "2D Slice", "Invalid"};
				ImGui::PushItemWidth(15 * fontSize);
				ImGui::SliderInt("View Mode", (int*)&globalState.viewMode, 0, 1, viewModes[std::clamp(globalState.viewMode + 1, 0, 3)]);
				ImGui::PopItemWidth();
				
				if(globalState.viewMode == GlobalState::VIEW_MODE_SLICE){
					const char* sliceModes[] = {"Invalid", "Sweep Angle", "Constant Altitude", "Vertical", "Invalid"};
					ImGui::PushItemWidth(15 * fontSize);
					ImGui::SliderInt("Slice Mode", (int*)&globalState.sliceMode, 0, 2, sliceModes[std::clamp(globalState.sliceMode + 1, 0, 4)]);
					ImGui::PopItemWidth();
					if(globalState.sliceMode == GlobalState::SLICE_MODE_CONSTANT_ALTITUDE){
						CustomFloatInput("Slice Altitude (meters)", 0, 25000, &globalState.sliceAltitude, &globalState.defaults->sliceAltitude);
					}
					if(globalState.sliceMode == GlobalState::SLICE_MODE_SWEEP_ANGLE){
						CustomFloatInput("Slice Angle (degrees)", 0.5, 19.5, &globalState.sliceAngle, &globalState.defaults->sliceAngle);
					}
					if(globalState.sliceMode == GlobalState::SLICE_MODE_VERTICAL){
						CustomFloatInput("Slice Rotation", 0, 180, &globalState.sliceVerticalRotation, &globalState.defaults->sliceVerticalRotation, CustomInput_SliderOnly);
						CustomFloatInput("Slice X location", -1, 1, &globalState.sliceVerticalLocationX, &globalState.defaults->sliceVerticalLocationX, CustomInput_SliderOnly);
						CustomFloatInput("Slice Y location", -1, 1, &globalState.sliceVerticalLocationY, &globalState.defaults->sliceVerticalLocationY, CustomInput_SliderOnly);
					}
					ImGui::Checkbox("Volumetric Slice", &globalState.sliceVolumetric);
				}
				
				ImGui::TreePop();
			}
			ImGui::Separator();
			
			if (ImGui::TreeNodeEx("Movement", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				CustomFloatInput("Movement Speed", 10, 1500, &globalState.moveSpeed, &globalState.defaults->moveSpeed);
				
				
				CustomFloatInput("Rotation Speed", 0.0f, 300.0f, &globalState.rotateSpeed, &globalState.defaults->rotateSpeed);
				ImGui::TreePop();
				//ImGui::Text("MovementSpeed: %f", GS->globalState.moveSpeed);
			}
			ImGui::Separator();
			
			if (ImGui::TreeNodeEx("Animation", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				ImGui::Checkbox("Time", &GS->globalState.animate);
				ImGui::Checkbox("Cuttoff", &GS->globalState.animateCutoff);
				//ImGui::Text("Animation Speed:");
				//ImGui::SliderFloat("##1", &GS->globalState.animateSpeed, 0.0f, 5.0f);
				CustomFloatInput("Time Animation Speed", 1, 15, &globalState.animateSpeed, &globalState.defaults->animateSpeed);
				CustomFloatInput("Cutoff Animation Speed", 0.1, 2, &globalState.animateCutoffSpeed, &globalState.defaults->animateCutoffSpeed);
				ImGui::TreePop();
			}
			ImGui::Separator();
			if (ImGui::TreeNodeEx("Data", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Poll Data", &globalState.pollData);
				if (ImGui::Button("Load Files")) {
					ChooseFiles();
				}
				if (ImGui::TreeNodeEx("Download", ImGuiTreeNodeFlags_SpanAvailWidth)) {
					ImGui::Text("Download data:");
					if(globalState.downloadData){
						ImGui::BeginDisabled();
					}
					if(ImGui::Button("Start")){
						globalState.downloadData = true;
					}
					if(globalState.downloadData){
						ImGui::EndDisabled();
					}
					ImGui::SameLine();
					if(!globalState.downloadData){
						ImGui::BeginDisabled();
					}
					if(ImGui::Button("Stop")){
						globalState.downloadData = false;
					}
					if(!globalState.downloadData){
						ImGui::EndDisabled();
					}
					CustomFloatInput("Download interval (seconds)", 30, 300, &globalState.downloadPollInterval, &globalState.defaults->downloadPollInterval, CustomInput_SliderOnly | CustomInput_Short);
					
					static std::string siteIdSelection = "";
					
					ImGui::PushID("site_button");
					ImGui::Text("Select Site:");
					if (ImGui::Button(globalState.downloadSiteId.c_str())){
						siteIdSelection = globalState.downloadSiteId;
						ImGui::OpenPopup("Select Site");
					}
					
					if (ImGui::BeginPopup("Select Site", NULL))
					{
						ImGui::Text("Select a site to download from. \nAlternatively you can select a site by left clicking on the site name in the world.\n\n");
						ImGui::Separator();
						ImGui::InputText("Custom Site ID", &siteIdSelection);
						ImGui::Separator();
						for(size_t i = 0; i < NexradSites::numberOfSites; i++){
							NexradSites::Site* site = &NexradSites::sites[i];
							bool selected = site->name == siteIdSelection;
							if(ToggleButton(site->name, selected)){
								siteIdSelection = site->name;
							}
							if(i % 15 != 14){
								ImGui::SameLine();
							}
						}
						ImGui::Separator();
						if (ImGui::Button("OK", ImVec2(120,0))) {
							ImGui::CloseCurrentPopup();
							globalState.downloadSiteId = siteIdSelection;
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel", ImVec2(120,0))) {
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}
					ImGui::PopID();
					
					if (ImGui::TreeNodeEx("Advanced", ImGuiTreeNodeFlags_SpanAvailWidth)) {
						CustomTextInput("Data URL", &globalState.downloadUrl, &globalState.defaults->downloadUrl);
						ImGui::TreePop();
					}
					
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::Separator();
			if (ImGui::TreeNodeEx("Map", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				ImGui::Checkbox("Show Map", &globalState.enableMap);
				CustomFloatInput("Map Brightness", 0.01, 1.0, &globalState.mapBrightness, &globalState.defaults->mapBrightness);
				ImGui::Checkbox("Show Tiles", &globalState.enableMapTiles);
				ImGui::Checkbox("Show GIS info", &globalState.enableMapGIS);
				CustomFloatInput("GIS Brightness", 0.01, 1.5, &globalState.mapBrightnessGIS, &globalState.defaults->mapBrightnessGIS);
				if (ImGui::TreeNodeEx("Waypoints", ImGuiTreeNodeFlags_SpanAvailWidth)) {
					char idChr[3] = {};
					bool markersChanged = false;
					ImGui::PushItemWidth(10 * ImGui::GetFontSize());
					for(int id = 0; id < globalState.locationMarkers.size(); id++){
						GlobalState::Waypoint &marker = globalState.locationMarkers[id];
						ImGui::Separator();
						// only allows for about 60000 markers in the ui
						idChr[0] = (id % 255) + 1;
						idChr[1] = id / 255 + 1;
						ImGui::PushID(idChr);
						markersChanged |= ImGui::InputText("Name", &marker.name);
						markersChanged |= ImGui::InputDouble("Latitude", &marker.latitude);
						markersChanged |= ImGui::InputDouble("Longitude", &marker.longitude);
						markersChanged |= ImGui::InputDouble("Altitude", &marker.altitude);
						markersChanged |= ImGui::Checkbox("", &marker.enabled);
						ImGui::SameLine();
						if (ImGui::Button("Teleport")) {
							SimpleVector3<float> location = SimpleVector3<float>(globalState.globe->GetPointScaledDegrees(marker.latitude, marker.longitude, marker.altitude));
							globalState.EmitEvent("Teleport","", &location);
						}
						ImGui::SameLine();
						// handle deletion where the button must be clicked twice
						static int deleteId = -1;
						static double deleteTime = -1;
						if(now - deleteTime > 5){
							deleteId = -1;
						}
						bool isSelectedForDeletion = deleteId == id;
						if(isSelectedForDeletion){
							ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0, 0.6f, 0.6f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0, 0.7f, 0.7f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0, 0.8f, 0.8f));
						}
						if(ImGui::Button("Delete")) {
							if(isSelectedForDeletion){
								globalState.locationMarkers.erase(globalState.locationMarkers.begin()+id);
								markersChanged = true;
								deleteId = -1;
							}else{
								deleteId = id;
								deleteTime = now;
							}
						}
						if(isSelectedForDeletion){
							ImGui::PopStyleColor(3);
						}
						
						ImGui::PopID();
					}
					ImGui::PopItemWidth();
					ImGui::Separator();
					if (ImGui::Button("New Waypoint")) {
						GlobalState::Waypoint newWaypoint = {};
						newWaypoint.name = "New";
						newWaypoint.latitude = globalState.globe->GetTopLatitudeDegrees();
						newWaypoint.longitude = globalState.globe->GetTopLongitudeDegrees();
						newWaypoint.altitude = globalState.globe->GetLocationScaled(SimpleVector3<>(0, 0, 0)).radius();
						globalState.locationMarkers.push_back(newWaypoint);
						markersChanged = true;
					}
					if(markersChanged){
						globalState.EmitEvent("LocationMarkersUpdate");
					}
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			//ImGui::Separator();

		}
		
		if (ImGui::CollapsingHeader("Settings")) {
			if (ImGui::TreeNodeEx("Display", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
				if(CustomFloatInput("Max FPS", 20, 120, &globalState.maxFPS, &globalState.defaults->maxFPS)){
					UpdateEngineSettings();
				}
				if(ImGui::Checkbox("VSync", &globalState.vsync)){
					UpdateEngineSettings();
				}
				
				ImGuiComboFlags flags = 0;
				const char* qualityNames[] =  { "Custom", "GPU Melter", "Very High", "High", "Normal", "Low", "Very Low", "Potato" };
				const float qualityValues[] = { 10,       3,            2,           1,      0,        -1,    -2,         -10      };
				int qualityCurrentIndex = 0; 
				for (int n = 0; n < IM_ARRAYSIZE(qualityNames); n++){
					if(globalState.quality == qualityValues[n]){
						qualityCurrentIndex = n;
						break;
					}
				}
				const char* comboPreviewValue = qualityNames[qualityCurrentIndex];  // Pass in the preview value visible before opening the combo (it could be anything)
				if (ImGui::BeginCombo("Quality", comboPreviewValue, flags)){
					for (int n = 0; n < IM_ARRAYSIZE(qualityNames); n++){
						const bool isSelected = (qualityCurrentIndex == n);
						if (ImGui::Selectable(qualityNames[n], isSelected)){
							//qualityCurrentIndex = n;
							if(globalState.quality != qualityValues[n]){
								globalState.quality = qualityValues[n];
								globalState.EmitEvent("UpdateVolumeParameters");
								globalState.testFloat = globalState.quality;
							}
						}
						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						if (isSelected){
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				
				if(globalState.quality == 10 && CustomFloatInput("Step Size", 0.1f, 20.0f, &globalState.qualityCustomStepSize, &globalState.defaults->qualityCustomStepSize)){
					globalState.EmitEvent("UpdateVolumeParameters");
				}
				
				if(ImGui::Checkbox("Enable Fuzz", &globalState.enableFuzz)){
					globalState.EmitEvent("UpdateVolumeParameters");
				}
				
				ImGui::Checkbox("Enable TAA", &globalState.temporalAntiAliasing);
				
				if (ImGui::Button("External Settings Window")) {
					if(uiWindow != NULL && !globalState.vrMode){
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
					UpdateEngineSettings();
					//ImGui::SetWindowCollapsed(true);
					ExternalWindow();
				}
				ImGui::TreePop();
			}
			
			if (ImGui::TreeNodeEx("Reset", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				if(ImGui::Button("Reset Basic Settings")) {
					globalState.EmitEvent("ResetBasicSettings");
				}
				{
					// button must be clicked twice
					static bool selectedForDeletion = false;
					static double deleteTime = false;
					if(now - deleteTime > 5){
						selectedForDeletion = false;
					}
					bool applyRedStyle = selectedForDeletion;
					if(applyRedStyle){
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0, 0.6f, 0.6f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0, 0.7f, 0.7f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0, 0.8f, 0.8f));
					}
					if(ImGui::Button("Reset All Settings")) {
						if(selectedForDeletion){
							globalState.EmitEvent("ResetAllSettings");
							selectedForDeletion = false;
						}else{
							selectedForDeletion = true;
							deleteTime = now;
						}
					}
					if(applyRedStyle){
						ImGui::PopStyleColor(3);
					}
				}
				ImGui::TreePop();
			}
				
			if (ImGui::TreeNodeEx("Joke", ImGuiTreeNodeFlags_SpanAvailWidth)) {	
				ImGui::Checkbox("Audio Height", &globalState.audioControlledHeight);
				ImGui::Checkbox("Audio Opacity", &globalState.audioControlledOpacity);
				ImGui::Checkbox("Audio Cutoff", &globalState.audioControlledCutoff);
				CustomFloatInput("Audio Multiplier", 1.0f, 10.0f, &globalState.audioControlMultiplier, &globalState.defaults->audioControlMultiplier);
				ImGui::TreePop();
			}
		}
		
		if(globalState.opacityMultiplier + 0.9f < 0.001f && globalState.cutoff == 1.0f){
			globalState.developmentMode = true;
		}
		
		if (globalState.developmentMode && ImGui::CollapsingHeader("Ligma", ImGuiTreeNodeFlags_DefaultOpen)) {
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
			ImGui::SameLine();
			if (ImGui::Button("Show Console")) {
				NativeAPI::ShowConsole();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cache State")) {
				globalState.devShowCacheState = !globalState.devShowCacheState;
			}
			if (ImGui::TreeNodeEx("Bad", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				static int leakRunCount = 1;
				static bool leakImmediateFree = true;
				ImGui::InputInt("Leak Run Count", &leakRunCount);
				ImGui::Checkbox("Immediately Free Textures", &leakImmediateFree);
				if (ImGui::Button("Run Transient Leak")) {
					for(int i = 0; i < leakRunCount; i++){
						FName TextureName = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), TEXT("LeakTest"));
						UE_LOG(LogTemp, Display, TEXT("Created texture /Engine/Transient.%s"), *(TextureName.ToString()));
						UTexture2D* texture = UTexture2D::CreateTransient(4096, 4096, EPixelFormat::PF_B8G8R8A8, TextureName);
						if(leakImmediateFree){
							texture->ConditionalBeginDestroy();
						}
					}
				}
				ImGui::TreePop();
			}
			
			static int fileIndex = 0;
			ImGui::SetNextItemWidth(200);
			ImGui::InputInt("Jump pos", &fileIndex);
			ImGui::SameLine();
			if (ImGui::Button("Jump")) {
				size_t fileIndexValue = fileIndex;
				globalState.EmitEvent("JumpToIndex", "", &fileIndexValue);
			}
			
			#if WITH_EDITOR
			static std::string assetName = "/Engine/Transient.VolumeTexture1";
			ImGui::InputText("Asset", &assetName);
			if (ImGui::Button("Open Asset")) {
				//FAssetEditorManager::Get().OpenEditorsForAssets(assetName.c_str());
				FString assetPath = FString(assetName.c_str());
				//GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(assetPath);
				UObject* foundObject = FindObject<UObject>(ANY_PACKAGE, *assetPath);
				if (foundObject != NULL)
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(foundObject);
				}
			}
			/*ImGui::SameLine();
			if (ImGui::Button("Asset List")) {
				//FAssetEditorManager::Get().OpenEditorsForAssets(assetName.c_str());
				FString assetPath = FString(assetName.c_str());
				//GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(assetPath);
				//UPackage* Package = LoadPackage(NULL, *assetPath, LOAD_NoRedirects);
				UPackage* package = FindPackage(NULL, *assetPath);
				//UObject* package = FindObject<UObject>(ANY_PACKAGE, *assetPath);;

				if (package)
				{
					//package->FullyLoad();
					fprintf(stderr, "found");

					FString assetBaseName = FPaths::GetBaseFilename(assetPath);
					UObject* foundObject = FindObject<UObject>(package, *assetBaseName);


					if (foundObject != NULL)
					{
						GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(foundObject);
					}
				}
			}*/
			#endif
			
			ImGui::Text("Custom float input:");
			CustomFloatInput("test float", 0, 3, &globalState.testFloat);
			CustomFloatInput("test float##2", 0, 2, &globalState.testFloat, &globalState.defaults->testFloat);
			
			CustomFloatInput("gui scale", 1, 2, &globalState.guiScale, &globalState.defaults->guiScale);
			
			
			ImGui::Checkbox("Scalability Test", &scalabilityTest);
			if (ImGui::Button("Test")) {
				ligma(GS->globalState.testBool);
			}
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
	
	if(io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L)){
		globalState.developmentMode = !globalState.developmentMode;
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
	//FImGuiModule::Get().GetProperties().SetInputEnabled(Value);
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	fprintf(stderr,"Test\n");
	globalState->EmitEvent("Test");
	globalState->EmitEvent("TestUnregistered");
	#ifdef HDF5
		fprintf(stderr,"HDF5 module enabled\n");
	#endif
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

// send controls to an external window
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

// close external settings window and move controls to main viewport
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
		// FString radarDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("../files/dir/"));
		// FString fullradarDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*radarDir);
		// const char* radarDirLocaition = StringCast<ANSICHAR>(*fullradarDir).Get();
		fileChooser = new pfd::public_open_file("Open Radar Files", "", { "All Files", "*" }, pfd::opt::multiselect);
	}
}
void AImGuiUI::UpdateEngineSettings() {
	GlobalState& globalState = GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.VSync %i"), globalState.vsync && !globalState.vrMode));
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.VSyncEditor %i"), globalState.vsync && !globalState.vrMode));
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("t.MaxFPS %f"), (globalState.maxFPS == 0 || globalState.vrMode) ? 0 : std::max(1.0f,globalState.maxFPS)));
}
