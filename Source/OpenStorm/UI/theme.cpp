#include "theme.h"
#include "imgui.h"


namespace Theme{
	void Green(){
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_FrameBg]                = ImVec4(0.01f, 0.37f, 0.01f, 0.54f);
		colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.98f, 0.40f, 0.40f);
		colors[ImGuiCol_FrameBgActive]          = ImVec4(0.00f, 0.61f, 0.06f, 0.67f);
		colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.48f, 0.21f, 1.00f);
		colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.98f, 0.38f, 1.00f);
		colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.88f, 0.37f, 1.00f);
		colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.98f, 0.36f, 1.00f);
		colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.98f, 0.46f, 0.40f);
		colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.98f, 0.38f, 1.00f);
		colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.98f, 0.20f, 1.00f);
		colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.98f, 0.28f, 0.31f);
		colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.98f, 0.29f, 0.80f);
		colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.98f, 0.32f, 1.00f);
		colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.75f, 0.12f, 0.78f);
		colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.75f, 0.17f, 1.00f);
		colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.98f, 0.36f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.98f, 0.29f, 0.67f);
		colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.98f, 0.31f, 0.95f);
		colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.58f, 0.26f, 0.86f);
		colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.98f, 0.44f, 0.80f);
		colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.68f, 0.26f, 1.00f);
		colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.15f, 0.09f, 0.97f);
		colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.42f, 0.16f, 1.00f);
		colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.98f, 0.35f, 1.00f);


	}
}