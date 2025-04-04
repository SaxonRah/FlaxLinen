// v WeatherCycle.cpp
#include "WeatherCycle.h"
#include "../LinenFlax.h"
#include "../TimeSystem.h"
#include "../WeatherSystem.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Engine/Time.h"
#include "Engine/Scripting/Plugins/PluginManager.h"

WeatherCycle::WeatherCycle(const SpawnParams& params)
    : Script(params)
{
    _tickUpdate = true;

    m_plugin = PluginManager::GetPlugin<LinenFlax>();
    m_weatherSystem = m_plugin ? m_plugin->GetSystem<WeatherSystem>() : nullptr;
    m_timeSystem = m_plugin ? m_plugin->GetSystem<TimeSystem>() : nullptr;
}

void WeatherCycle::OnEnable()
{
    LOG(Info, "WeatherCycle enabled");

    // Initialize current values
    m_currentSkyColor = ClearWeatherColor;
    m_targetSkyColor = m_currentSkyColor;

    if (m_plugin) {
        // Subscribe to weather change events
        m_plugin->GetEventSystem().Subscribe<WeatherChangedEvent>(
            [this](const WeatherChangedEvent& event) {
                WeatherCondition condition = m_weatherSystem->StringToWeatherCondition(event.newWeather);
                float intensity = event.intensity;

                LOG(Info, "Weather changed to: {0} (Intensity: {1:0.2f})",
                    String(event.newWeather.c_str()), intensity);

                // Update current weather tracking
                m_currentWeather = condition;
                m_currentIntensity = intensity;
                m_transitionProgress = 0.0f;

                // Update visuals with new weather
                UpdateWeatherVisuals(condition, intensity, m_transitionProgress);
            });

        // Check if weather system is available for initial setup
        if (m_weatherSystem) {
            // Set initial weather
            m_currentWeather = m_weatherSystem->GetCurrentWeather();
            m_currentIntensity = m_weatherSystem->GetWeatherIntensity();
            m_transitionProgress = m_weatherSystem->GetTransitionProgress();

            // Initialize visuals to current weather
            UpdateWeatherVisuals(m_currentWeather, m_currentIntensity, m_transitionProgress);

            LOG(Info, "Initial weather: {0} (Intensity: {1:0.2f})",
                String(GetWeatherName(m_currentWeather)), m_currentIntensity);

            // Apply weather lock if set
            if (LockWeather) {
                m_weatherSystem->ToggleWeatherLock(true);
            }
        } else {
            LOG(Warning, "WeatherSystem not available");
        }
    } else {
        LOG(Warning, "LinenFlax plugin not available");
    }
}

void WeatherCycle::OnDisable()
{
    LOG(Info, "WeatherCycle disabled");

    // Clean up any active effects
    if (m_rainEffect) {
        m_rainEffect->DeleteObject();
        m_rainEffect = nullptr;
    }
    if (m_snowEffect) {
        m_snowEffect->DeleteObject();
        m_snowEffect = nullptr;
    }
    if (m_fogEffect) {
        m_fogEffect->DeleteObject();
        m_fogEffect = nullptr;
    }
    if (m_windEffect) {
        m_windEffect->DeleteObject();
        m_windEffect = nullptr;
    }
}

