// v TimeSystem.cpp
#include "TimeSystem.h"
#include "Engine/Core/Log.h"
#include "LinenFlax.h"
#include <iomanip>
#include <sstream>


TimeSystem::TimeSystem()
{
    // Initialize with default values
}

TimeSystem::~TimeSystem()
{
    Destroy();
}

void TimeSystem::Initialize()
{
    // Set initial values
    m_timeScale = 1.0f;
    m_accumulatedTime = 0.0f;
    m_minute = 0;
    m_hour = 6; // Start at 6 AM
    m_day = 1;
    m_month = 1;
    m_year = 1;

    LOG(Info, "Time System Initialized. Starting at {0} on day {1}/{2}/{3}",
        String(GetFormattedTime().c_str()), m_day, m_month, m_year);
}

void TimeSystem::Shutdown()
{
    LOG(Info, "Time System Shutdown.");
}

void TimeSystem::Update(float deltaTime)
{
    // Update game time based on deltaTime and time scale
    // Debug log
    static float debugTimer = 0.0f;
    debugTimer += deltaTime;
    if (debugTimer >= 1.0f) {
        debugTimer = 0.0f;
        LOG(Info, "TimeSystem Update: Time {0}, Hour: {1}, Day: {2}, Progress: {3:0.3f}",
            String(GetFormattedTime().c_str()), m_hour, m_day, GetDayProgress());
    }

    UpdateGameTime(deltaTime);
}

void TimeSystem::UpdateGameTime(float deltaTime)
{
    // Scale time according to time scale factor
    float scaledDelta = deltaTime * m_timeScale;

    // Add to accumulated time
    m_accumulatedTime += scaledDelta;

    // One minute passes every 60 seconds
    const float secondsPerMinute = 1.0f; // Adjust this for faster/slower time

    // Check if enough time has passed for a minute
    if (m_accumulatedTime >= secondsPerMinute) {
        int minutesToAdd = static_cast<int>(m_accumulatedTime / secondsPerMinute);
        m_accumulatedTime -= minutesToAdd * secondsPerMinute;

        // Add minutes
        m_minute += minutesToAdd;

        // Handle hour change
        if (m_minute >= 60) {
            int hoursToAdd = m_minute / 60;
            m_minute %= 60;

            int oldHour = m_hour;
            m_hour += hoursToAdd;

            // Handle day change
            if (m_hour >= 24) {
                int daysToAdd = m_hour / 24;
                m_hour %= 24;

                int oldDay = m_day;
                m_day += daysToAdd;

                // Handle month change
                if (m_day > m_daysPerMonth) {
                    int monthsToAdd = (m_day - 1) / m_daysPerMonth;
                    m_day = ((m_day - 1) % m_daysPerMonth) + 1;

                    std::string oldSeason = GetCurrentSeason();
                    m_month += monthsToAdd;

                    // Handle year change
                    if (m_month > m_monthsPerYear) {
                        int yearsToAdd = (m_month - 1) / m_monthsPerYear;
                        m_month = ((m_month - 1) % m_monthsPerYear) + 1;
                        m_year += yearsToAdd;
                    }

                    // Check for season change
                    std::string newSeason = GetCurrentSeason();
                    if (oldSeason != newSeason) {
                        SeasonChangedEvent event;
                        event.previousSeason = oldSeason;
                        event.newSeason = newSeason;
                        event.seasonDay = GetDayOfSeason();
                        m_plugin->GetEventSystem().PublishImmediate(event);

                        LOG(Info, "Season changed from {0} to {1}",
                            String(oldSeason.c_str()), String(newSeason.c_str()));
                    }
                }

                // Publish day changed event
                DayChangedEvent dayEvent;
                dayEvent.previousDay = oldDay;
                dayEvent.newDay = m_day;
                dayEvent.seasonName = GetCurrentSeason();
                m_plugin->GetEventSystem().PublishImmediate(dayEvent);

                LOG(Info, "Day changed to {0}/{1}/{2}", m_day, m_month, m_year);
            }

            // Publish hour changed event
            HourChangedEvent hourEvent;
            hourEvent.previousHour = oldHour;
            hourEvent.newHour = m_hour;
            hourEvent.isDayTime = IsDaytime();
            m_plugin->GetEventSystem().PublishImmediate(hourEvent);

            // Check for special time transitions (day/night)
            CheckForTimeEvents();
        }
    }
}

