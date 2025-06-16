#include "AdaptiveMusic.h"
#include <iostream>
#include <filesystem>

// Include SoLoud headers
#include "soloud.h"
#include "soloud_wav.h"

// MusicTrack implementation
MusicTrack::MusicTrack(const std::string& filePath) : filePath(filePath), sound(nullptr), handle(0), playing(false), volume(1.0f) {}

MusicTrack::~MusicTrack() {
    if (sound) {
        delete sound;
        sound = nullptr;
    }
}

bool MusicTrack::Load() {
    if (sound) {
        delete sound;
        sound = nullptr;
    }
    
    sound = new SoLoud::Wav();
    if (!sound) {
        std::cerr << "Failed to create sound object" << std::endl;
        return false;
    }
    
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "File not found: " << filePath << std::endl;
        return false;
    }
    
    int result = sound->load(filePath.c_str());
    if (result != SoLoud::SO_NO_ERROR) {
        std::cerr << "Failed to load sound file: " << filePath << std::endl;
        return false;
    }
    
    return true;
}

void MusicTrack::Play(int loops) {
    if (!sound || !AdaptiveMusic().GetEngine()) return;
    
    sound->setLooping(loops < 0);
    handle = AdaptiveMusic().GetEngine()->play(*sound, volume);
    playing = true;
}

void MusicTrack::Stop() {
    if (playing && AdaptiveMusic().GetEngine() && AdaptiveMusic().GetEngine()->isValidVoiceHandle(handle)) {
        AdaptiveMusic().GetEngine()->stop(handle);
        playing = false;
    }
}

void MusicTrack::Pause() {
    if (playing && AdaptiveMusic().GetEngine() && AdaptiveMusic().GetEngine()->isValidVoiceHandle(handle)) {
        AdaptiveMusic().GetEngine()->setPause(handle, true);
    }
}

void MusicTrack::Resume() {
    if (AdaptiveMusic().GetEngine() && AdaptiveMusic().GetEngine()->isValidVoiceHandle(handle)) {
        AdaptiveMusic().GetEngine()->setPause(handle, false);
    }
}

void MusicTrack::SetVolume(float vol) {
    volume = vol;
    if (playing && AdaptiveMusic().GetEngine() && AdaptiveMusic().GetEngine()->isValidVoiceHandle(handle)) {
        AdaptiveMusic().GetEngine()->setVolume(handle, volume);
    }
}

bool MusicTrack::IsPlaying() const {
    return playing && AdaptiveMusic().GetEngine() && 
           AdaptiveMusic().GetEngine()->isValidVoiceHandle(handle) && 
           !AdaptiveMusic().GetEngine()->getPause(handle);
}

// MusicLayer implementation
MusicLayer::MusicLayer(const std::string& name, IntensityLevel level) 
    : name(name), level(level), currentTrack(0), volume(1.0f) {}

MusicLayer::~MusicLayer() {
    tracks.clear();
}

void MusicLayer::AddTrack(const std::string& filePath) {
    auto track = std::make_shared<MusicTrack>(filePath);
    if (track->Load()) {
        tracks.push_back(track);
    }
}

void MusicLayer::Play(TransitionType transition) {
    if (tracks.empty()) return;
    
    // For simplicity, we'll just play the first track
    // A more complex implementation would handle transitions between tracks
    if (currentTrack < tracks.size()) {
        switch (transition) {
            case TransitionType::IMMEDIATE:
                for (auto& track : tracks) {
                    track->Stop();
                }
                tracks[currentTrack]->Play(-1); // Loop
                break;
                
            case TransitionType::CROSSFADE:
                // In a real implementation, this would gradually fade out current tracks
                // and fade in the new one
                for (auto& track : tracks) {
                    track->Stop();
                }
                tracks[currentTrack]->Play(-1); // Loop
                break;
                
            case TransitionType::SEQUENTIAL:
                // Would wait for current track to finish before playing the next
                if (!tracks[currentTrack]->IsPlaying()) {
                    tracks[currentTrack]->Play(-1); // Loop
                }
                break;
        }
    }
}