void WeatherCycle::OnUpdate()
{
    if (!m_weatherSystem)
        return;

    // Check for debug weather control
    if (DebugWeatherType >= 0 && DebugWeatherType <= 9) {
        WeatherCondition debugCondition = static_cast<WeatherCondition>(DebugWeatherType);
        SetDebugWeather(DebugWeatherType, 1.0f);
    }

    // Update lock status if changed
    static bool prevLockStatus = LockWeather;
    if (prevLockStatus != LockWeather) {
        m_weatherSystem->ToggleWeatherLock(LockWeather);
        prevLockStatus = LockWeather;

        LOG(Info, "Weather lock {0}", String(LockWeather ? "enabled" : "disabled"));
    }

    // Get current weather state
    WeatherCondition currentCondition = m_weatherSystem->GetCurrentWeather();
    float currentIntensity = m_weatherSystem->GetWeatherIntensity();
    float transitionProgress = m_weatherSystem->GetTransitionProgress();

    // Update visuals if weather changed
    if (currentCondition != m_currentWeather || Math::Abs(currentIntensity - m_currentIntensity) > 0.05f || Math::Abs(transitionProgress - m_transitionProgress) > 0.05f) {

        m_currentWeather = currentCondition;
        m_currentIntensity = currentIntensity;
        m_transitionProgress = transitionProgress;

        UpdateWeatherVisuals(currentCondition, currentIntensity, transitionProgress);

        if (DebugLogging) {
            LOG(Info, "Weather update: {0} (Intensity: {1:0.2f}, Progress: {2:0.2f})",
                String(GetWeatherName(currentCondition)), currentIntensity, transitionProgress);
        }
    }

    // Apply smooth transitions for visual effects
    float lerpFactor = Time::GetDeltaTime() * 2.0f;

    if (SunLight) {
        Color newColor = Color::Lerp(m_currentSkyColor, m_targetSkyColor, lerpFactor);
        if (m_currentSkyColor != newColor) {
            m_currentSkyColor = newColor;
            SunLight->Color = m_currentSkyColor;
        }
    }

    // Smoothly update particle effects
    if (RainEmitter) {
        float newIntensity = Math::Lerp(m_currentRainIntensity, m_targetRainIntensity, lerpFactor);
        if (Math::Abs(newIntensity - m_currentRainIntensity) > 0.01f) {
            m_currentRainIntensity = newIntensity;

            // Check if we should enable/disable the effect
            bool shouldBeActive = m_currentRainIntensity > 0.05f;

            // Handle the ParticleSystem correctly
            if (shouldBeActive && !m_rainEffect) {
                // Spawn a new effect if needed
                // m_rainEffect = RainEmitter->Spawn(Actor::GetTransform());
                // TODO
            } else if (!shouldBeActive && m_rainEffect) {
                // Stop the effect when no longer needed
                m_rainEffect->DeleteObject();
                m_rainEffect = nullptr;
            }

            // If the effect is active, modify its properties based on intensity
            if (m_rainEffect) {
                // You might need to scale some property of the effect based on intensity
                // For example:
                // m_rainEffect->SetScale(Vector3(1.0f, 1.0f, 1.0f) * m_currentRainIntensity);
            }
        }
    }

    // Apply similar smooth transitions for snow, fog, wind emitters
    // Implementation would follow same pattern as rain emitter
}

void WeatherCycle::SetDebugWeather(int weatherType, float intensity)
{
    if (!m_weatherSystem)
        return;

    WeatherCondition condition = static_cast<WeatherCondition>(weatherType);

    // Force weather lock
    if (!LockWeather) {
        LockWeather = true;
        m_weatherSystem->ToggleWeatherLock(true);
    }

    // Force weather change
    m_weatherSystem->ForceWeatherChange(condition, intensity, 5.0f);

    LOG(Info, "Debug: Forced weather to {0} (Intensity: {1:0.2f})",
        String(GetWeatherName(condition)), intensity);
}

void WeatherCycle::UpdateWeatherVisuals(WeatherCondition condition, float intensity, float transitionProgress)
{
    // Update all visual effects based on weather condition and intensity
    UpdateSkyColor(condition, intensity);
    UpdateRainEffect(condition, intensity);
    UpdateSnowEffect(condition, intensity);
    UpdateFogEffect(condition, intensity);
    UpdateWindEffect(condition, intensity);
}

void WeatherCycle::UpdateSkyColor(WeatherCondition condition, float intensity)
{
    if (!SunLight)
        return;

    // Choose base color based on weather type
    Color baseColor;

    switch (condition) {
    case WeatherCondition::Clear:
        baseColor = ClearWeatherColor;
        break;
    case WeatherCondition::Cloudy:
    case WeatherCondition::Overcast:
        baseColor = CloudyWeatherColor;
        break;
    case WeatherCondition::Rain:
        baseColor = RainyWeatherColor;
        break;
    case WeatherCondition::Thunderstorm:
        baseColor = StormyWeatherColor;
        break;
    case WeatherCondition::Snow:
    case WeatherCondition::Blizzard:
        baseColor = SnowyWeatherColor;
        break;
    case WeatherCondition::Foggy:
        baseColor = FoggyWeatherColor;
        break;
    default:
        baseColor = ClearWeatherColor;
        break;
    }

    // Adjust brightness based on intensity
    float brightnessAdjust = 1.0f - (intensity * 0.3f);

    // Set target color
    m_targetSkyColor = baseColor * brightnessAdjust;

    // Apply time of day influences if we have a time system
    if (m_timeSystem) {
        TimeOfDay timeOfDay = m_timeSystem->GetTimeOfDay();

        // Darken colors at dawn/dusk
        if (timeOfDay == TimeOfDay::Dawn || timeOfDay == TimeOfDay::Dusk) {
            m_targetSkyColor.R *= 0.8f;
            m_targetSkyColor.G *= 0.7f;
            // Blue remains same or slightly increased
        }
        // Darken at night
        else if (timeOfDay == TimeOfDay::Night || timeOfDay == TimeOfDay::Midnight) {
            m_targetSkyColor = m_targetSkyColor * 0.4f;
        }
    }
}

