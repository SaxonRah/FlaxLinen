// v WeatherCycle.h
#pragma once

#include "../WeatherSystem.h"

#include "Engine/Scripting/Script.h"
#include "Engine/Level/Actors/PointLight.h"
#include "Engine/Level/Actors/DirectionalLight.h"
#include "Engine/Particles/ParticleEffect.h"
#include "Engine/Particles/ParticleEmitter.h"
#include "Engine/Particles/ParticleSystem.h"

class LinenFlax;
class WeatherSystem;
class TimeSystem;

API_CLASS() class LINENFLAX_API WeatherCycle : public Script
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE(WeatherCycle);

public:
    // Weather visual settings for each type of weather
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"Weather Effects\"), Tooltip(\"Light that will be influenced by weather\")")
    DirectionalLight* SunLight = nullptr;

    API_FIELD(Attributes="EditorOrder(1), EditorDisplay(\"Weather Effects\"), Tooltip(\"Emitter for rain particles\")")
    ParticleEmitter* RainEmitter = nullptr;

    API_FIELD(Attributes="EditorOrder(2), EditorDisplay(\"Weather Effects\"), Tooltip(\"Emitter for snow particles\")")
    ParticleEmitter* SnowEmitter = nullptr;

    API_FIELD(Attributes="EditorOrder(3), EditorDisplay(\"Weather Effects\"), Tooltip(\"Emitter for fog particles\")")
    ParticleEmitter* FogEmitter = nullptr;

    API_FIELD(Attributes="EditorOrder(4), EditorDisplay(\"Weather Effects\"), Tooltip(\"Emitter for dust/wind particles\")")
    ParticleEmitter* WindEmitter = nullptr;

    API_FIELD(Attributes="EditorOrder(5), EditorDisplay(\"Visual Settings\"), Tooltip(\"Color for clear weather\")")
    Color ClearWeatherColor = Color(1.0f, 1.0f, 1.0f, 1.0f);

    API_FIELD(Attributes="EditorOrder(6), EditorDisplay(\"Visual Settings\"), Tooltip(\"Color for cloudy weather\")")
    Color CloudyWeatherColor = Color(0.8f, 0.8f, 0.85f, 1.0f);

    API_FIELD(Attributes="EditorOrder(7), EditorDisplay(\"Visual Settings\"), Tooltip(\"Color for rainy weather\")")
    Color RainyWeatherColor = Color(0.6f, 0.6f, 0.7f, 1.0f);

    API_FIELD(Attributes="EditorOrder(8), EditorDisplay(\"Visual Settings\"), Tooltip(\"Color for stormy weather\")")
    Color StormyWeatherColor = Color(0.4f, 0.4f, 0.5f, 1.0f);

    API_FIELD(Attributes="EditorOrder(9), EditorDisplay(\"Visual Settings\"), Tooltip(\"Color for snowy weather\")")
    Color SnowyWeatherColor = Color(0.9f, 0.9f, 1.0f, 1.0f);

    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Visual Settings\"), Tooltip(\"Color for foggy weather\")")
    Color FoggyWeatherColor = Color(0.7f, 0.7f, 0.7f, 1.0f);

    // Debug controls
    API_FIELD(Attributes="EditorOrder(11), EditorDisplay(\"Debug\"), Tooltip(\"Force a specific weather type\")")
    int32 DebugWeatherType = -1;

    API_FIELD(Attributes="EditorOrder(12), EditorDisplay(\"Debug\"), Tooltip(\"Log debug information\")")
    bool DebugLogging = false;

    API_FIELD(Attributes="EditorOrder(13), EditorDisplay(\"Debug\"), Tooltip(\"Lock weather to prevent automatic changes\")")
    bool LockWeather = false;
private:

    ParticleEffect* m_rainEffect = nullptr;
    ParticleEffect* m_snowEffect = nullptr;
    ParticleEffect* m_fogEffect = nullptr;
    ParticleEffect* m_windEffect = nullptr;

protected:
    // System references
    LinenFlax* m_plugin = nullptr;
    WeatherSystem* m_weatherSystem = nullptr;
    TimeSystem* m_timeSystem = nullptr;

    // State tracking
    WeatherCondition m_currentWeather = WeatherCondition::Clear;
    float m_currentIntensity = 0.0f;
    float m_transitionProgress = 1.0f;

    // Target values for interpolation
    Color m_targetSkyColor;
    float m_targetFogDensity = 0.0f;
    float m_targetRainIntensity = 0.0f;
    float m_targetSnowIntensity = 0.0f;
    float m_targetWindIntensity = 0.0f;

    // Current values for smooth transitions
    Color m_currentSkyColor;
    float m_currentFogDensity = 0.0f;
    float m_currentRainIntensity = 0.0f;
    float m_currentSnowIntensity = 0.0f;
    float m_currentWindIntensity = 0.0f;

public:
    // Called when script is being initialized
    void OnEnable() override;
    
    // Called when script is being disabled
    void OnDisable() override;
    
    // Called when script is updated (once per frame)
    void OnUpdate() override;
    
    // Debug weather control
    void SetDebugWeather(int weatherType, float intensity);
    
    // Update weather visuals
    void UpdateWeatherVisuals(WeatherCondition condition, float intensity, float transitionProgress);
    
    // Update individual effects
    void UpdateSkyColor(WeatherCondition condition, float intensity);
    void UpdateRainEffect(WeatherCondition condition, float intensity);
    void UpdateSnowEffect(WeatherCondition condition, float intensity);
    void UpdateFogEffect(WeatherCondition condition, float intensity);
    void UpdateWindEffect(WeatherCondition condition, float intensity);
    
    // Helper for weather type conversion
    const char* GetWeatherName(WeatherCondition condition) const;
};
// ^ WeatherCycle.h