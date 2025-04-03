// v QuestEvents.h
#pragma once

#include "EventSystem.h"
#include "QuestTypes.h"
#include <string>


// Event fired when a quest is completed
class QuestCompletedEvent : public EventType<QuestCompletedEvent> {
public:
    std::string questId;
    std::string questTitle;
    int experienceGained = 0;
};

// Event fired when a quest's state changes
class QuestStateChangedEvent : public EventType<QuestStateChangedEvent> {
public:
    std::string questId;
    std::string questTitle;
    QuestState oldState;
    QuestState newState;
};

// ^ QuestEvents.h