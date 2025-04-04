// v WeatherSystem.cpp
#include "WeatherSystem.h"
#include "Engine/Core/Log.h"
#include "LinenFlax.h"
#include "TimeSystem.h"
#include <ctime>


// WeatherState serialization
void WeatherState::Serialize(BinaryWriter& writer) const
{
    writer.Write(static_cast<int32_t>(m_condition));
    writer.Write(m_intensity);
    writer.Write(m_transitionProgress);
    writer.Write(m_duration);
    writer.Write(m_remainingTime);
    writer.Write(m_isDangerous);
}

void WeatherState::Deserialize(BinaryReader& reader)
{
    int32_t conditionValue;
    reader.Read(conditionValue);
    m_condition = static_cast<WeatherCondition>(conditionValue);

    reader.Read(m_intensity);
    reader.Read(m_transitionProgress);
    reader.Read(m_duration);
    reader.Read(m_remainingTime);
    reader.Read(m_isDangerous);
}

void WeatherState::SerializeToText(TextWriter& writer) const
{
    writer.Write("weatherCondition", static_cast<int>(m_condition));
    writer.Write("weatherIntensity", m_intensity);
    writer.Write("weatherTransitionProgress", m_transitionProgress);
    writer.Write("weatherDuration", m_duration);
    writer.Write("weatherRemainingTime", m_remainingTime);
    writer.Write("weatherIsDangerous", m_isDangerous ? 1 : 0);
}

void WeatherState::DeserializeFromText(TextReader& reader)
{
    int conditionValue;
    if (reader.Read("weatherCondition", conditionValue))
        m_condition = static_cast<WeatherCondition>(conditionValue);

    reader.Read("weatherIntensity", m_intensity);
    reader.Read("weatherTransitionProgress", m_transitionProgress);
    reader.Read("weatherDuration", m_duration);
    reader.Read("weatherRemainingTime", m_remainingTime);

    int dangerousValue;
    if (reader.Read("weatherIsDangerous", dangerousValue))
        m_isDangerous = (dangerousValue != 0);
}

WeatherSystem::WeatherSystem()
{
    // Define system dependencies
    m_dependencies.insert("TimeSystem");

    // Initialize random generator with a time-based seed
    m_randomGenerator.seed(static_cast<unsigned int>(std::time(nullptr)));
}

WeatherSystem::~WeatherSystem()
{
    Destroy();
}

void WeatherSystem::Initialize()
{
    // Initialize current weather to clear
    m_currentWeather.SetCondition(WeatherCondition::Clear);
    m_currentWeather.SetIntensity(0.0f);
    m_currentWeather.SetDuration(m_minWeatherDuration);
    m_currentWeather.SetRemainingTime(m_minWeatherDuration);
    m_currentWeather.SetIsDangerous(false);

    // Copy to target weather
    m_targetWeather = m_currentWeather;

    // Set up initial season-based weather probabilities
    auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
    if (timeSystem) {
        UpdateWeatherProbabilities(timeSystem->GetCurrentSeason(), timeSystem->GetTimeOfDay());
    }

    // Subscribe to time events
    if (m_plugin) {
        // Subscribe to hour changed events
        m_plugin->GetEventSystem().Subscribe<HourChangedEvent>(
            [this](const HourChangedEvent& event) {
                // Update time tracking
                m_timeSinceLastCheck += 1.0f; // 1 hour

                // Reduce remaining weather time
                float remainingTime = m_currentWeather.GetRemainingTime() - 1.0f;
                m_currentWeather.SetRemainingTime(remainingTime);

                // Check if we should update weather
                if (ShouldTriggerWeatherChange()) {
                    PickNewWeather();
                }
            });

        // Subscribe to season changed events
        m_plugin->GetEventSystem().Subscribe<SeasonChangedEvent>(
            [this](const SeasonChangedEvent& event) {
                // Get time system for time of day
                auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
                if (timeSystem) {
                    // Update probabilities for new season
                    UpdateWeatherProbabilities(event.newSeason, timeSystem->GetTimeOfDay());

                    // Force weather change for season transition
                    PickNewWeather();
                }
            });
    }

    LOG(Info, "Weather System Initialized. Starting weather: Clear");
}

void WeatherSystem::Shutdown()
{
    LOG(Info, "Weather System Shutdown.");
}

void WeatherSystem::Update(float deltaTime)
{
    // Don't need continuous updates as we're using hour events for major changes
    // But we do need to handle transitions
    if (m_isTransitioning) {
        UpdateWeatherTransition(deltaTime);
    }
}