void TimeSystem::CheckForTimeEvents()
{
    TimeOfDay currentTimeOfDay = GetTimeOfDay();

    // Log changes in time of day
    switch (currentTimeOfDay) {
    case TimeOfDay::Dawn:
        LOG(Info, "Dawn breaks as the sun begins to rise");
        break;
    case TimeOfDay::Morning:
        LOG(Info, "Morning arrives as the world awakens");
        break;
    case TimeOfDay::Noon:
        LOG(Info, "The sun reaches its peak at noon");
        break;
    case TimeOfDay::Afternoon:
        LOG(Info, "The afternoon sun shines warmly");
        break;
    case TimeOfDay::Evening:
        LOG(Info, "Evening approaches as the day winds down");
        break;
    case TimeOfDay::Dusk:
        LOG(Info, "Dusk falls as the sun begins to set");
        break;
    case TimeOfDay::Night:
        LOG(Info, "Night blankets the world in darkness");
        break;
    case TimeOfDay::Midnight:
        LOG(Info, "Midnight marks the deepest part of night");
        break;
    }
}

void TimeSystem::SetTimeScale(float scale)
{
    if (scale < 0.0f) {
        LOG(Warning, "Cannot set negative time scale, using 0.0 instead");
        m_timeScale = 0.0f;
    } else {
        // Set a higher minimum to ensure movement
        if (scale > 0.0f && scale < 1.0f) {
            scale = 1.0f;
        }
        m_timeScale = scale;
        LOG(Info, "Time scale set to {0}x", m_timeScale);
    }
}

void TimeSystem::SetHour(int hour)
{
    if (hour >= 0 && hour < 24) {
        int oldHour = m_hour;
        m_hour = hour;

        HourChangedEvent event;
        event.previousHour = oldHour;
        event.newHour = m_hour;
        event.isDayTime = IsDaytime();
        m_plugin->GetEventSystem().PublishImmediate(event);

        LOG(Info, "Hour set to {0}", m_hour);
        CheckForTimeEvents();
    } else {
        LOG(Warning, "Invalid hour {0}. Must be between 0-23", hour);
    }
}

void TimeSystem::SetDay(int day)
{
    if (day > 0 && day <= m_daysPerMonth) {
        int oldDay = m_day;
        m_day = day;

        DayChangedEvent event;
        event.previousDay = oldDay;
        event.newDay = m_day;
        event.seasonName = GetCurrentSeason();
        m_plugin->GetEventSystem().PublishImmediate(event);

        LOG(Info, "Day set to {0}", m_day);
    } else {
        LOG(Warning, "Invalid day {0}. Must be between 1-{1}", day, m_daysPerMonth);
    }
}

void TimeSystem::SetMonth(int month)
{
    if (month > 0 && month <= m_monthsPerYear) {
        std::string oldSeason = GetCurrentSeason();
        m_month = month;
        std::string newSeason = GetCurrentSeason();

        if (oldSeason != newSeason) {
            SeasonChangedEvent event;
            event.previousSeason = oldSeason;
            event.newSeason = newSeason;
            event.seasonDay = GetDayOfSeason();
            m_plugin->GetEventSystem().PublishImmediate(event);
        }

        LOG(Info, "Month set to {0} ({1})", m_month, String(GetCurrentSeason().c_str()));
    } else {
        LOG(Warning, "Invalid month {0}. Must be between 1-{1}", month, m_monthsPerYear);
    }
}

void TimeSystem::SetYear(int year)
{
    if (year > 0) {
        m_year = year;
        LOG(Info, "Year set to {0}", m_year);
    } else {
        LOG(Warning, "Invalid year {0}. Must be greater than 0", year);
    }
}

TimeOfDay TimeSystem::GetTimeOfDay() const
{
    if (m_hour == 0) {
        return TimeOfDay::Midnight;
    } else if (m_hour < m_dawnHour) {
        return TimeOfDay::Night;
    } else if (m_hour < m_dayHour) {
        return TimeOfDay::Dawn;
    } else if (m_hour < 12) {
        return TimeOfDay::Morning;
    } else if (m_hour == 12) {
        return TimeOfDay::Noon;
    } else if (m_hour < m_duskHour) {
        return TimeOfDay::Afternoon;
    } else if (m_hour < m_nightHour) {
        return TimeOfDay::Dusk;
    } else {
        return TimeOfDay::Night;
    }
}

float TimeSystem::GetDayProgress() const
{
    // Calculate progress through the day (0.0-1.0)
    return (m_hour * 60 + m_minute) / (24.0f * 60.0f);
}

std::string TimeSystem::GetCurrentSeason() const
{
    int seasonIndex = (m_month - 1) % m_seasons.size();
    return m_seasons[seasonIndex];
}

