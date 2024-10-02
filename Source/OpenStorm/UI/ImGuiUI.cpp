#include "ImGuiUI.h"
#include "Font.h"
#include "Native.h"
#include "../Radar/SystemAPI.h"
#include "../Radar/Products/RadarProduct.h"
#include "../Radar/Globe.h"
#include "../Radar/NexradSites/NexradSites.h"
#include "../Objects/RadarGameStateBase.h"
#include "../Objects/RadarViewPawn.h"
#include "ImGuiController.h"
#include "../EngineHelpers/StringUtils.h"
#include "./portable-file-dialogs.h"
#include "./ClickableInterface.h"

#include <vector>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "ImGuiModule.h"

#include "HAL/FileManager.h"

#if WITH_EDITOR
//#include "Toolkits/AssetEditorManager.h"
// #include "Engine/Engine.h"
// #include "EngineGlobals.h"
// #include "Editor/EditorEngine.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#endif

static bool inlineLabel = false;

typedef int CustomInputFlags;
enum CustomInputFlags_{
	CustomInput_SliderOnly = 1, // Only show the slider. Only use this if values outside of the slider value do absolutely nothing
	CustomInput_Short = 2 // make it shorter
};

// display a tooltip for the previous element
void CustomTooltipForPrevious(const char* tooltipText){
	static double hoverStart = 0;
	static ImGuiID lastId = 0;
	static ImVec2 lastMousePos;
	ImGuiID itemId = ImGui::GetHoveredID();
	// fprintf(stderr, "%i\n", itemId);
	if (ImGui::IsItemHovered(/*ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay*/)){
		double now = SystemAPI::CurrentTime();
		// fprintf(stderr, "%i %i %f\n", itemId, lastId, hoverStart);
		if(itemId != lastId || hoverStart == 0){
			hoverStart = now;
			lastId = itemId;
		}
		if(now - hoverStart > 0.5){
			#ifdef __clang__
			#pragma clang diagnostic push
			#pragma clang diagnostic ignored "-Wformat-security"
			#endif
			ImGui::SetTooltip(tooltipText);
			#ifdef __clang__
			#pragma clang diagnostic pop
			#endif
		}
	}
	ImVec2 mousePos = ImGui::GetMousePos();
	if(itemId != lastId || lastMousePos.x != mousePos.x || lastMousePos.y != mousePos.y){
		lastMousePos = mousePos;
		hoverStart = 0;
	}
}

template <typename T>
bool ResetButton(T* value, T* defaultValue = NULL){
	float frameHeight = ImGui::GetFrameHeight();
	if(ImGui::Button(ICON_FA_DELETE_LEFT ,ImVec2(frameHeight, frameHeight))){
		*value = *defaultValue;
		return true;
	}
	CustomTooltipForPrevious("Reset value");
	return false;
}

static const char* customInputToolTipText = NULL;