void WeatherSystem::UpdateWeatherProbabilities(const std::string& season, TimeOfDay timeOfDay)
{
    // Clear previous probabilities
    m_seasonWeatherProbabilities.clear();

    // Base weather probabilities that apply to all seasons and times
    std::unordered_map<WeatherCondition, float> baseProbabilities = {
        { WeatherCondition::Clear, 0.3f },
        { WeatherCondition::Cloudy, 0.2f },
        { WeatherCondition::Overcast, 0.1f },
        { WeatherCondition::Foggy, 0.05f },
        { WeatherCondition::Rain, 0.15f },
        { WeatherCondition::Thunderstorm, 0.05f },
        { WeatherCondition::Snow, 0.0f },
        { WeatherCondition::Blizzard, 0.0f },
        { WeatherCondition::Heatwave, 0.0f },
        { WeatherCondition::Windy, 0.15f }
    };

    // Adjust for season
    if (season == "Spring") {
        baseProbabilities[WeatherCondition::Clear] = 0.25f;
        baseProbabilities[WeatherCondition::Cloudy] = 0.2f;
        baseProbabilities[WeatherCondition::Rain] = 0.25f;
        baseProbabilities[WeatherCondition::Thunderstorm] = 0.1f;
        baseProbabilities[WeatherCondition::Foggy] = 0.1f;
        baseProbabilities[WeatherCondition::Windy] = 0.1f;
    } else if (season == "Summer") {
        baseProbabilities[WeatherCondition::Clear] = 0.4f;
        baseProbabilities[WeatherCondition::Cloudy] = 0.15f;
        baseProbabilities[WeatherCondition::Rain] = 0.1f;
        baseProbabilities[WeatherCondition::Thunderstorm] = 0.15f;
        baseProbabilities[WeatherCondition::Heatwave] = 0.15f;
        baseProbabilities[WeatherCondition::Foggy] = 0.05f;
    } else if (season == "Fall") {
        baseProbabilities[WeatherCondition::Clear] = 0.2f;
        baseProbabilities[WeatherCondition::Cloudy] = 0.25f;
        baseProbabilities[WeatherCondition::Overcast] = 0.2f;
        baseProbabilities[WeatherCondition::Rain] = 0.15f;
        baseProbabilities[WeatherCondition::Foggy] = 0.15f;
        baseProbabilities[WeatherCondition::Windy] = 0.05f;
    } else if (season == "Winter") {
        baseProbabilities[WeatherCondition::Clear] = 0.2f;
        baseProbabilities[WeatherCondition::Cloudy] = 0.15f;
        baseProbabilities[WeatherCondition::Overcast] = 0.15f;
        baseProbabilities[WeatherCondition::Snow] = 0.25f;
        baseProbabilities[WeatherCondition::Blizzard] = 0.1f;
        baseProbabilities[WeatherCondition::Foggy] = 0.1f;
        baseProbabilities[WeatherCondition::Windy] = 0.05f;
    }

    // Adjust for time of day
    if (timeOfDay == TimeOfDay::Dawn || timeOfDay == TimeOfDay::Dusk) {
        // More fog at dawn/dusk
        baseProbabilities[WeatherCondition::Foggy] += 0.1f;

        // Reduce some others
        baseProbabilities[WeatherCondition::Clear] -= 0.05f;
        baseProbabilities[WeatherCondition::Thunderstorm] -= 0.05f;
    } else if (timeOfDay == TimeOfDay::Night || timeOfDay == TimeOfDay::Midnight) {
        // Fewer thunderstorms at night
        baseProbabilities[WeatherCondition::Thunderstorm] -= 0.05f;

        // More clear nights
        baseProbabilities[WeatherCondition::Clear] += 0.05f;
    }

    // Store final probabilities
    m_seasonWeatherProbabilities[season] = baseProbabilities;

    LOG(Info, "Updated weather probabilities for season: {0}", String(season.c_str()));
}