int TimeSystem::GetDayOfSeason() const
{
    return m_day + ((m_month - 1) % m_seasons.size()) * m_daysPerMonth;
}

std::string TimeSystem::GetFormattedTime() const
{
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << m_hour;
    ss << ":" << std::setw(2) << std::setfill('0') << m_minute;
    return ss.str();
}

std::string TimeSystem::GetFormattedDate() const
{
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << m_day;
    ss << "/" << std::setw(2) << std::setfill('0') << m_month;
    ss << "/" << m_year;
    return ss.str();
}

bool TimeSystem::IsDaytime() const
{
    return m_hour >= m_dayHour && m_hour < m_duskHour;
}

void TimeSystem::AdvanceTimeSeconds(int seconds)
{
    if (seconds <= 0) {
        LOG(Warning, "Cannot advance by negative or zero seconds");
        return;
    }

    // Convert seconds to minutes for more manageable processing
    int minutes = seconds / 60;
    int remainingSeconds = seconds % 60;

    if (minutes > 0) {
        AdvanceTimeMinutes(minutes);
    }

    if (remainingSeconds > 0) {
        // Store old values for events
        int oldHour = m_hour;
        int oldMinute = m_minute;
        int oldDay = m_day;

        // Update seconds and accumulate time
        m_accumulatedTime += remainingSeconds;

        // Handle minute change
        if (m_accumulatedTime >= 60.0f) {
            int minutesToAdd = static_cast<int>(m_accumulatedTime / 60.0f);
            m_accumulatedTime = fmod(m_accumulatedTime, 60.0f);

            // If we have minutes to add, use the minutes function to handle all the cascading time changes
            if (minutesToAdd > 0) {
                AdvanceTimeMinutes(minutesToAdd);
                return; // Already handled events in the minutes function
            }
        }

        LOG(Info, "Advanced time by {0} seconds to {1} on {2}",
            remainingSeconds, String(GetFormattedTime().c_str()), String(GetFormattedDate().c_str()));
    }
}

void TimeSystem::AdvanceTimeMinutes(int minutes)
{
    if (minutes <= 0) {
        LOG(Warning, "Cannot advance by negative or zero minutes");
        return;
    }

    // Store old values for events
    int oldHour = m_hour;
    int oldMinute = m_minute;
    int oldDay = m_day;
    std::string oldSeason = GetCurrentSeason();

    // Update minute
    m_minute += minutes;

    // Handle hour change
    if (m_minute >= 60) {
        int hoursToAdd = m_minute / 60;
        m_minute %= 60;

        // Update hour
        m_hour += hoursToAdd;

        // Handle day change
        if (m_hour >= 24) {
            int daysToAdd = m_hour / 24;
            m_hour %= 24;

            // Update day
            m_day += daysToAdd;

            // Handle month change
            if (m_day > m_daysPerMonth) {
                int monthsToAdd = (m_day - 1) / m_daysPerMonth;
                m_day = ((m_day - 1) % m_daysPerMonth) + 1;

                // Update month
                m_month += monthsToAdd;

                // Handle year change
                if (m_month > m_monthsPerYear) {
                    int yearsToAdd = (m_month - 1) / m_monthsPerYear;
                    m_month = ((m_month - 1) % m_monthsPerYear) + 1;
                    m_year += yearsToAdd;
                }
            }
        }
    }

    // Fire events if needed
    if (m_hour != oldHour) {
        HourChangedEvent hourEvent;
        hourEvent.previousHour = oldHour;
        hourEvent.newHour = m_hour;
        hourEvent.isDayTime = IsDaytime();
        m_plugin->GetEventSystem().PublishImmediate(hourEvent);
    }

    if (m_day != oldDay) {
        DayChangedEvent dayEvent;
        dayEvent.previousDay = oldDay;
        dayEvent.newDay = m_day;
        dayEvent.seasonName = GetCurrentSeason();
        m_plugin->GetEventSystem().PublishImmediate(dayEvent);
    }

    std::string newSeason = GetCurrentSeason();
    if (oldSeason != newSeason) {
        SeasonChangedEvent seasonEvent;
        seasonEvent.previousSeason = oldSeason;
        seasonEvent.newSeason = newSeason;
        seasonEvent.seasonDay = GetDayOfSeason();
        m_plugin->GetEventSystem().PublishImmediate(seasonEvent);
    }

    LOG(Info, "Advanced time by {0} minutes to {1} on {2}",
        minutes, String(GetFormattedTime().c_str()), String(GetFormattedDate().c_str()));

    CheckForTimeEvents();
}

