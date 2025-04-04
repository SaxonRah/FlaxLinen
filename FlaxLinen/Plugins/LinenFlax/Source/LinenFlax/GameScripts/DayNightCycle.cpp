// DayNightCycle.cpp
#include "DayNightCycle.h"
#include "../LinenFlax.h"
#include "../TimeSystem.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Engine/Time.h"
#include "Engine/Level/Actors/DirectionalLight.h"
#include "Engine/Level/Actors/Light.h"
#include "Engine/Level/Level.h"
#include "Engine/Scripting/Plugins/PluginManager.h"


DayNightCycle::DayNightCycle(const SpawnParams& params)
    : Script(params)
{
    // Enable ticking by default
    _tickUpdate = true;

    m_plugin = PluginManager::GetPlugin<LinenFlax>();
    m_timeSystem = m_plugin ? m_plugin->GetSystem<TimeSystem>() : nullptr;
}

void DayNightCycle::OnEnable()
{
    LOG(Info, "DayNightCycle script enabled");

    FindAndAssignLights();

    // Initialize sun state
    m_sunCurrentRotation = Quaternion::Identity;
    m_sunTargetRotation = Quaternion::Identity;
    m_sunCurrentColor = Color(1, 1, 1, 1);
    m_sunTargetColor = m_sunCurrentColor;
    m_sunCurrentBrightness = 1.0f;
    m_sunTargetBrightness = m_sunCurrentBrightness;

    // Initialize moon state
    if (MoonLight != nullptr) {
        m_moonCurrentRotation = Quaternion::Identity;
        m_moonTargetRotation = Quaternion::Identity;
        m_moonCurrentColor = NighttimeColor;
        m_moonTargetColor = m_moonCurrentColor;
        m_moonCurrentBrightness = 0.0f; // Start with moon off
        m_moonTargetBrightness = m_moonCurrentBrightness;
    }

    // Rest of initialization code...
    m_prevHour = -1;

    if (m_plugin) {
        // Subscribe to hour changed events which is what we mainly need
        m_plugin->GetEventSystem().Subscribe<HourChangedEvent>(
            [this](const HourChangedEvent& event) {
                // Store the event information we need
                m_lastEventHour = event.newHour;
                m_lastEventIsDaytime = event.isDayTime;
                m_receivedHourEvent = true;

                LOG(Info, "Hour changed event received: {0} -> {1}, isDayTime: {2}",
                    event.previousHour, event.newHour, String(event.isDayTime ? "Yes" : "No"));
            });

        // Initial setup with TimeSystem
        if (m_timeSystem) {
            // Set time scale from our property
            m_timeSystem->SetTimeScale(TimeScale);
            LOG(Info, "Time scale set to {0}", TimeScale);

            // Set debug hour if needed
            if (UseDebugHour && DebugHour >= 0 && DebugHour < 24) {
                m_timeSystem->DebugSetTime(DebugHour, 0);
                LOG(Info, "Debug time set to {0}:00", DebugHour);
            }

            // Get initial time values
            m_currentHour = m_timeSystem->GetHour();
            m_currentDayProgress = m_timeSystem->GetDayProgress();

            // Update the sun and moon positions initially
            UpdateSun(m_currentDayProgress);
            UpdateMoon(m_currentDayProgress);
        } else {
            LOG(Warning, "TimeSystem not available. Day/night cycle won't function properly.");
        }
    } else {
        LOG(Warning, "LinenFlax plugin not available. Day/night cycle won't function properly.");
    }
}

void DayNightCycle::OnDisable()
{
    LOG(Info, "DayNightCycle script disabled");
    // Note: We're relying on Script destruction to clean up event subscriptions
}

