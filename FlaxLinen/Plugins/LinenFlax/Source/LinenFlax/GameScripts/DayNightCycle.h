// v DayNightCycle.h
#pragma once

#include "Engine/Scripting/Script.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Level/Actors/DirectionalLight.h"

class LinenFlax;
class TimeSystem;

API_CLASS() class LINENFLAX_API DayNightCycle : public Script
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE(DayNightCycle);

public:
    // Reference to the directional light used as the sun
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"Light Settings\"), Tooltip(\"The directional light that will be used as the sun\")")
    DirectionalLight* SunLight = nullptr;
    
    // Reference to the directional light used as the moon
    API_FIELD(Attributes="EditorOrder(1), EditorDisplay(\"Light Settings\"), Tooltip(\"The directional light that will be used as the moon\")")
    DirectionalLight* MoonLight = nullptr;

    // Reference to the directional light used as East Ambient Light
    API_FIELD(Attributes="EditorOrder(2), EditorDisplay(\"Light Settings\"), Tooltip(\"The directional light that will be used as the moon\")")
    DirectionalLight* EastAmbientLight = nullptr;
    
    // Reference to the directional light used as West Ambient Light
    API_FIELD(Attributes="EditorOrder(3), EditorDisplay(\"Light Settings\"), Tooltip(\"The directional light that will be used as the moon\")")
    DirectionalLight* WestAmbientLight = nullptr;

    // Color for daytime
    API_FIELD(Attributes="EditorOrder(4), EditorDisplay(\"Light Settings\"), Tooltip(\"Color of the sun light during daytime\")")
    Color DaytimeColor = Color(1.0f, 0.9f, 0.7f, 1.0f);
    
    // Color for nighttime
    API_FIELD(Attributes="EditorOrder(5), EditorDisplay(\"Light Settings\"), Tooltip(\"Color of the moon light during nighttime\")")
    Color NighttimeColor = Color(0.1f, 0.1f, 0.3f, 1.0f);
    
    // Intensity for daytime
    API_FIELD(Attributes="EditorOrder(6), EditorDisplay(\"Light Settings\"), Tooltip(\"Intensity of the sun light during daytime\")")
    float DaytimeIntensity = 10.0f;
    
    // Intensity for nighttime
    API_FIELD(Attributes="EditorOrder(7), EditorDisplay(\"Light Settings\"), Tooltip(\"Intensity of the moon light during nighttime\")")
    float NighttimeIntensity = 0.5f;

    // Control the time scale via the editor
    API_FIELD(Attributes="EditorOrder(8), EditorDisplay(\"Time Settings\"), Tooltip(\"How fast time passes. Higher values = faster day/night cycle\")")
    float TimeScale = 60.0f; // 1 game minute passes every real second by default
    
    API_FIELD(Attributes="EditorOrder(9), EditorDisplay(\"Animation\"), Tooltip(\"How fast the lights transition between positions. Higher values = faster transitions\")")
    float TransitionSpeed = 1.0f;

    // Editor debug controls
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Debug\"), Tooltip(\"Set a specific hour for testing\")")
    int32 DebugHour = -1;
    
    // Whether to use the debug hour
    API_FIELD(Attributes="EditorOrder(11), EditorDisplay(\"Debug\"), Tooltip(\"When enabled, uses the debug hour instead of real-time progression\")")
    bool UseDebugHour = false;

    API_FIELD(Attributes="EditorOrder(12), EditorDisplay(\"Debug\"), Tooltip(\"Force advance time by this many seconds each frame\")")
    float DebugForceTimeAdvanceSeconds = 0.0f;
    
    API_FIELD(Attributes="EditorOrder(13), EditorDisplay(\"Debug\"), Tooltip(\"Override the day progress value (0.0-1.0)\")")
    float DebugOverrideDayProgress = -1.0f;

    API_FIELD(Attributes="EditorOrder(14), EditorDisplay(\"Debug\"), Tooltip(\"Log debug information\")")
    bool DebugLogging = false;


    API_FIELD(Attributes="EditorOrder(15), EditorDisplay(\"Light Settings\"), Tooltip(\"Color of the moon light during nighttime\")")
    float AmbientIntensity = 0.5f;

    API_FIELD(Attributes="EditorOrder(16), EditorDisplay(\"Light Settings\"), Tooltip(\"Color of the moon light during nighttime\")")
    Color TransitionColor = Color(0.9f, 0.7f, 0.5f, 1.0f); // Warm orange/yellow for transitions

private:
    // State tracking
    int m_prevHour = -1;
    int m_currentHour = 12;
    float m_currentDayProgress = 0.5f;
    bool m_prevUseDebugHour = false;
    int m_prevDebugHour = 12;
    
    // Event handling flags
    bool m_receivedHourEvent = false;
    int m_lastEventHour = -1;
    bool m_lastEventIsDaytime = true;
    
    // Sun transition state
    Quaternion m_sunTargetRotation;
    Quaternion m_sunCurrentRotation;
    Color m_sunTargetColor;
    Color m_sunCurrentColor;
    float m_sunTargetBrightness;
    float m_sunCurrentBrightness;
    
    // Moon transition state
    Quaternion m_moonTargetRotation;
    Quaternion m_moonCurrentRotation;
    Color m_moonTargetColor;
    Color m_moonCurrentColor;
    float m_moonTargetBrightness;
    float m_moonCurrentBrightness;

protected:
    LinenFlax* m_plugin = nullptr;
    TimeSystem* m_timeSystem = nullptr;

public:
    // Called when script is being initialized
    void OnEnable() override;
    
    // Called when script is being disabled
    void OnDisable() override;
    
    // Called when script is updated (once per frame)
    void OnUpdate() override;
    
    // Updates the sun's rotation and color based on time of day
    void UpdateSun(float dayProgress);

    // Updates the moon's rotation and color based on time of day
    void UpdateMoon(float dayProgress);

    void FindAndAssignLights();
};
// ^ DayNightCycle.h