// set the tooltip text for the next custom input
void SetCustomInputTooltip(const char* tooltipText){
	customInputToolTipText = tooltipText;
}

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
		if(customInputToolTipText){
			CustomTooltipForPrevious(customInputToolTipText);
		}
	}
	if(!(flags & CustomInput_SliderOnly)){
		ImGui::PushItemWidth(8 * fontSize);
		changed |= ImGui::InputFloat("##floatInput", value, 0.1f, 1.0f, "%.2f");
		ImGui::PopItemWidth();
		if(customInputToolTipText){
			CustomTooltipForPrevious(customInputToolTipText);
		}
		ImGui::SameLine();
	}
	float sliderWidth = (flags & CustomInput_SliderOnly && (flags & CustomInput_Short) == 0) ? 20 * fontSize + style.ItemSpacing.x : 12 * fontSize;
	if(defaultValue != NULL){
		float frameHeight = ImGui::GetFrameHeight();
		ImGui::PushItemWidth(sliderWidth - frameHeight - style.ItemSpacing.x);
		changed |= ImGui::SliderFloat("##floatSlider", value, minSlider, maxSlider);
		ImGui::PopItemWidth();
		if(customInputToolTipText){
			CustomTooltipForPrevious(customInputToolTipText);
		}
		ImGui::SameLine();
		changed |= ResetButton(value, defaultValue);
	}else{
		ImGui::PushItemWidth(sliderWidth);
		changed |= ImGui::SliderFloat("##floatSlider", value, minSlider, maxSlider);
		ImGui::PopItemWidth();
		if(customInputToolTipText){
			CustomTooltipForPrevious(customInputToolTipText);
		}
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
		if(customInputToolTipText){
			CustomTooltipForPrevious(customInputToolTipText);
		}
	}
	ImGui::PopID();
	customInputToolTipText = NULL;
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
		if(customInputToolTipText){
			CustomTooltipForPrevious(customInputToolTipText);
		}
	}
	float itemWidth = ((flags & CustomInput_Short) == 0) ? 20 * fontSize : 12 * fontSize;
	itemWidth += style.ItemSpacing.x;
	if(defaultValue != NULL){
		float frameHeight = ImGui::GetFrameHeight();
		ImGui::PushItemWidth(itemWidth - frameHeight - style.ItemSpacing.x);
		changed |= ImGui::InputText("##text", value);
		if(customInputToolTipText){
			CustomTooltipForPrevious(customInputToolTipText);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		changed |= ResetButton(value, defaultValue);
	}else{
		ImGui::PushItemWidth(itemWidth);
		changed |= ImGui::InputText("##text", value);
		if(customInputToolTipText){
			CustomTooltipForPrevious(customInputToolTipText);
		}
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
		if(customInputToolTipText){
			CustomTooltipForPrevious(customInputToolTipText);
		}
	}
	ImGui::PopID();
	customInputToolTipText = NULL;
	return changed;
}

// create a combo element
template <typename T>
bool EnumSelectable(const char* label, T* setting, const T value){
	bool isSelected = *setting == value;
	if (ImGui::Selectable(label, isSelected)) {
		if (!isSelected) {
			*setting = value;
			return true;
		}
	}
	if (isSelected) {
		ImGui::SetItemDefaultFocus();
	}
	return false;
}

