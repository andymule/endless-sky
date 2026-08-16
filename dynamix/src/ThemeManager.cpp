#include "ThemeManager.h"

namespace Dynamix {

    void ThemeManager::ApplyTheme(Theme theme) {
        switch (theme) {
            case Theme::DARK:
                ApplyDarkTheme();
                break;
            case Theme::RED:
                ApplyRedTheme();
                break;
            case Theme::LIGHT:
                ApplyLightTheme();
                break;
        }
    }

    const char* ThemeManager::GetThemeName(Theme theme) {
        switch (theme) {
            case Theme::DARK:
                return "Dark";
            case Theme::RED:
                return "Red";
            case Theme::LIGHT:
                return "Light";
            default:
                return "Unknown";
        }
    }

    int ThemeManager::GetThemeCount() {
        return 3; // Dark, Red, Light
    }

    ThemeManager::Theme ThemeManager::IndexToTheme(int index) {
        switch (index) {
            case 0:
                return Theme::DARK;
            case 1:
                return Theme::RED;
            case 2:
                return Theme::LIGHT;
            default:
                return Theme::DARK;
        }
    }

    int ThemeManager::ThemeToIndex(Theme theme) { return static_cast<int>(theme); }

    void ThemeManager::ApplyDarkTheme() {
        // Use ImGui's built-in dark theme
        ImGui::StyleColorsDark();
    }

    void ThemeManager::ApplyRedTheme() {
        // Start with dark as base
        ImGui::StyleColorsDark();

        // Apply custom red theme
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Window and background colors - dark red tones
        colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.05f, 0.05f, 0.94f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.04f, 0.04f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.07f, 0.07f, 1.00f);

        // Frame colors - buttons, sliders, etc.
        colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.08f, 0.08f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.35f, 0.12f, 0.12f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.45f, 0.15f, 0.15f, 0.67f);

        // Title bar colors
        colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.07f, 0.07f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.30f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.05f, 0.05f, 1.00f);

        // Button colors
        colors[ImGuiCol_Button] = ImVec4(0.35f, 0.12f, 0.12f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.20f, 0.20f, 1.00f);

        // Header colors (for collapsing headers, selectables)
        colors[ImGuiCol_Header] = ImVec4(0.40f, 0.14f, 0.14f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.18f, 0.18f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.20f, 0.20f, 1.00f);

        // Separator and border colors
        colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.15f, 0.15f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.55f, 0.19f, 0.19f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.65f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.10f, 0.10f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        // Resize grip colors
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.45f, 0.15f, 0.15f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.55f, 0.19f, 0.19f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.65f, 0.22f, 0.22f, 0.95f);

        // Tab colors
        colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.08f, 0.08f, 0.86f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.45f, 0.15f, 0.15f, 0.80f);
        colors[ImGuiCol_TabSelected] = ImVec4(0.35f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TabDimmed] = ImVec4(0.20f, 0.07f, 0.07f, 0.97f);
        colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.30f, 0.10f, 0.10f, 1.00f);

        // Scrollbar colors
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.04f, 0.04f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.19f, 0.19f, 1.00f);

        // Check mark and slider grab colors
        colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.28f, 0.28f, 1.00f);

        // Text colors
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.85f, 0.85f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.60f, 0.20f, 0.20f, 0.35f);

        // Plot colors (for any graphs or plots)
        colors[ImGuiCol_PlotLines] = ImVec4(0.80f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.32f, 0.32f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.70f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.80f, 0.28f, 0.28f, 1.00f);

        // Table colors
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.25f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.35f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.25f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);

        // Drag and drop colors
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.80f, 0.28f, 0.28f, 0.90f);

        // Navigation colors
        colors[ImGuiCol_NavCursor] = ImVec4(0.80f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    void ThemeManager::ApplyLightTheme() {
        // Start with ImGui's light theme as base
        ImGui::StyleColorsLight();

        // Apply custom light theme adjustments
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Enhance the light theme with better contrast and modern colors

        // Window and background colors - light tones
        colors[ImGuiCol_WindowBg] = ImVec4(0.96f, 0.96f, 0.96f, 0.94f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

        // Frame colors - buttons, sliders, etc.
        colors[ImGuiCol_FrameBg] = ImVec4(0.85f, 0.85f, 0.85f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.75f, 0.75f, 0.75f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.65f, 0.65f, 0.65f, 0.67f);

        // Title bar colors
        colors[ImGuiCol_TitleBg] = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);

        // Button colors
        colors[ImGuiCol_Button] = ImVec4(0.78f, 0.78f, 0.78f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.58f, 0.58f, 0.58f, 1.00f);

        // Header colors (for collapsing headers, selectables)
        colors[ImGuiCol_Header] = ImVec4(0.72f, 0.72f, 0.72f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.62f, 0.62f, 0.62f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);

        // Separator and border colors
        colors[ImGuiCol_Separator] = ImVec4(0.60f, 0.60f, 0.60f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.50f, 0.50f, 0.50f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        // Resize grip colors
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.65f, 0.65f, 0.65f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.55f, 0.55f, 0.55f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.45f, 0.45f, 0.45f, 0.95f);

        // Tab colors
        colors[ImGuiCol_Tab] = ImVec4(0.85f, 0.85f, 0.85f, 0.86f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.75f, 0.75f, 0.75f, 0.80f);
        colors[ImGuiCol_TabSelected] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_TabDimmed] = ImVec4(0.90f, 0.90f, 0.90f, 0.97f);
        colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);

        // Scrollbar colors
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.88f, 0.88f, 0.88f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

        // Check mark and slider grab colors
        colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

        // Text colors
        colors[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.60f, 0.60f, 0.60f, 0.35f);

        // Plot colors (for any graphs or plots)
        colors[ImGuiCol_PlotLines] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

        // Table colors
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.30f, 0.30f, 0.30f, 0.06f);

        // Drag and drop colors
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.30f, 0.30f, 0.30f, 0.90f);

        // Navigation colors
        colors[ImGuiCol_NavCursor] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

} // namespace Dynamix