#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

// Forward declarations for SoLoud
namespace SoLoud {
    class Soloud;
    class Wav;
}

// Enum for different music intensity levels
enum class IntensityLevel {
    AMBIENT,
    LOW,
    MEDIUM,
    HIGH,
    BOSS
};

// Enum for different transitions between music segments
enum class TransitionType {
    IMMEDIATE,
    CROSSFADE,
    SEQUENTIAL
};

// Class representing a single music track
class MusicTrack {
public:
    MusicTrack(const std::string& filePath);
    ~MusicTrack();
    
    bool Load();
    void Play(int loops = -1);
    void Stop();
    void Pause();
    void Resume();
    void SetVolume(float volume); // 0.0 to 1.0
    
    bool IsPlaying() const;
    std::string GetFilePath() const { return filePath; }
    SoLoud::Wav* GetSound() const { return sound; }
    unsigned int GetHandle() const { return handle; }

private:
    std::string filePath;
    SoLoud::Wav* sound = nullptr;
    unsigned int handle = 0;
    bool playing = false;
    float volume = 1.0f;
};

// Class representing a layer in adaptive music
class MusicLayer {
public:
    MusicLayer(const std::string& name, IntensityLevel level);
    ~MusicLayer();
    
    void AddTrack(const std::string& filePath);
    void Play(TransitionType transition = TransitionType::CROSSFADE);
    void Stop();
    void SetVolume(float volume); // 0.0 to 1.0
    
    std::string GetName() const { return name; }
    IntensityLevel GetLevel() const { return level; }
    std::vector<std::shared_ptr<MusicTrack>> GetTracks() const { return tracks; }

private:
    std::string name;
    IntensityLevel level;
    std::vector<std::shared_ptr<MusicTrack>> tracks;
    int currentTrack = 0;
    float volume = 1.0f;
};

// Main adaptive music system class
class AdaptiveMusic {
public:
    AdaptiveMusic();
    ~AdaptiveMusic();
    
    bool Initialize();
    void Shutdown();
    
    // Add a new layer
    void AddLayer(const std::string& name, IntensityLevel level);
    
    // Add a track to a specific layer
    bool AddTrackToLayer(const std::string& layerName, const std::string& filePath);
    
    // Change the intensity level
    void SetIntensityLevel(IntensityLevel level, TransitionType transition = TransitionType::CROSSFADE);
    
    // Update the music system (call this regularly)
    void Update();
    
    // Basic controls
    void Play();
    void Stop();
    void Pause();
    void Resume();
    
    // Get current state
    IntensityLevel GetCurrentIntensity() const { return currentIntensity; }
    bool IsPlaying() const { return playing; }
    bool IsInitialized() const { return initialized; }
    
    // Getters for the UI
    std::vector<std::shared_ptr<MusicLayer>> GetLayers() const { return layers; }
    std::shared_ptr<MusicLayer> GetLayerByName(const std::string& name);
    std::map<IntensityLevel, std::string> GetIntensityNames() const { return intensityNames; }
    
    // SoLoud instance accessor
    SoLoud::Soloud* GetEngine() const { return soloudEngine; }

private:
    SoLoud::Soloud* soloudEngine = nullptr;
    std::vector<std::shared_ptr<MusicLayer>> layers;
    std::map<std::string, std::shared_ptr<MusicLayer>> layersByName;
    IntensityLevel currentIntensity = IntensityLevel::AMBIENT;
    bool initialized = false;
    bool playing = false;
    
    // Map intensity levels to human-readable names
    std::map<IntensityLevel, std::string> intensityNames = {
        {IntensityLevel::AMBIENT, "Ambient"},
        {IntensityLevel::LOW, "Low"},
        {IntensityLevel::MEDIUM, "Medium"},
        {IntensityLevel::HIGH, "High"},
        {IntensityLevel::BOSS, "Boss"}
    };
}; 