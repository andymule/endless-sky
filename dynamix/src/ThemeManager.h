#pragma once

#include "imgui.h"

namespace Dynamix {

    /**
     * ThemeManager - Manages Dear ImGui color themes for the application
     *
     * Provides a centralized way to define and apply different color themes
     * to the Dear ImGui interface. Supports multiple predefined themes and
     * allows for easy switching between them.
     */
    class ThemeManager {
    public:
        enum class Theme { DARK = 0, RED = 1, LIGHT = 2 };

        /**
         * Apply the specified theme to the current ImGui context
         * @param theme The theme to apply
         */
        static void ApplyTheme(Theme theme);

        /**
         * Get the name of a theme for display in UI
         * @param theme The theme to get the name for
         * @return Human-readable theme name
         */
        static const char* GetThemeName(Theme theme);

        /**
         * Get the total number of available themes
         * @return Number of themes
         */
        static int GetThemeCount();

        /**
         * Convert integer index to theme enum
         * @param index Theme index (0-based)
         * @return Theme enum value
         */
        static Theme IndexToTheme(int index);

        /**
         * Convert theme enum to integer index
         * @param theme Theme enum value
         * @return Integer index (0-based)
         */
        static int ThemeToIndex(Theme theme);

    private:
        /**
         * Apply the dark theme (original ImGui dark style)
         */
        static void ApplyDarkTheme();

        /**
         * Apply the red theme (custom red color scheme)
         */
        static void ApplyRedTheme();

        /**
         * Apply the light theme (bright/light color scheme)
         */
        static void ApplyLightTheme();
    };

} // namespace Dynamix