void WeatherSystem::PickNewWeather()
{
    if (m_weatherLocked) {
        LOG(Info, "Weather is locked, skipping weather change");
        return;
    }

    // Get current season from time system
    auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
    if (!timeSystem) {
        LOG(Warning, "TimeSystem not available, can't determine probabilities");
        return;
    }

    std::string currentSeason = timeSystem->GetCurrentSeason();

    // Get probabilities for current season
    auto& probabilities = m_seasonWeatherProbabilities[currentSeason];
    if (probabilities.empty()) {
        // Fallback to default probabilities if none found for season
        UpdateWeatherProbabilities(currentSeason, timeSystem->GetTimeOfDay());
        probabilities = m_seasonWeatherProbabilities[currentSeason];
    }

    // Create a distribution based on probabilities
    std::vector<WeatherCondition> conditions;
    std::vector<float> weights;

    for (const auto& pair : probabilities) {
        conditions.push_back(pair.first);
        weights.push_back(pair.second);
    }

    // Create discrete distribution
    std::discrete_distribution<> distribution(weights.begin(), weights.end());

    // Pick a random weather condition
    WeatherCondition newCondition = conditions[distribution(m_randomGenerator)];

    // Determine intensity (0.0 to 1.0)
    std::uniform_real_distribution<float> intensityDist(0.3f, 1.0f);
    float intensity = intensityDist(m_randomGenerator);

    // Determine duration
    std::uniform_real_distribution<float> durationDist(m_minWeatherDuration, m_maxWeatherDuration);
    float duration = durationDist(m_randomGenerator);

    // Set up target weather
    m_targetWeather.SetCondition(newCondition);
    m_targetWeather.SetIntensity(intensity);
    m_targetWeather.SetDuration(duration);
    m_targetWeather.SetRemainingTime(duration);

    // Determine if dangerous
    m_targetWeather.SetIsDangerous(
        newCondition == WeatherCondition::Thunderstorm || newCondition == WeatherCondition::Blizzard || (newCondition == WeatherCondition::Heatwave && intensity > 0.7f));

    // Begin transition
    m_isTransitioning = true;

    LOG(Info, "Weather changing to: {0} (Intensity: {1:0.2f}, Duration: {2:0.1f} hours, Dangerous: {3})",
        String(WeatherConditionToString(newCondition).c_str()),
        intensity,
        duration,
        String(m_targetWeather.IsDangerous() ? "Yes" : "No"));

    // Reset transition progress
    m_targetWeather.SetTransitionProgress(0.0f);

    // Reset time since last check
    m_timeSinceLastCheck = 0.0f;
}

void WeatherSystem::UpdateWeatherTransition(float deltaTime)
{
    if (!m_isTransitioning)
        return;

    // Get current transition progress
    float progress = m_targetWeather.GetTransitionProgress();

    // Calculate new progress
    progress += deltaTime * m_transitionSpeed * 0.1f; // Scale transition to be reasonable

    // Check if transition is complete
    if (progress >= 1.0f) {
        progress = 1.0f;
        m_isTransitioning = false;

        // Fire event for complete weather change
        WeatherChangedEvent event;
        event.previousWeather = WeatherConditionToString(m_currentWeather.GetCondition());
        event.newWeather = WeatherConditionToString(m_targetWeather.GetCondition());
        event.intensity = m_targetWeather.GetIntensity();
        event.isDangerous = m_targetWeather.IsDangerous();

        m_plugin->GetEventSystem().Publish(event);

        // Update current weather to match target
        m_currentWeather = m_targetWeather;

        LOG(Info, "Weather transition complete: {0} (Intensity: {1:0.2f})",
            String(WeatherConditionToString(m_currentWeather.GetCondition()).c_str()),
            m_currentWeather.GetIntensity());
    }

    // Update progress
    m_targetWeather.SetTransitionProgress(progress);
}

bool WeatherSystem::ShouldTriggerWeatherChange()
{
    // Check if current weather's time is up
    if (m_currentWeather.GetRemainingTime() <= 0.0f) {
        return true;
    }

    // Or if we've reached the update frequency
    if (m_timeSinceLastCheck >= m_weatherUpdateFrequency) {
        // Apply a random chance to change even if duration isn't up
        std::uniform_real_distribution<float> changeDist(0.0f, 1.0f);
        float changeChance = changeDist(m_randomGenerator);

        // 20% chance to change weather even if not expired
        if (changeChance < 0.2f) {
            LOG(Info, "Random weather change triggered");
            return true;
        }
    }

    return false;
}

WeatherCondition WeatherSystem::GetCurrentWeather() const
{
    if (m_isTransitioning) {
        // During transition, blend between current and target based on progress
        float progress = m_targetWeather.GetTransitionProgress();
        if (progress > 0.5f) {
            return m_targetWeather.GetCondition();
        }
    }
    return m_currentWeather.GetCondition();
}