void MusicLayer::Stop() {
    for (auto& track : tracks) {
        track->Stop();
    }
}

void MusicLayer::SetVolume(float vol) {
    volume = vol;
    for (auto& track : tracks) {
        track->SetVolume(volume);
    }
}

// AdaptiveMusic implementation
AdaptiveMusic::AdaptiveMusic() : soloudEngine(nullptr), initialized(false), playing(false) {
    intensityNames = {
        {IntensityLevel::AMBIENT, "Ambient"},
        {IntensityLevel::LOW, "Low"},
        {IntensityLevel::MEDIUM, "Medium"},
        {IntensityLevel::HIGH, "High"},
        {IntensityLevel::BOSS, "Boss"}
    };
}

AdaptiveMusic::~AdaptiveMusic() {
    Shutdown();
}

bool AdaptiveMusic::Initialize() {
    if (initialized) return true;
    
    soloudEngine = new SoLoud::Soloud();
    if (!soloudEngine) {
        std::cerr << "Failed to create SoLoud engine" << std::endl;
        return false;
    }
    
    int result = soloudEngine->init();
    if (result != SoLoud::SO_NO_ERROR) {
        std::cerr << "Failed to initialize SoLoud: " << result << std::endl;
        delete soloudEngine;
        soloudEngine = nullptr;
        return false;
    }
    
    initialized = true;
    return true;
}

void AdaptiveMusic::Shutdown() {
    if (!initialized) return;
    
    Stop();
    
    // Clear layers
    layers.clear();
    layersByName.clear();
    
    // Shutdown SoLoud
    if (soloudEngine) {
        soloudEngine->deinit();
        delete soloudEngine;
        soloudEngine = nullptr;
    }
    
    initialized = false;
}

void AdaptiveMusic::AddLayer(const std::string& name, IntensityLevel level) {
    auto layer = std::make_shared<MusicLayer>(name, level);
    layers.push_back(layer);
    layersByName[name] = layer;
}

bool AdaptiveMusic::AddTrackToLayer(const std::string& layerName, const std::string& filePath) {
    auto it = layersByName.find(layerName);
    if (it != layersByName.end()) {
        it->second->AddTrack(filePath);
        return true;
    }
    return false;
}

void AdaptiveMusic::SetIntensityLevel(IntensityLevel level, TransitionType transition) {
    if (level == currentIntensity) return;
    
    currentIntensity = level;
    
    // Stop all layers first
    for (auto& layer : layers) {
        layer->Stop();
    }
    
    // Play only the layers that match the current intensity
    for (auto& layer : layers) {
        if (layer->GetLevel() == currentIntensity) {
            layer->Play(transition);
        }
    }
}

void AdaptiveMusic::Update() {
    // Not needed for basic functionality with SoLoud
    // Would be used for more advanced features like cross-fading
}

void AdaptiveMusic::Play() {
    playing = true;
    
    // Play layers that match the current intensity
    for (auto& layer : layers) {
        if (layer->GetLevel() == currentIntensity) {
            layer->Play();
        }
    }
}

void AdaptiveMusic::Stop() {
    playing = false;
    
    for (auto& layer : layers) {
        layer->Stop();
    }
}

void AdaptiveMusic::Pause() {
    if (!initialized || !soloudEngine) return;
    
    soloudEngine->setPauseAll(true);
}

void AdaptiveMusic::Resume() {
    if (!initialized || !soloudEngine) return;
    
    soloudEngine->setPauseAll(false);
}

std::shared_ptr<MusicLayer> AdaptiveMusic::GetLayerByName(const std::string& name) {
    auto it = layersByName.find(name);
    if (it != layersByName.end()) {
        return it->second;
    }
    return nullptr;
} 