void DayNightCycle::OnUpdate()
{
    if (SunLight == nullptr)
        return;

    // Check for debug mode changes
    bool settingsChanged = (UseDebugHour != m_prevUseDebugHour || (UseDebugHour && DebugHour != m_prevDebugHour));

    m_prevUseDebugHour = UseDebugHour;
    m_prevDebugHour = DebugHour;

    // Handle any received events
    if (m_receivedHourEvent) {
        m_receivedHourEvent = false;
        m_currentHour = m_lastEventHour;

        // We still need to get the day progress from TimeSystem
        if (m_timeSystem) {
            m_currentDayProgress = m_timeSystem->GetDayProgress();

            UpdateSun(m_currentDayProgress);
            UpdateMoon(m_currentDayProgress);
        }
    }

    // Apply settings/debug changes if needed
    if (m_timeSystem) {
        // Update time scale if needed
        if (m_timeSystem->GetTimeScale() != TimeScale) {
            m_timeSystem->SetTimeScale(TimeScale);
        }

        // Handle debug hour setting
        if (settingsChanged && UseDebugHour && DebugHour >= 0 && DebugHour < 24) {
            m_timeSystem->DebugSetTime(DebugHour, 0);
            LOG(Info, "Debug time set to {0}:00", DebugHour);

            // Update current values immediately
            m_currentHour = m_timeSystem->GetHour();
            m_currentDayProgress = m_timeSystem->GetDayProgress();

            UpdateSun(m_currentDayProgress);
            UpdateMoon(m_currentDayProgress);
        }

        // Force advance time if requested
        if (DebugForceTimeAdvanceSeconds > 0.0f) {
            m_timeSystem->AdvanceTimeSeconds(static_cast<int>(DebugForceTimeAdvanceSeconds));

            if (DebugLogging) {
                LOG(Info, "Forced time advance by {0} seconds", DebugForceTimeAdvanceSeconds);
            }

            // Update current values immediately after advancing
            m_currentHour = m_timeSystem->GetHour();
            m_currentDayProgress = m_timeSystem->GetDayProgress();

            UpdateSun(m_currentDayProgress);
            UpdateMoon(m_currentDayProgress);
        }

        // Check for hour changes even if no event was received
        // This is a fallback in case events aren't working properly
        int currentHour = m_timeSystem->GetHour();
        if (currentHour != m_prevHour) {
            m_prevHour = currentHour;
            m_currentHour = currentHour;
            m_currentDayProgress = m_timeSystem->GetDayProgress();

            LOG(Info, "Hour change detected (poll): {0}, Day progress: {1:0.3f}, Is daytime: {2}",
                String(m_timeSystem->GetFormattedTime().c_str()),
                m_currentDayProgress,
                String(m_timeSystem->IsDaytime() ? "Yes" : "No"));

            UpdateSun(m_currentDayProgress);
            UpdateMoon(m_currentDayProgress);
        }

        // Debug override of day progress
        if (DebugOverrideDayProgress >= 0.0f && DebugOverrideDayProgress <= 1.0f) {
            m_currentDayProgress = DebugOverrideDayProgress;

            if (DebugLogging) {
                LOG(Info, "Using override day progress: {0:0.3f}", m_currentDayProgress);
            }

            UpdateSun(m_currentDayProgress);
            UpdateMoon(m_currentDayProgress);
        } else if (DebugLogging) {
            // Only update logging if requested
            LOG(Info, "Time: {0}, Day progress: {1:0.3f}",
                String(m_timeSystem->GetFormattedTime().c_str()),
                m_currentDayProgress);
        }
    }
}
/*
void DayNightCycle::UpdateSun(float dayProgress)
{
    if (SunLight == nullptr)
        return;
    
    // Calculate sun angle for a full 360-degree rotation
    float sunAngle = dayProgress * 360.0f;
    
    // Apply rotation
    m_sunTargetRotation = Quaternion::Euler(sunAngle, 180.0f, 0.0f);
    
    // Calculate visibility based on whether sun is above horizon
    bool isSunVisible = (sunAngle < 180.0f);
    
    // Calculate sun brightness
    if (isSunVisible) {
        // Sun is above horizon
        // Brightest at 90 degrees (zenith)
        float brightnessMultiplier = 1.0f - Math::Abs(sunAngle - 90.0f) / 90.0f;
        brightnessMultiplier = Math::Max(brightnessMultiplier, 0.2f); // Keep minimum brightness when visible
        m_sunTargetBrightness = DaytimeIntensity * brightnessMultiplier;
        m_sunTargetColor = DaytimeColor;
    } else {
        // Sun is below horizon
        m_sunTargetBrightness = 0.0f;
    }
    
    // Smoothly interpolate values
    float transitionFactor = Math::Min(Time::GetDeltaTime() * TransitionSpeed, 1.0f);
    
    if (m_sunCurrentRotation == Quaternion::Identity) {
        m_sunCurrentRotation = m_sunTargetRotation;
        m_sunCurrentColor = m_sunTargetColor;
        m_sunCurrentBrightness = m_sunTargetBrightness;
    } else {
        Quaternion::Slerp(m_sunCurrentRotation, m_sunTargetRotation, transitionFactor, m_sunCurrentRotation);
        m_sunCurrentColor = Color::Lerp(m_sunCurrentColor, m_sunTargetColor, transitionFactor);
        m_sunCurrentBrightness = Math::Lerp(m_sunCurrentBrightness, m_sunTargetBrightness, transitionFactor);
    }
    
    // Apply values to sun light
    SunLight->SetLocalOrientation(m_sunCurrentRotation);
    SunLight->Color = m_sunCurrentColor;
    SunLight->Brightness = m_sunCurrentBrightness;
    
    if (DebugLogging) {
        LOG(Info, "Sun - Day progress: {0:0.3f}, Angle: {1:0.1f}, Brightness: {2:0.2f}", 
            dayProgress, sunAngle, m_sunCurrentBrightness);
    }
}

void DayNightCycle::UpdateMoon(float dayProgress)
{
    if (MoonLight == nullptr)
        return;
    
    float moonOffset = 5.0f;
    // Moon is always 180 degrees offset from the sun
    float moonAngle = moonOffset + fmod(dayProgress * 360.0f + 180.0f, 360.0f);
    
    // Apply rotation
    m_moonTargetRotation = Quaternion::Euler(moonAngle, 180.0f, 0.0f);
    
    bool isMoonVisible = (moonAngle < 180.0f);
    
    // Calculate moon offset
    if (isMoonVisible) { moonOffset = -moonOffset; }

    // Moon brightness
    m_moonTargetBrightness = NighttimeIntensity;
    m_moonTargetColor = NighttimeColor;
    
    // Smoothly interpolate values
    float transitionFactor = Math::Min(Time::GetDeltaTime() * TransitionSpeed, 1.0f);
    
    if (m_moonCurrentRotation == Quaternion::Identity) {
        m_moonCurrentRotation = m_moonTargetRotation;
        m_moonCurrentColor = m_moonTargetColor;
        m_moonCurrentBrightness = m_moonTargetBrightness;
    } else {
        Quaternion::Slerp(m_moonCurrentRotation, m_moonTargetRotation, transitionFactor, m_moonCurrentRotation);
        m_moonCurrentColor = Color::Lerp(m_moonCurrentColor, m_moonTargetColor, transitionFactor);
        m_moonCurrentBrightness = Math::Lerp(m_moonCurrentBrightness, m_moonTargetBrightness, transitionFactor);
    }
    
    // Apply values to moon light
    MoonLight->SetLocalOrientation(m_moonCurrentRotation);
    MoonLight->Color = m_moonCurrentColor;
    MoonLight->Brightness = m_moonCurrentBrightness;
    
    if (DebugLogging) {
        LOG(Info, "Moon - Day progress: {0:0.3f}, Angle: {1:0.1f}, Brightness: {2:0.2f}", 
            dayProgress, moonAngle, m_moonCurrentBrightness);
    }
}
*/
void DayNightCycle::UpdateSun(float dayProgress)
{
    if (SunLight == nullptr)
        return;

    // Calculate sun angle for a full 360-degree rotation
    float sunAngle = dayProgress * 360.0f;

    // Apply rotation
    m_sunTargetRotation = Quaternion::Euler(sunAngle, 180.0f, 0.0f);

    // Calculate visibility based on whether sun is above horizon
    bool isSunVisible = (sunAngle < 180.0f);

    // Set brightness based on visibility
    if (isSunVisible) {
        m_sunTargetBrightness = DaytimeIntensity;
        m_sunTargetColor = DaytimeColor;
    } else {
        m_sunTargetBrightness = 0.0f;
    }

    // Apply values to sun light
    SunLight->SetLocalOrientation(m_sunTargetRotation);
    SunLight->Color = m_sunTargetColor;
    SunLight->Brightness = m_sunTargetBrightness;

    // Update east ambient light (90 degrees from sun)
    if (EastAmbientLight != nullptr) {
        //float eastAngle = fmod(sunAngle + 90.0f, 360.0f);
        //Quaternion eastRotation = Quaternion::Euler(eastAngle, 180.0f, 0.0f);
        //EastAmbientLight->SetLocalOrientation(eastRotation);

        // East light is visible during dawn/dusk transitions
        //bool isEastVisible = (eastAngle < 180.0f);
        //float eastBrightness = isEastVisible ? AmbientIntensity : 0.0f;

        EastAmbientLight->Color = TransitionColor;
        //EastAmbientLight->Brightness = eastBrightness;
        EastAmbientLight->Brightness = AmbientIntensity;
    }

    if (DebugLogging) {
        LOG(Info, "Sun - Day progress: {0:0.3f}, Angle: {1:0.1f}, Brightness: {2:0.2f}",
            dayProgress, sunAngle, m_sunTargetBrightness);
    }
}