float WeatherSystem::GetWeatherIntensity() const
{
    if (m_isTransitioning) {
        // Blend intensity during transition
        float progress = m_targetWeather.GetTransitionProgress();
        return m_currentWeather.GetIntensity() * (1.0f - progress) + m_targetWeather.GetIntensity() * progress;
    }
    return m_currentWeather.GetIntensity();
}

bool WeatherSystem::IsWeatherDangerous() const
{
    if (m_isTransitioning) {
        // If transitioning to dangerous, consider dangerous after 50% transition
        float progress = m_targetWeather.GetTransitionProgress();
        if (progress > 0.5f) {
            return m_targetWeather.IsDangerous();
        }
    }
    return m_currentWeather.IsDangerous();
}

std::string WeatherSystem::GetWeatherName() const
{
    return WeatherConditionToString(GetCurrentWeather());
}

float WeatherSystem::GetTransitionProgress() const
{
    if (m_isTransitioning) {
        return m_targetWeather.GetTransitionProgress();
    }
    return 1.0f; // Not transitioning means fully at current weather
}

const std::unordered_map<WeatherCondition, float>& WeatherSystem::GetWeatherProbabilities() const
{
    auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
    std::string currentSeason = timeSystem ? timeSystem->GetCurrentSeason() : "Spring";

    // If we have probabilities for this season, return them
    auto it = m_seasonWeatherProbabilities.find(currentSeason);
    if (it != m_seasonWeatherProbabilities.end()) {
        return it->second;
    }

    // Otherwise return empty map (should never happen)
    static std::unordered_map<WeatherCondition, float> emptyMap;
    return emptyMap;
}

void WeatherSystem::ForceWeatherChange(WeatherCondition condition, float intensity, float transitionDuration)
{
    // Set up target weather
    m_targetWeather.SetCondition(condition);
    m_targetWeather.SetIntensity(intensity);

    // Determine duration
    std::uniform_real_distribution<float> durationDist(m_minWeatherDuration, m_maxWeatherDuration);
    float duration = durationDist(m_randomGenerator);

    m_targetWeather.SetDuration(duration);
    m_targetWeather.SetRemainingTime(duration);

    // Determine if dangerous
    m_targetWeather.SetIsDangerous(
        condition == WeatherCondition::Thunderstorm || condition == WeatherCondition::Blizzard || (condition == WeatherCondition::Heatwave && intensity > 0.7f));

    // Begin transition
    m_isTransitioning = true;

    // Adjust transition speed for requested duration
    float originalSpeed = m_transitionSpeed;
    if (transitionDuration > 0.0f) {
        m_transitionSpeed = 1.0f / transitionDuration;
    }

    LOG(Info, "Forced weather change to: {0} (Intensity: {1:0.2f}, Duration: {2:0.1f} hours, Transition: {3:0.1f}s)",
        String(WeatherConditionToString(condition).c_str()),
        intensity,
        duration,
        transitionDuration);

    // Reset transition progress
    m_targetWeather.SetTransitionProgress(0.0f);

    // Reset time since last check
    m_timeSinceLastCheck = 0.0f;

    // Restore original transition speed after a delay
    // In a real implementation, you'd want to store this and restore it after transition completes
    m_transitionSpeed = originalSpeed;
}

void WeatherSystem::SetWeatherDuration(float hours)
{
    if (hours < 1.0f)
        hours = 1.0f;

    m_currentWeather.SetDuration(hours);
    m_currentWeather.SetRemainingTime(hours);

    LOG(Info, "Set current weather duration to {0} hours", hours);
}

void WeatherSystem::SetWeatherTransitionSpeed(float speed)
{
    if (speed <= 0.0f) {
        LOG(Warning, "Cannot set non-positive transition speed, using 0.1");
        speed = 0.1f;
    }

    m_transitionSpeed = speed;
    LOG(Info, "Weather transition speed set to {0}", speed);
}

void WeatherSystem::ToggleWeatherLock(bool locked)
{
    m_weatherLocked = locked;
    LOG(Info, "Weather lock {0}", String(locked ? "enabled" : "disabled"));
}

bool WeatherSystem::IsWeatherLocked() const
{
    return m_weatherLocked;
}

std::string WeatherSystem::WeatherConditionToString(WeatherCondition condition) const
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

