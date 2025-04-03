// v WeatherSystem.h
#pragma once

#include "RPGSystem.h"
#include "EventSystem.h"
#include "Serialization.h"
#include "TimeSystem.h"
#include <string>
#include <vector>
#include <random>
#include <unordered_map>

// Weather-related events
class WeatherChangedEvent : public EventType<WeatherChangedEvent> {
public:
    std::string previousWeather;
    std::string newWeather;
    float intensity;
    bool isDangerous;
};

// Define weather condition types
enum class WeatherCondition {
    Clear,
    Cloudy,
    Overcast,
    Foggy,
    Rain,
    Thunderstorm,
    Snow,
    Blizzard,
    Heatwave,
    Windy
};

// Weather state class to track current conditions
class WeatherState {
public:
    WeatherState() : 
        m_condition(WeatherCondition::Clear), 
        m_intensity(0.0f),
        m_transitionProgress(0.0f),
        m_duration(0.0f),
        m_remainingTime(0.0f),
        m_isDangerous(false) {}

    WeatherCondition GetCondition() const { return m_condition; }
    float GetIntensity() const { return m_intensity; }
    float GetTransitionProgress() const { return m_transitionProgress; }
    float GetDuration() const { return m_duration; }
    float GetRemainingTime() const { return m_remainingTime; }
    bool IsDangerous() const { return m_isDangerous; }

    void SetCondition(WeatherCondition condition) { m_condition = condition; }
    void SetIntensity(float intensity) { m_intensity = intensity; }
    void SetTransitionProgress(float progress) { m_transitionProgress = progress; }
    void SetDuration(float duration) { m_duration = duration; }
    void SetRemainingTime(float time) { m_remainingTime = time; }
    void SetIsDangerous(bool dangerous) { m_isDangerous = dangerous; }

    void Serialize(BinaryWriter& writer) const;
    void Deserialize(BinaryReader& reader);
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);

private:
    WeatherCondition m_condition;
    float m_intensity;
    float m_transitionProgress;
    float m_duration;
    float m_remainingTime;
    bool m_isDangerous;
};

class WeatherSystem : public RPGSystem {
public:
    // Delete copy constructor and assignment operator
    WeatherSystem(const WeatherSystem&) = delete;
    WeatherSystem& operator=(const WeatherSystem&) = delete;
    
    // Meyer's Singleton - thread-safe in C++11 and beyond
    static WeatherSystem* GetInstance() {
        static WeatherSystem* instance = new WeatherSystem();
        return instance;
    }
    
    // Cleanup method
    static void Destroy() {
        static WeatherSystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }
    
    ~WeatherSystem();

    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetName() const override { return "WeatherSystem"; }
    
    // Serialization
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);
    
    // Weather control methods
    void ForceWeatherChange(WeatherCondition condition, float intensity, float transitionDuration = 10.0f);
    void SetWeatherDuration(float hours);
    void SetWeatherTransitionSpeed(float speed);
    
    // Weather information
    WeatherCondition GetCurrentWeather() const;
    float GetWeatherIntensity() const;
    bool IsWeatherDangerous() const;
    std::string GetWeatherName() const;
    
    // Weather transitions
    float GetTransitionProgress() const;
    
    // Return weather likelihoods for season and time of day
    const std::unordered_map<WeatherCondition, float>& GetWeatherProbabilities() const;
    
    // Debug control
    void ToggleWeatherLock(bool locked);
    bool IsWeatherLocked() const;

    // Convert enum to string
    std::string WeatherConditionToString(WeatherCondition condition) const;
    WeatherCondition StringToWeatherCondition(const std::string& str) const;
    
private:
    // Private constructor
    WeatherSystem();
    
    // Weather state
    WeatherState m_currentWeather;
    WeatherState m_targetWeather;
    
    // System configuration
    float m_weatherUpdateFrequency = 4.0f; // Hours between weather checks
    float m_minWeatherDuration = 2.0f;     // Minimum hours a weather condition lasts
    float m_maxWeatherDuration = 12.0f;    // Maximum hours a weather condition lasts
    float m_transitionSpeed = 1.0f;        // Base transition speed
    
    // Tracking
    float m_timeSinceLastCheck = 0.0f;
    bool m_isTransitioning = false;
    bool m_weatherLocked = false;
    
    // Random generator
    std::mt19937 m_randomGenerator;
    
    // Weather probabilities by season and time
    std::unordered_map<std::string, std::unordered_map<WeatherCondition, float>> m_seasonWeatherProbabilities;
    
    // Helper methods
    void UpdateWeatherProbabilities(const std::string& season, TimeOfDay timeOfDay);
    void PickNewWeather();
    void UpdateWeatherTransition(float deltaTime);
    bool ShouldTriggerWeatherChange();
    
};
// ^ WeatherSystem.h