void WeatherCycle::UpdateRainEffect(WeatherCondition condition, float intensity)
{
    if (!RainEmitter)
        return;

    // Set target rain intensity based on weather condition
    switch (condition) {
    case WeatherCondition::Rain:
        m_targetRainIntensity = intensity;
        break;
    case WeatherCondition::Thunderstorm:
        m_targetRainIntensity = intensity * 1.5f; // Heavier rain during storms
        break;
    default:
        m_targetRainIntensity = 0.0f; // No rain for other weather types
        break;
    }
}

void WeatherCycle::UpdateSnowEffect(WeatherCondition condition, float intensity)
{
    if (!SnowEmitter)
        return;

    // Set target snow intensity based on weather condition
    switch (condition) {
    case WeatherCondition::Snow:
        m_targetSnowIntensity = intensity;
        break;
    case WeatherCondition::Blizzard:
        m_targetSnowIntensity = intensity * 1.5f; // Heavier snow during blizzards
        break;
    default:
        m_targetSnowIntensity = 0.0f; // No snow for other weather types
        break;
    }
}

void WeatherCycle::UpdateFogEffect(WeatherCondition condition, float intensity)
{
    if (!FogEmitter)
        return;

    // Set target fog density based on weather condition
    switch (condition) {
    case WeatherCondition::Foggy:
        m_targetFogDensity = intensity;
        break;
    case WeatherCondition::Rain:
        m_targetFogDensity = intensity * 0.3f; // Light fog during rain
        break;
    case WeatherCondition::Snow:
        m_targetFogDensity = intensity * 0.4f; // Moderate fog during snow
        break;
    case WeatherCondition::Blizzard:
        m_targetFogDensity = intensity * 0.7f; // Heavy fog during blizzards
        break;
    default:
        m_targetFogDensity = 0.0f; // No fog for other weather types
        break;
    }

    // Apply time of day influences
    if (m_timeSystem) {
        TimeOfDay timeOfDay = m_timeSystem->GetTimeOfDay();

        // Increase fog at dawn
        if (timeOfDay == TimeOfDay::Dawn) {
            m_targetFogDensity = Math::Max(m_targetFogDensity, 0.3f);
        }
        // Light fog at dusk
        else if (timeOfDay == TimeOfDay::Dusk) {
            m_targetFogDensity = Math::Max(m_targetFogDensity, 0.2f);
        }
    }
}

void WeatherCycle::UpdateWindEffect(WeatherCondition condition, float intensity)
{
    if (!WindEmitter)
        return;

    // Set target wind intensity based on weather condition
    switch (condition) {
    case WeatherCondition::Windy:
        m_targetWindIntensity = intensity;
        break;
    case WeatherCondition::Thunderstorm:
        m_targetWindIntensity = intensity * 0.8f; // Strong wind during storms
        break;
    case WeatherCondition::Blizzard:
        m_targetWindIntensity = intensity * 0.9f; // Very strong wind during blizzards
        break;
    default:
        m_targetWindIntensity = intensity * 0.1f; // Light wind for other weather types
        break;
    }
}

const char* WeatherCycle::GetWeatherName(WeatherCondition condition) const
{
    switch (condition) {
    case WeatherCondition::Clear:
        return "Clear";
    case WeatherCondition::Cloudy:
        return "Cloudy";
    case WeatherCondition::Overcast:
        return "Overcast";
    case WeatherCondition::Foggy:
        return "Foggy";
    case WeatherCondition::Rain:
        return "Rain";
    case WeatherCondition::Thunderstorm:
        return "Thunderstorm";
    case WeatherCondition::Snow:
        return "Snow";
    case WeatherCondition::Blizzard:
        return "Blizzard";
    case WeatherCondition::Heatwave:
        return "Heatwave";
    case WeatherCondition::Windy:
        return "Windy";
    default:
        return "Unknown";
    }
}
// ^ WeatherCycle.cpp