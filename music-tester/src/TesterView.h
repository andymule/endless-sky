#pragma once

#include "AudioSystem.h"
#include "imgui.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <filesystem>

// Structure to hold a track and its properties
struct Track
{
    std::string name;
    float volume = 1.0f;
    bool enabled = true;
};

class TesterView
{
  public:
    TesterView();
    ~TesterView();

    bool initialize(SDL_Window* window, SDL_GLContext glContext);
    void render();
    void processEvents(const SDL_Event& event);
    bool isRunning() const { return m_isRunning; }
    void setRunning(bool running) { m_isRunning = running; }

    // Directory management
    void setMusicDirectory(const std::string& dir);
    void loadMusicFromDirectory();

    void drawFilterControls(size_t trackIndex);

  private:
    void renderMainWindow();
    void renderDirectoryInput();
    void renderGlobalControls();
    void renderTrackControls();
    void renderBusControls();
    void renderEffectsControls(Track& track, size_t trackIndex);
    void renderMasterControls();
    void renderFilterControls();

    // UI State
    bool m_isRunning = true;
    std::string m_musicDir;
    std::vector<Track> m_tracks;
    bool m_isPlaying = false;
    float m_playbackPosition = 0.0f;
    char m_dirInput[256] = "";

    // Audio System
    AudioSystem m_audioSystem;

    // SDL/OpenGL
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    bool m_masterEnabled = false;  // Master toggle state
    float m_masterVolume = 1.0f;   // Master volume control
};