void TimeSystem::AdvanceTimeHours(int hours)
{
    if (hours <= 0) {
        LOG(Warning, "Cannot advance by negative or zero hours");
        return;
    }

    // Store old values for events
    int oldHour = m_hour;
    int oldDay = m_day;
    std::string oldSeason = GetCurrentSeason();

    // Update hour
    m_hour += hours;

    // Handle day change
    if (m_hour >= 24) {
        int daysToAdd = m_hour / 24;
        m_hour %= 24;

        // Update day
        m_day += daysToAdd;

        // Handle month change
        if (m_day > m_daysPerMonth) {
            int monthsToAdd = (m_day - 1) / m_daysPerMonth;
            m_day = ((m_day - 1) % m_daysPerMonth) + 1;

            // Update month
            m_month += monthsToAdd;

            // Handle year change
            if (m_month > m_monthsPerYear) {
                int yearsToAdd = (m_month - 1) / m_monthsPerYear;
                m_month = ((m_month - 1) % m_monthsPerYear) + 1;
                m_year += yearsToAdd;
            }
        }
    }

    // Fire events if needed
    if (m_hour != oldHour) {
        HourChangedEvent hourEvent;
        hourEvent.previousHour = oldHour;
        hourEvent.newHour = m_hour;
        hourEvent.isDayTime = IsDaytime();
        m_plugin->GetEventSystem().PublishImmediate(hourEvent);
    }

    if (m_day != oldDay) {
        DayChangedEvent dayEvent;
        dayEvent.previousDay = oldDay;
        dayEvent.newDay = m_day;
        dayEvent.seasonName = GetCurrentSeason();
        m_plugin->GetEventSystem().PublishImmediate(dayEvent);
    }

    std::string newSeason = GetCurrentSeason();
    if (oldSeason != newSeason) {
        SeasonChangedEvent seasonEvent;
        seasonEvent.previousSeason = oldSeason;
        seasonEvent.newSeason = newSeason;
        seasonEvent.seasonDay = GetDayOfSeason();
        m_plugin->GetEventSystem().PublishImmediate(seasonEvent);
    }

    LOG(Info, "Advanced time by {0} hours to {1} on {2}",
        hours, String(GetFormattedTime().c_str()), String(GetFormattedDate().c_str()));

    CheckForTimeEvents();
}

void TimeSystem::AdvanceDays(int days)
{
    if (days <= 0) {
        LOG(Warning, "Cannot advance by negative or zero days");
        return;
    }

    // Store old values for events
    int oldDay = m_day;
    std::string oldSeason = GetCurrentSeason();

    // Update day
    m_day += days;

    // Handle month change
    if (m_day > m_daysPerMonth) {
        int monthsToAdd = (m_day - 1) / m_daysPerMonth;
        m_day = ((m_day - 1) % m_daysPerMonth) + 1;

        // Update month
        m_month += monthsToAdd;

        // Handle year change
        if (m_month > m_monthsPerYear) {
            int yearsToAdd = (m_month - 1) / m_monthsPerYear;
            m_month = ((m_month - 1) % m_monthsPerYear) + 1;
            m_year += yearsToAdd;
        }
    }

    // Fire day changed event
    DayChangedEvent dayEvent;
    dayEvent.previousDay = oldDay;
    dayEvent.newDay = m_day;
    dayEvent.seasonName = GetCurrentSeason();
    m_plugin->GetEventSystem().PublishImmediate(dayEvent);

    // Check for season change
    std::string newSeason = GetCurrentSeason();
    if (oldSeason != newSeason) {
        SeasonChangedEvent seasonEvent;
        seasonEvent.previousSeason = oldSeason;
        seasonEvent.newSeason = newSeason;
        seasonEvent.seasonDay = GetDayOfSeason();
        m_plugin->GetEventSystem().PublishImmediate(seasonEvent);
    }

    LOG(Info, "Advanced by {0} days to {1}", days, String(GetFormattedDate().c_str()));
}

void TimeSystem::DebugSetTime(int hour, int minute)
{
    if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60) {
        int oldHour = m_hour;
        m_hour = hour;
        m_minute = minute;

        HourChangedEvent event;
        event.previousHour = oldHour;
        event.newHour = m_hour;
        event.isDayTime = IsDaytime();
        m_plugin->GetEventSystem().PublishImmediate(event);

        LOG(Info, "Debug: Time set to {0}", String(GetFormattedTime().c_str()));
        CheckForTimeEvents();
    } else {
        LOG(Warning, "Invalid time {0}:{1}", hour, minute);
    }
}