void DayNightCycle::UpdateMoon(float dayProgress)
{
    if (MoonLight == nullptr)
        return;

    // Moon is always 180 degrees offset from the sun
    float moonAngle = fmod(dayProgress * 360.0f + 180.0f, 360.0f);

    // Apply rotation
    m_moonTargetRotation = Quaternion::Euler(moonAngle, 180.0f, 0.0f);

    // Calculate visibility based on whether moon is above horizon
    bool isMoonVisible = (moonAngle < 180.0f);

    // Set brightness based on visibility
    if (isMoonVisible) {
        m_moonTargetBrightness = NighttimeIntensity;
        m_moonTargetColor = NighttimeColor;
    } else {
        m_moonTargetBrightness = 0.0f;
    }

    // Apply values to moon light
    MoonLight->SetLocalOrientation(m_moonTargetRotation);
    MoonLight->Color = m_moonTargetColor;
    MoonLight->Brightness = m_moonTargetBrightness;

    // Update west ambient light (90 degrees from moon, 270 degrees from sun)
    if (WestAmbientLight != nullptr) {
        //float westAngle = fmod(moonAngle + 90.0f, 360.0f);
        //Quaternion westRotation = Quaternion::Euler(westAngle, 180.0f, 0.0f);
        //WestAmbientLight->SetLocalOrientation(westRotation);

        // West light is visible during dusk/dawn transitions
        //bool isWestVisible = (westAngle < 180.0f);
        //float westBrightness = isWestVisible ? AmbientIntensity : 0.0f;

        WestAmbientLight->Color = TransitionColor;
        //WestAmbientLight->Brightness = westBrightness;
        WestAmbientLight->Brightness = AmbientIntensity;
    }

    if (DebugLogging) {
        LOG(Info, "Moon - Day progress: {0:0.3f}, Angle: {1:0.1f}, Brightness: {2:0.2f}",
            dayProgress, moonAngle, m_moonTargetBrightness);
    }
}