WeatherCondition WeatherSystem::StringToWeatherCondition(const std::string& str) const
{
    if (str == "Clear")
        return WeatherCondition::Clear;
    if (str == "Cloudy")
        return WeatherCondition::Cloudy;
    if (str == "Overcast")
        return WeatherCondition::Overcast;
    if (str == "Foggy")
        return WeatherCondition::Foggy;
    if (str == "Rain")
        return WeatherCondition::Rain;
    if (str == "Thunderstorm")
        return WeatherCondition::Thunderstorm;
    if (str == "Snow")
        return WeatherCondition::Snow;
    if (str == "Blizzard")
        return WeatherCondition::Blizzard;
    if (str == "Heatwave")
        return WeatherCondition::Heatwave;
    if (str == "Windy")
        return WeatherCondition::Windy;

    return WeatherCondition::Clear; // Default
}

void WeatherSystem::Serialize(BinaryWriter& writer) const
{
    // Serialize current and target weather
    m_currentWeather.Serialize(writer);
    m_targetWeather.Serialize(writer);

    // Serialize system state
    writer.Write(m_weatherUpdateFrequency);
    writer.Write(m_minWeatherDuration);
    writer.Write(m_maxWeatherDuration);
    writer.Write(m_transitionSpeed);
    writer.Write(m_timeSinceLastCheck);
    writer.Write(m_isTransitioning);
    writer.Write(m_weatherLocked);

    LOG(Info, "WeatherSystem serialized");
}

void WeatherSystem::Deserialize(BinaryReader& reader)
{
    // Deserialize current and target weather
    m_currentWeather.Deserialize(reader);
    m_targetWeather.Deserialize(reader);

    // Deserialize system state
    reader.Read(m_weatherUpdateFrequency);
    reader.Read(m_minWeatherDuration);
    reader.Read(m_maxWeatherDuration);
    reader.Read(m_transitionSpeed);
    reader.Read(m_timeSinceLastCheck);
    reader.Read(m_isTransitioning);
    reader.Read(m_weatherLocked);

    LOG(Info, "WeatherSystem deserialized: Current weather {0} (Intensity: {1:0.2f})",
        String(WeatherConditionToString(m_currentWeather.GetCondition()).c_str()),
        m_currentWeather.GetIntensity());
}

void WeatherSystem::SerializeToText(TextWriter& writer) const
{
    // Write a prefix for current weather
    writer.Write("currentWeather_prefix", "current");

    // Serialize current weather
    m_currentWeather.SerializeToText(writer);

    // Write a prefix for target weather
    writer.Write("targetWeather_prefix", "target");

    // Serialize target weather
    m_targetWeather.SerializeToText(writer);

    // Serialize system state
    writer.Write("weatherUpdateFrequency", m_weatherUpdateFrequency);
    writer.Write("minWeatherDuration", m_minWeatherDuration);
    writer.Write("maxWeatherDuration", m_maxWeatherDuration);
    writer.Write("transitionSpeed", m_transitionSpeed);
    writer.Write("timeSinceLastCheck", m_timeSinceLastCheck);
    writer.Write("isTransitioning", m_isTransitioning ? 1 : 0);
    writer.Write("weatherLocked", m_weatherLocked ? 1 : 0);

    LOG(Info, "WeatherSystem serialized to text");
}

void WeatherSystem::DeserializeFromText(TextReader& reader)
{
    // Deserialize current weather
    m_currentWeather.DeserializeFromText(reader);

    // Deserialize target weather
    m_targetWeather.DeserializeFromText(reader);

    // Deserialize system state
    reader.Read("weatherUpdateFrequency", m_weatherUpdateFrequency);
    reader.Read("minWeatherDuration", m_minWeatherDuration);
    reader.Read("maxWeatherDuration", m_maxWeatherDuration);
    reader.Read("transitionSpeed", m_transitionSpeed);
    reader.Read("timeSinceLastCheck", m_timeSinceLastCheck);

    int isTransitioning;
    if (reader.Read("isTransitioning", isTransitioning))
        m_isTransitioning = (isTransitioning != 0);

    int weatherLocked;
    if (reader.Read("weatherLocked", weatherLocked))
        m_weatherLocked = (weatherLocked != 0);

    LOG(Info, "WeatherSystem deserialized from text: Current weather {0} (Intensity: {1:0.2f})",
        String(WeatherConditionToString(m_currentWeather.GetCondition()).c_str()),
        m_currentWeather.GetIntensity());
}
// ^ WeatherSystem.cpp