void TimeSystem::Serialize(BinaryWriter& writer) const
{
    writer.Write(m_timeScale);
    writer.Write(m_accumulatedTime);
    writer.Write(static_cast<int32_t>(m_minute));
    writer.Write(static_cast<int32_t>(m_hour));
    writer.Write(static_cast<int32_t>(m_day));
    writer.Write(static_cast<int32_t>(m_month));
    writer.Write(static_cast<int32_t>(m_year));

    // System config values
    writer.Write(static_cast<int32_t>(m_dawnHour));
    writer.Write(static_cast<int32_t>(m_dayHour));
    writer.Write(static_cast<int32_t>(m_duskHour));
    writer.Write(static_cast<int32_t>(m_nightHour));
    writer.Write(static_cast<int32_t>(m_daysPerMonth));
    writer.Write(static_cast<int32_t>(m_monthsPerYear));

    // Seasons
    writer.Write(static_cast<uint32_t>(m_seasons.size()));
    for (const auto& season : m_seasons) {
        writer.Write(season);
    }

    LOG(Info, "TimeSystem serialized");
}

void TimeSystem::Deserialize(BinaryReader& reader)
{
    reader.Read(m_timeScale);
    reader.Read(m_accumulatedTime);

    int32_t value;

    reader.Read(value);
    m_minute = value;
    reader.Read(value);
    m_hour = value;
    reader.Read(value);
    m_day = value;
    reader.Read(value);
    m_month = value;
    reader.Read(value);
    m_year = value;

    // System config values
    reader.Read(value);
    m_dawnHour = value;
    reader.Read(value);
    m_dayHour = value;
    reader.Read(value);
    m_duskHour = value;
    reader.Read(value);
    m_nightHour = value;
    reader.Read(value);
    m_daysPerMonth = value;
    reader.Read(value);
    m_monthsPerYear = value;

    // Seasons
    uint32_t seasonCount;
    reader.Read(seasonCount);

    m_seasons.clear();
    for (uint32_t i = 0; i < seasonCount; ++i) {
        std::string season;
        reader.Read(season);
        m_seasons.push_back(season);
    }

    LOG(Info, "TimeSystem deserialized: Current time {0} on {1}",
        String(GetFormattedTime().c_str()), String(GetFormattedDate().c_str()));
}

void TimeSystem::SerializeToText(TextWriter& writer) const
{
    writer.Write("timeScale", m_timeScale);
    writer.Write("accumulatedTime", m_accumulatedTime);
    writer.Write("minute", m_minute);
    writer.Write("hour", m_hour);
    writer.Write("day", m_day);
    writer.Write("month", m_month);
    writer.Write("year", m_year);

    // System config values
    writer.Write("dawnHour", m_dawnHour);
    writer.Write("dayHour", m_dayHour);
    writer.Write("duskHour", m_duskHour);
    writer.Write("nightHour", m_nightHour);
    writer.Write("daysPerMonth", m_daysPerMonth);
    writer.Write("monthsPerYear", m_monthsPerYear);

    // Seasons
    writer.Write("seasonCount", static_cast<int>(m_seasons.size()));
    for (size_t i = 0; i < m_seasons.size(); ++i) {
        writer.Write("season" + std::to_string(i), m_seasons[i]);
    }

    LOG(Info, "TimeSystem serialized to text");
}

void TimeSystem::DeserializeFromText(TextReader& reader)
{
    reader.Read("timeScale", m_timeScale);
    reader.Read("accumulatedTime", m_accumulatedTime);
    reader.Read("minute", m_minute);
    reader.Read("hour", m_hour);
    reader.Read("day", m_day);
    reader.Read("month", m_month);
    reader.Read("year", m_year);

    // System config values
    reader.Read("dawnHour", m_dawnHour);
    reader.Read("dayHour", m_dayHour);
    reader.Read("duskHour", m_duskHour);
    reader.Read("nightHour", m_nightHour);
    reader.Read("daysPerMonth", m_daysPerMonth);
    reader.Read("monthsPerYear", m_monthsPerYear);

    // Seasons
    int seasonCount = 0;
    reader.Read("seasonCount", seasonCount);

    m_seasons.clear();
    for (int i = 0; i < seasonCount; ++i) {
        std::string season;
        reader.Read("season" + std::to_string(i), season);
        m_seasons.push_back(season);
    }

    LOG(Info, "TimeSystem deserialized from text: Current time {0} on {1}",
        String(GetFormattedTime().c_str()), String(GetFormattedDate().c_str()));
}