void DayNightCycle::FindAndAssignLights()
{
    LOG(Info, "Searching for directional lights to assign to day/night cycle...");

    Array<Actor*> lights = Level::GetActors(DirectionalLight::GetStaticClass(), true);

    // Reset all light assignments
    SunLight = nullptr;
    MoonLight = nullptr;
    EastAmbientLight = nullptr;
    WestAmbientLight = nullptr;

    // Count how many lights we find
    int assignedCount = 0;

    for (Actor* light : lights) {
        if (!light || !light->Is<DirectionalLight>())
            continue;

        DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
        String lightName = dirLight->GetName().ToLower();

        // Check each name pattern
        if (lightName.Contains(String("sun")) || lightName.Contains(String("day"))) {
            SunLight = dirLight;
            LOG(Info, "Assigned sun light: {0}", dirLight->GetNamePath());
            assignedCount++;
        } else if (lightName.Contains(String("moon")) || lightName.Contains(String("night"))) {
            MoonLight = dirLight;
            LOG(Info, "Assigned moon light: {0}", dirLight->GetNamePath());
            assignedCount++;
        } else if (lightName.Contains(String("east")) || lightName.Contains(String("dawn"))) {
            EastAmbientLight = dirLight;
            LOG(Info, "Assigned east ambient light: {0}", dirLight->GetNamePath());
            assignedCount++;
        } else if (lightName.Contains(String("west")) || lightName.Contains(String("dusk"))) {
            WestAmbientLight = dirLight;
            LOG(Info, "Assigned west ambient light: {0}", dirLight->GetNamePath());
            assignedCount++;
        }
    }

    // If we didn't find specific lights, try to assign any remaining directional lights
    if (assignedCount < 4) {
        LOG(Warning, "Some directional lights weren't found by name. Attempting to assign remaining lights automatically.");

        for (Actor* light : lights) {
            if (!light || !light->Is<DirectionalLight>())
                continue;

            DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);

            // Skip already assigned lights
            if (dirLight == SunLight || dirLight == MoonLight || dirLight == EastAmbientLight || dirLight == WestAmbientLight)
                continue;

            // Assign to first available slot
            if (SunLight == nullptr) {
                SunLight = dirLight;
                LOG(Info, "Auto-assigned sun light: {0}", dirLight->GetNamePath());
            } else if (MoonLight == nullptr) {
                MoonLight = dirLight;
                LOG(Info, "Auto-assigned moon light: {0}", dirLight->GetNamePath());
            } else if (EastAmbientLight == nullptr) {
                EastAmbientLight = dirLight;
                LOG(Info, "Auto-assigned east ambient light: {0}", dirLight->GetNamePath());
            } else if (WestAmbientLight == nullptr) {
                WestAmbientLight = dirLight;
                LOG(Info, "Auto-assigned west ambient light: {0}", dirLight->GetNamePath());
                break; // We've assigned all four lights now
            }
        }
    }

    // Log results
    if (SunLight == nullptr)
        LOG(Warning, "No directional light found for sun. Please create one named 'Sun' or 'Day'.");
    if (MoonLight == nullptr)
        LOG(Warning, "No directional light found for moon. Please create one named 'Moon' or 'Night'.");
    if (EastAmbientLight == nullptr)
        LOG(Warning, "No directional light found for east. Please create one named 'East' or 'Dawn'.");
    if (WestAmbientLight == nullptr)
        LOG(Warning, "No directional light found for west. Please create one named 'West' or 'Dusk'.");
}
// ^ DayNightCycle.cpp