/**
 * create combo for setting based on array of values
 * valueLabels is an array of names for items in dropdown
 * values is an array of values for items in dropdown
 * count is the length of valueLabels and values
*/
template <typename T>
bool EnumCombo(const char* label, T* setting, const char** valueLabels, T* values, int count){
	const char* comboPreviewValue = "Invalid";
	bool changed = false;
	for(int i = 0; i < count; i++){
		if(values[i] == *setting){
			comboPreviewValue = valueLabels[i];
		}
	}
	if (ImGui::BeginCombo(label, comboPreviewValue)){
		for(int i = 0; i < count; i++){
			changed |= EnumSelectable(valueLabels[i], setting, values[i]);
		}
		ImGui::EndCombo();
	}
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
ImGuiUI::ImGuiUI(){
	
}
ImGuiUI::~ImGuiUI(){
	
}


//const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());

// Called every frame
void ImGuiUI::MainUI()
{
	GlobalState &globalState = *imGuiController->GetGlobalState();
	double now = SystemAPI::CurrentTime();
	ImGuiIO& io = ImGui::GetIO();
	
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
			float oldRepeatRate = io.KeyRepeatRate;
			io.KeyRepeatRate = 0.0001f;
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
			io.KeyRepeatRate = oldRepeatRate;
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
				CustomTooltipForPrevious("Which radar product to display");
				
				SetCustomInputTooltip("Limits the minimum value that will be displayed in the radar volume");
				CustomFloatInput("Cutoff", 0, 1, &globalState.cutoff, &globalState.defaults->cutoff, CustomInput_SliderOnly);
				SetCustomInputTooltip("How transparent the radar volume is with larger values being more opaque");
				CustomFloatInput("Opacity", 0.2, 4, &globalState.opacityMultiplier, &globalState.defaults->opacityMultiplier);
				SetCustomInputTooltip("Multiplies the height of the radar volume");
				CustomFloatInput("Height Exaggeration", 1, 4, &globalState.verticalScale, &globalState.defaults->verticalScale);
				
				bool spatialInterpolationOldValue = globalState.spatialInterpolation;
				ImGui::Checkbox("Spatial Interpolation", &globalState.spatialInterpolation);
				CustomTooltipForPrevious("If the pixels in the volume should be smoothed together");
				if(spatialInterpolationOldValue != globalState.spatialInterpolation){
					globalState.EmitEvent("UpdateVolumeParameters");
				}
				
				ImGui::Checkbox("Temporal Interpolation", &globalState.temporalInterpolation);
				CustomTooltipForPrevious("If the time in between the volumes should be smoothed together");
				
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
				CustomTooltipForPrevious("Select if the view is 3D or 2D by dragging");
				
				if(globalState.viewMode == GlobalState::VIEW_MODE_SLICE){
					const char* sliceModes[] = {"Invalid", "Sweep Angle", "Constant Altitude", "Vertical", "Invalid"};
					ImGui::PushItemWidth(15 * fontSize);
					ImGui::SliderInt("Slice Mode", (int*)&globalState.sliceMode, 0, 2, sliceModes[std::clamp(globalState.sliceMode + 1, 0, 4)]);
					ImGui::PopItemWidth();
					CustomTooltipForPrevious("Select how the radar volume is sliced by dragging");
					if(globalState.sliceMode == GlobalState::SLICE_MODE_CONSTANT_ALTITUDE){
						SetCustomInputTooltip("The altitude of the slice in meters");
						CustomFloatInput("Slice Altitude (meters)", 0, 25000, &globalState.sliceAltitude, &globalState.defaults->sliceAltitude);
					}
					if(globalState.sliceMode == GlobalState::SLICE_MODE_SWEEP_ANGLE){
						SetCustomInputTooltip("The angle of the slice in degrees");
						CustomFloatInput("Slice Angle (degrees)", 0.5, 19.5, &globalState.sliceAngle, &globalState.defaults->sliceAngle);
					}
					if(globalState.sliceMode == GlobalState::SLICE_MODE_VERTICAL){
						SetCustomInputTooltip("The rotation of the slice in degrees");
						CustomFloatInput("Slice Rotation", 0, 180, &globalState.sliceVerticalRotation, &globalState.defaults->sliceVerticalRotation, CustomInput_SliderOnly);
						SetCustomInputTooltip("The location of the slice where positive is east");
						CustomFloatInput("Slice X location", -1, 1, &globalState.sliceVerticalLocationX, &globalState.defaults->sliceVerticalLocationX, CustomInput_SliderOnly);
						SetCustomInputTooltip("The location of the slice where positive is north");
						CustomFloatInput("Slice Y location", -1, 1, &globalState.sliceVerticalLocationY, &globalState.defaults->sliceVerticalLocationY, CustomInput_SliderOnly);
					}
					ImGui::Checkbox("Volumetric Slice", &globalState.sliceVolumetric);
					CustomTooltipForPrevious("If the slice should be rendered as a volumetric slice instead of 2D");
				}
				
				ImGui::TreePop();
			}
			ImGui::Separator();
			
			if (ImGui::TreeNodeEx("Movement", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				SetCustomInputTooltip("How fast you move around, holding down shift will multiply this by 4");
				CustomFloatInput("Movement Speed", 10, 1500, &globalState.moveSpeed, &globalState.defaults->moveSpeed);
				
				SetCustomInputTooltip("How sensitive the mouse or controller is to rotation");
				CustomFloatInput("Rotation Speed", 0.0f, 300.0f, &globalState.rotateSpeed, &globalState.defaults->rotateSpeed);
				ImGui::TreePop();
			}
			ImGui::Separator();
			
			if (ImGui::TreeNodeEx("Animation", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				ImGui::Checkbox("Time", &globalState.animate);
				CustomTooltipForPrevious("Will play through volumes when checked, same as play button");
				const char* comboNames[] = {"Default", "Loop Loaded", "Loop All", "Bounce", "Bounce Loaded", "None"};
				GlobalState::LoopMode comboValues[] = {GlobalState::LOOP_MODE_DEFAULT, GlobalState::LOOP_MODE_CACHE, GlobalState::LOOP_MODE_ALL, GlobalState::LOOP_MODE_BOUNCE, GlobalState::LOOP_MODE_CACHE_BOUNCE, GlobalState::LOOP_MODE_NONE};
				ImGui::PushItemWidth(12 * fontSize);
				EnumCombo("Loop Mode", &globalState.animateLoopMode, comboNames, comboValues, sizeof(comboNames)/sizeof(comboNames[0]));
				CustomTooltipForPrevious(
					"Changes how it behaves when the end of the data is reached while animating\n"
					"Default: go until end and then repeat loaded data at the end\n"
					"Loop Loaded: repeat loaded data around the currently selected volume\n"
					"Loop All: go to other side of data when the end is reached\n"
					"Bounce: reverse direction when reaching the end\n"
					"Bounce Loaded: bounce through loaded data around the currently selected volume\n"
					"None: stop when reaching the end\n"
				);
				ImGui::PopItemWidth();
				SetCustomInputTooltip("How many volumes per second to play through");
				CustomFloatInput("Time Animation Speed", 1, 20, &globalState.animateSpeed, &globalState.defaults->animateSpeed);
				ImGui::Checkbox("Cuttoff", &globalState.animateCutoff);
				CustomTooltipForPrevious("Will animate between a cutoff of zero and the value currently set for cutoff\nThe cutoff must be more than zero for this to work");
				SetCustomInputTooltip("How fast to animate the cutoff value");
				CustomFloatInput("Cutoff Animation Speed", 0.1, 2, &globalState.animateCutoffSpeed, &globalState.defaults->animateCutoffSpeed);
				ImGui::TreePop();
			}
			ImGui::Separator();
			if (ImGui::TreeNodeEx("Data", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Poll Data", &globalState.pollData);
				CustomTooltipForPrevious("If new data should be automatically loaded from the selected data");
				if (ImGui::Button("Load Files")) {
					ChooseFiles();
				}
				CustomTooltipForPrevious("Load a directory of radar files");
				if(globalState.openDownloadDropdown){
					ImGui::SetNextItemOpen(true);
					globalState.openDownloadDropdown = false;
				}
				if (ImGui::TreeNodeEx("Download", ImGuiTreeNodeFlags_SpanAvailWidth)) {
					ImGui::Text("Download data:");
					CustomTooltipForPrevious("Download data off of the internet");
					if(globalState.downloadData){
						ImGui::BeginDisabled();
					}
					if(ImGui::Button("Start")){
						globalState.downloadData = true;
						globalState.pollData = true;
					}
					CustomTooltipForPrevious("Start downloading data");
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
					CustomTooltipForPrevious("Stop downloading data");
					if(!globalState.downloadData){
						ImGui::EndDisabled();
					}
					
					SetCustomInputTooltip("How often to check for updated radar files on the internet");
					CustomFloatInput("Download interval (seconds)", 30, 300, &globalState.downloadPollInterval, &globalState.defaults->downloadPollInterval, CustomInput_SliderOnly | CustomInput_Short);
					
					ImGui::PushItemWidth(11 * fontSize - ImGui::GetStyle().ItemSpacing.x * 1.5f);
					ImGui::Text("Download Previous File Count");
					CustomTooltipForPrevious("Number of files before the current one to download off the internet");
					ImGui::InputInt("##previousFileCount", &globalState.downloadPreviousCount);
					CustomTooltipForPrevious("Number of files before the current one to download off the internet");
					ImGui::SameLine();
					ResetButton(&globalState.downloadPreviousCount, &globalState.defaults->downloadPreviousCount);
					ImGui::PopItemWidth();
					
					
					// delete dropdown
					static float downloadDeleteAfterLocal = globalState.downloadDeleteAfter;
					static bool deletePopupOpen = false;
					const char* comboNames[] = {"Never", "2 Hours", "6 Hours", "12 Hours", "1 Day",  "2 Days",  "7 Days",  "30 Days",  "100 Days",  "1 Year"};
					float comboValues[] =        {0,       2*3600,     6*3600,    12*3600,    24*3600, 2*24*3600, 7*24*3600, 30*24*3600, 100*24*3600, 365*24*3600};
					ImGui::Text("Delete files after");
					ImGui::PushItemWidth(12 * fontSize);
					EnumCombo("##deleteFilesAfter", &downloadDeleteAfterLocal, comboNames, comboValues, sizeof(comboNames)/sizeof(comboNames[0]));
					CustomTooltipForPrevious("Automatically delete downloaded radar files when they are older than this amount of time");
					ImGui::PopItemWidth();
					if(downloadDeleteAfterLocal > 0 && (globalState.downloadDeleteAfter <= 0 || globalState.downloadDeleteAfter > downloadDeleteAfterLocal) && !deletePopupOpen){
						// prompt user before possibly deleting files
						ImGui::OpenPopup("Confirm Delete");
						deletePopupOpen = true;
					}else if(downloadDeleteAfterLocal <= 0 || (downloadDeleteAfterLocal > globalState.downloadDeleteAfter && globalState.downloadDeleteAfter > 0)){
						// already enabled and increasing time so just set the new value
						globalState.downloadDeleteAfter = downloadDeleteAfterLocal;
					}
					if (ImGui::BeginPopup("Confirm Delete")){
						ImGui::Text("Enabling this will delete downloaded radar files older than the selected time.");
						ImGui::Separator();
						ImGui::Text("Are you sure?");
						if (ImGui::Button("Delete Files", ImVec2(120,0))) {
							ImGui::CloseCurrentPopup();
							globalState.downloadDeleteAfter = downloadDeleteAfterLocal;
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel", ImVec2(120,0))) {
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}else if(deletePopupOpen){
						// popup was closed
						deletePopupOpen = false;
						downloadDeleteAfterLocal = globalState.downloadDeleteAfter;
					}
					
					// ImGui::InputFloat("downloadDeleteAfter", &globalState.downloadDeleteAfter);
					
					
					static std::string siteIdSelection = "";
					ImGui::PushID("site_button");
					ImGui::Text("Select Site:");
					if (ImGui::Button(globalState.downloadSiteId.c_str())){
						siteIdSelection = globalState.downloadSiteId;
						ImGui::OpenPopup("Select Site");
					}
					CustomTooltipForPrevious("Select the site to download radar data for");
					
					if (ImGui::BeginPopup("Select Site")){
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
						SetCustomInputTooltip("What http server to download data from");
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
				CustomTooltipForPrevious("Toggle entire map");
				SetCustomInputTooltip("How bright the map is, increasing this makes the radar volume less visible");
				CustomFloatInput("Map Brightness", 0.01, 1.0, &globalState.mapBrightness, &globalState.defaults->mapBrightness);
				ImGui::Checkbox("Show Tiles", &globalState.enableMapTiles);
				CustomTooltipForPrevious("Show the satellite imagery");
				ImGui::Checkbox("Show GIS info", &globalState.enableMapGIS);
				CustomTooltipForPrevious("Show boundaries and roads");
				SetCustomInputTooltip("How bright boundaries are compared to the rest of the map");
				CustomFloatInput("GIS Brightness", 0.01, 1.5, &globalState.mapBrightnessGIS, &globalState.defaults->mapBrightnessGIS);
				ImGui::Checkbox("Show Radar Sites", &globalState.enableSiteMarkers);
				CustomTooltipForPrevious("Show the clickable radar site markers");
				float siteColor[4] = {globalState.siteMarkerColorR / 255.0f, globalState.siteMarkerColorG / 255.0f, globalState.siteMarkerColorB / 255.0f, globalState.siteMarkerColorA / 255.0f};
				bool siteColorChanged = ImGui::ColorEdit4("Radar Site Color", siteColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Uint8);
				CustomTooltipForPrevious("Change color of radar site markers");
				if(siteColorChanged){
					globalState.siteMarkerColorR = (int)(siteColor[0] * 255);
					globalState.siteMarkerColorG = (int)(siteColor[1] * 255);
					globalState.siteMarkerColorB = (int)(siteColor[2] * 255);
					globalState.siteMarkerColorA = (int)(siteColor[3] * 255);
					globalState.EmitEvent("LocationMarkersUpdate");
				}
				
				if (ImGui::TreeNodeEx("Waypoints", ImGuiTreeNodeFlags_SpanAvailWidth)) {
					CustomTooltipForPrevious("Create your own markers");
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
						CustomTooltipForPrevious("Name of the marker displayed on the map");
						markersChanged |= ImGui::InputDouble("Latitude", &marker.latitude);
						CustomTooltipForPrevious("Latitude of the marker in degrees");
						markersChanged |= ImGui::InputDouble("Longitude", &marker.longitude);
						CustomTooltipForPrevious("Longitude of the marker in degrees");
						markersChanged |= ImGui::InputDouble("Altitude", &marker.altitude);
						CustomTooltipForPrevious("Altitude of the marker in meters");
						markersChanged |= ImGui::Checkbox("", &marker.enabled);
						CustomTooltipForPrevious("Show the marker on the map");
						ImGui::SameLine();
						float markerColor[4] = {marker.colorR / 255.0f, marker.colorG / 255.0f, marker.colorB / 255.0f, marker.colorA / 255.0f};
						bool markerColorChanged = ImGui::ColorEdit4("Color", markerColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_Uint8);
						if(markerColorChanged){
							marker.colorR = (int)(markerColor[0] * 255);
							marker.colorG = (int)(markerColor[1] * 255);
							marker.colorB = (int)(markerColor[2] * 255);
							marker.colorA = (int)(markerColor[3] * 255);
							markersChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("Teleport")) {
							SimpleVector3<float> location = SimpleVector3<float>(globalState.globe->GetPointScaledDegrees(marker.latitude, marker.longitude, marker.altitude));
							globalState.EmitEvent("Teleport","", &location);
						}
						CustomTooltipForPrevious("Teleport to marker");
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
						CustomTooltipForPrevious("Delete the marker");
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
					CustomTooltipForPrevious("Create a new marker");
					if(markersChanged){
						globalState.EmitEvent("LocationMarkersUpdate");
					}
					ImGui::TreePop();
				}else{
					CustomTooltipForPrevious("Create your own markers");
				}
				ImGui::TreePop();
			}
			//ImGui::Separator();

		}
		
		if (ImGui::CollapsingHeader("Settings")) {
			if (ImGui::TreeNodeEx("Display", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
				SetCustomInputTooltip("Maximum number of frames per second to run at");
				if(CustomFloatInput("Max FPS", 20, 120, &globalState.maxFPS, &globalState.defaults->maxFPS)){
					globalState.EmitEvent("UpdateEngineSettings");
				}
				if(ImGui::Checkbox("VSync", &globalState.vsync)){
					globalState.EmitEvent("UpdateEngineSettings");
				}
				CustomTooltipForPrevious("Sync framerate to monitor to prevent screen tearing");
				
				ImGuiComboFlags flags = 0;
				const char* qualityNames[] = { "Custom", "GPU Melter", "Very High", "High", "Normal", "Low", "Very Low", "Potato" };
				float qualityValues[] =      { 10,       3,            2,           1,      0,        -1,    -2,         -10      };
				if(EnumCombo("Quality", &globalState.quality, qualityNames, qualityValues, sizeof(qualityNames)/sizeof(qualityNames[0]))){
					globalState.EmitEvent("UpdateVolumeParameters");
				}
				CustomTooltipForPrevious("Higher quality levels will decrease graininess at expense of performance");
				
				if(globalState.quality == 10){
					if(CustomFloatInput("Step Size", 0.1f, 20.0f, &globalState.qualityCustomStepSize, &globalState.defaults->qualityCustomStepSize)){
						globalState.EmitEvent("UpdateVolumeParameters");
					}
					CustomTooltipForPrevious("Custom step size for ray marching, lower values decrease graininess at expense of performance");
				}
				
				
				if(ImGui::Checkbox("Enable Fuzz", &globalState.enableFuzz)){
					globalState.EmitEvent("UpdateVolumeParameters");
				}
				CustomTooltipForPrevious("Add noise to prevent artifacts");
				
				ImGui::Checkbox("Enable TAA", &globalState.temporalAntiAliasing);
				CustomTooltipForPrevious("TAA decreases graininess but can hide smaller details");
				
				ImGui::PushItemWidth(11 * fontSize - ImGui::GetStyle().ItemSpacing.x * 1.5f);
				if(ImGui::InputInt("Radar Cache Size", &globalState.radarCacheSize)){
					globalState.radarCacheSize = std::clamp(globalState.radarCacheSize, 0, 4000);
				}
				CustomTooltipForPrevious("The number of radar volumes to load into RAM. The cache is shown on the bottom right.");
				ImGui::PopItemWidth();
					
				if (ImGui::Button("External Settings Window")) {
					if(imGuiController->uiWindow != NULL && !globalState.vrMode){
						imGuiController->InternalWindow();
					}else{
						imGuiController->ExternalWindow();
					}
				}
				CustomTooltipForPrevious("Move this panel into its own window");
				ImGui::SameLine();
				if (ToggleButton("VR " ICON_FA_VR_CARDBOARD, globalState.vrMode)) {
					globalState.vrMode = !globalState.vrMode;
					if(globalState.vrMode){
						GEngine->Exec(imGuiController->GetWorld(), TEXT("vr.bEnableStereo 1"));
					}else{
						GEngine->Exec(imGuiController->GetWorld(), TEXT("vr.bEnableStereo 0"));
					}
					globalState.EmitEvent("UpdateEngineSettings");
					//ImGui::SetWindowCollapsed(true);
					imGuiController->ExternalWindow();
				}
				CustomTooltipForPrevious("Enable virtual reality mode if a headset is connected");
				ImGui::TreePop();
			}
			
			if (ImGui::TreeNodeEx("Integration", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				ImGui::Checkbox("Discord presence", &globalState.discordPresence);
				CustomTooltipForPrevious("Show OpenStorm on discord");
				ImGui::TreePop();
			}
			
			if (ImGui::TreeNodeEx("Reset", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				if(ImGui::Button("Reset Basic Settings")) {
					globalState.EmitEvent("ResetBasicSettings");
				}
				CustomTooltipForPrevious("Resets basic view settings, mainly in the radar dropdown");
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
					CustomTooltipForPrevious("Resets all settings to defaults");
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
				imGuiController->unsafeFrames = 10;
				return;
			}

			if (ImGui::Button("External win")) {
				imGuiController->ExternalWindow();
			}
			ImGui::SameLine();
			if (ImGui::Button("Internal win")) {
				// this was thought to be impossible
				imGuiController->InternalWindow();
			}
			ImGui::SameLine();
			if (ImGui::Button("Unlock")) {
				imGuiController->UnlockMouse();
			}
			ImGui::SameLine();
			if (ImGui::Button("Lock")) {
				imGuiController->LockMouse();
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
				ligma(globalState.testBool);
			}
		}
		
		
	}
	ImGui::End();
	
	
	if(showDemoWindow){
		ImGui:: ShowDemoWindow();
	}
	
	
}



void ImGuiUI::ligma(bool Value)
{
	//FImGuiModule::Get().GetProperties().SetInputEnabled(Value);
	GlobalState* globalState = imGuiController->GetGlobalState();
	fprintf(stderr,"Test\n");
	globalState->EmitEvent("Test");
	globalState->EmitEvent("TestUnregistered");
	#ifdef HDF5
		fprintf(stderr,"HDF5 module enabled\n");
	#endif
}



void ImGuiUI::ChooseFiles() {
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

void ImGuiUI::Tick(float deltaTime){
	GlobalState &globalState = *imGuiController->GetGlobalState();
	if(fileChooser != NULL){
		// check if user has taken action on file dialog
		if(fileChooser->ready()){
			std::vector<std::string> files = fileChooser->result();
			if(files.size() > 0){
				globalState.downloadData = false;
				globalState.EmitEvent("LoadDirectory", files[0], NULL);
			}
			delete fileChooser;
			fileChooser = NULL;
		}
	}
}