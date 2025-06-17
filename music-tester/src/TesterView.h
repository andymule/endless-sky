#pragma once

#include "AudioSystem.h"
#include "imgui.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>

// Structure to hold a track and its properties
struct Track {
    std::string name;
    float volume = 1.0f;
    bool active = false;
    bool looping = false;
};

class TesterView {
public:
    TesterView();
    ~TesterView();

    bool Initialize(SDL_Window* window, SDL_GLContext glContext);
    void Render();
    void ProcessEvents(const SDL_Event& event);
    bool IsRunning() const { return m_isRunning; }
    void SetRunning(bool running) { m_isRunning = running; }

    // Directory management
    void SetMusicDirectory(const std::string& dir);
    void LoadMusicFromDirectory();

    void drawFilterControls(size_t trackIndex);

private:
    void cleanup();
    void RenderMainWindow();
    void RenderDirectoryInput();
    void RenderGlobalControls();
    void RenderTrackControls();
    void RenderBusControls();

    // UI State
    bool m_isRunning = true;
    std::string m_musicDir;
    std::vector<Track> m_tracks;
    bool m_isPlaying = false;
    char m_dirInput[256] = "";

    // Audio System
    AudioTester::AudioSystem m_audioSystem;

    // SDL/OpenGL
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
};