// v TimeSystem.h
#pragma once

#include "RPGSystem.h"
#include "EventSystem.h"
#include "Serialization.h"
#include <chrono>
#include <string>
#include <vector>

// Time-related events
class DayChangedEvent : public EventType<DayChangedEvent> {
public:
    int previousDay;
    int newDay;
    std::string seasonName;
};

class HourChangedEvent : public EventType<HourChangedEvent> {
public:
    int previousHour;
    int newHour;
    bool isDayTime;
};

class SeasonChangedEvent : public EventType<SeasonChangedEvent> {
public:
    std::string previousSeason;
    std::string newSeason;
    int seasonDay;
};

// Enum for time of day
enum class TimeOfDay {
    Dawn,
    Morning, 
    Noon,
    Afternoon,
    Evening,
    Dusk, 
    Night,
    Midnight
};

class TimeSystem : public RPGSystem {
public:
    // Delete copy constructor and assignment operator
    TimeSystem(const TimeSystem&) = delete;
    TimeSystem& operator=(const TimeSystem&) = delete;
    
    // Meyer's Singleton - thread-safe in C++11 and beyond
    static TimeSystem* GetInstance() {
        static TimeSystem* instance = new TimeSystem();
        return instance;
    }
    
    // Cleanup method
    static void Destroy() {
        static TimeSystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }
    
    ~TimeSystem();

    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetName() const override { return "TimeSystem"; }
    
    // Serialization
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);
    
    // Game time setters
    void SetTimeScale(float scale);
    void SetHour(int hour);
    void SetDay(int day);
    void SetMonth(int month);
    void SetYear(int year);
    
    // Game time getters
    float GetTimeScale() const { return m_timeScale; }
    int GetHour() const { return m_hour; }
    int GetDay() const { return m_day; }
    int GetMonth() const { return m_month; }
    int GetYear() const { return m_year; }
    int GetMinute() const { return m_minute; }
    int GetDaysPerMonth() const { return m_daysPerMonth; }
    int GetMonthsPerYear() const { return m_monthsPerYear; }
    TimeOfDay GetTimeOfDay() const;
    
    // Time calculations
    float GetDayProgress() const; // 0.0-1.0 representing progress through the day
    std::string GetCurrentSeason() const;
    int GetDayOfSeason() const;
    std::string GetFormattedTime() const; // Returns HH:MM format
    std::string GetFormattedDate() const; // Returns DD/MM/YYYY format
    bool IsDaytime() const;
    
    // Season info
    const std::vector<std::string>& GetSeasons() const { return m_seasons; }
    
    // Time manipulation
    void AdvanceTimeSeconds(int seconds);
    void AdvanceTimeMinutes(int minutes);
    void AdvanceTimeHours(int hours);
    void AdvanceDays(int days);
    
    // Debugging
    void DebugSetTime(int hour, int minute);

private:
    // Private constructor
    TimeSystem();
    
    // Time tracking
    float m_timeScale = 1.0f; // Game time passes this many times faster than real time
    float m_accumulatedTime = 0.0f;
    int m_minute = 0;
    int m_hour = 6; // Start at 6 AM
    int m_day = 1;
    int m_month = 1;
    int m_year = 1;
    
    // System configuration
    int m_dawnHour = 5;
    int m_dayHour = 7;
    int m_duskHour = 18;
    int m_nightHour = 20;
    int m_daysPerMonth = 30;
    int m_monthsPerYear = 4;
    
    // Define seasons (usually 4)
    std::vector<std::string> m_seasons = {"Spring", "Summer", "Fall", "Winter"};
    
    // Helper methods
    void UpdateGameTime(float deltaTime);
    void CheckForTimeEvents();
};