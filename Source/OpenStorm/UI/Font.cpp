#include "Font.h"
#include "imgui.h"
#include "ImGuiModule.h"
#include "FontData.h"







void LoadFonts(){
    //static const ImWchar OpenFontIcons_ranges[] = { 0xe000, 0xe0fe, 0 }; // Will not be copied by AddFont* so keep in scope.
    static const ImWchar FontAwesome_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 }; // Will not be copied by AddFont* so keep in scope.
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	ImFontConfig config = {};
    strncpy(config.Name, "Roboto and Font Awesome", 24);
	//ImFont* font = io.Fonts->AddFontDefault();
	//ImFont* fontRoboto = io.Fonts->AddFontFromMemoryCompressedTTF(Roboto_compressed_data, Roboto_compressed_size, 32, &config);
	ImFont* fontRoboto = io.Fonts->AddFontFromMemoryCompressedBase85TTF(Roboto_compressed_data_base85, 32, &config);
    fontRoboto->Scale = 0.4;
    config.MergeMode = true;
	//config.SizePixels = 0.1;
	config.GlyphOffset = ImVec2(-2, 1.5);
	//ImFont* fontOpenFontIcons = io.Fonts->AddFontFromMemoryCompressedTTF(OpenFontIcons_compressed_data, OpenFontIcons_compressed_size, 32, &config, OpenFontIcons_ranges);
	ImFont* fontIcons = io.Fonts->AddFontFromMemoryCompressedBase85TTF(FontAwesome_compressed_data_base85, 32, &config, FontAwesome_ranges);
    fontIcons->Scale = 0.4;
	//io.Fonts->Build();
    FImGuiModule::Get().RebuildFontAtlas();
}