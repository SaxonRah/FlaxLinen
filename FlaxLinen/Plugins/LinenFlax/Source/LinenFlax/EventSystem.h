// v EventSystem.h
#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>


#include "Engine/Core/Log.h"

// Event Priority enum class
enum class EventPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

// Base event class
class Event {
public:
    virtual ~Event() = default;
    virtual std::type_index GetType() const = 0;

    // Priority management
    EventPriority GetPriority() const { return m_priority; }
    void SetPriority(EventPriority priority) { m_priority = priority; }

protected:
    EventPriority m_priority = EventPriority::Normal;
};

// Template derived event
template <typename T>
class EventType : public Event {
public:
    std::type_index GetType() const override { return std::type_index(typeid(T)); }
};

// Event handler wrapper
class EventHandlerBase {
public:
    virtual ~EventHandlerBase() = default;
    virtual void Handle(const Event* event) const = 0;
};

template <typename T>
class EventHandler : public EventHandlerBase {
public:
    using HandlerFunc = std::function<void(const T&)>;

    EventHandler(HandlerFunc handler)
        : m_handler(handler)
    {
    }

    void Handle(const Event* event) const override
    {
        m_handler(*static_cast<const T*>(event));
    }

private:
    HandlerFunc m_handler;
};

// Optimized event system with filtering capabilities and priority support
class EventSystem {
public:
    template <typename T>
    void Subscribe(std::function<void(const T&)> handler, const std::string& filter = "")
    {
        static_assert(std::is_base_of<EventType<T>, T>::value, "T must derive from EventType<T>");

        LOG(Info, "Subscribing event of type {0}", String(typeid(T).name()));

        std::type_index type = std::type_index(typeid(T));
        auto handlerPtr = std::make_shared<EventHandler<T>>(handler);

        if (filter.empty()) {
            m_handlers[type].push_back(handlerPtr);
        } else {
            m_filteredHandlers[type][filter].push_back(handlerPtr);
        }
    }

    template <typename T>
    void Publish(const T& event, const std::string& filter = "", EventPriority priority = EventPriority::Normal)
    {
        static_assert(std::is_base_of<EventType<T>, T>::value, "T must derive from EventType<T>");

        LOG(Info, "Publishing event of type {0}", String(typeid(T).name()));

        // Create a shared copy of the event
        auto eventPtr = std::make_shared<T>(event);

        // Set the priority
        eventPtr->SetPriority(priority);

        // Queue the event
        std::type_index type = std::type_index(typeid(T));
        m_eventQueue.push(QueuedEvent(eventPtr, type, filter));
    }

    template <typename T>
    void PublishImmediate(const T& event, const std::string& filter = "")
    {
        static_assert(std::is_base_of<EventType<T>, T>::value, "T must derive from EventType<T>");

        LOG(Info, "Publishing immediate event of type {0}", String(typeid(T).name()));

        // Create a shared copy of the event
        auto eventPtr = std::make_shared<T>(event);
        std::type_index type = std::type_index(typeid(T));

        // Process global handlers
        if (m_handlers.find(type) != m_handlers.end()) {
            for (const auto& handler : m_handlers[type]) {
                handler->Handle(eventPtr.get());
            }
        }

        // Process filtered handlers
        if (!filter.empty() && m_filteredHandlers.find(type) != m_filteredHandlers.end() && m_filteredHandlers[type].find(filter) != m_filteredHandlers[type].end()) {
            for (const auto& handler : m_filteredHandlers[type][filter]) {
                handler->Handle(eventPtr.get());
            }
        }
    }

    void ProcessEvents()
    {
        std::vector<QueuedEvent> processingBatch;

        // Move all events from the priority queue to our processing batch
        while (!m_eventQueue.empty()) {
            processingBatch.push_back(m_eventQueue.top());
            m_eventQueue.pop();
        }

        // Process events in priority order (highest priority first)
        for (const auto& queuedEvent : processingBatch) {
            const auto& eventPtr = queuedEvent.event;
            const auto& type = queuedEvent.type;
            const auto& filter = queuedEvent.filter;

            // Process global handlers
            if (m_handlers.find(type) != m_handlers.end()) {
                for (const auto& handler : m_handlers[type]) {
                    handler->Handle(eventPtr.get());
                }
            }

            // Process filtered handlers
            if (!filter.empty() && m_filteredHandlers.find(type) != m_filteredHandlers.end() && m_filteredHandlers[type].find(filter) != m_filteredHandlers[type].end()) {
                for (const auto& handler : m_filteredHandlers[type][filter]) {
                    handler->Handle(eventPtr.get());
                }
            }
        }
    }

private:
    struct QueuedEvent {
        std::shared_ptr<Event> event;
        std::type_index type;
        std::string filter;

        // Constructor to capture priority from the event
        QueuedEvent(std::shared_ptr<Event> e, std::type_index t, const std::string& f)
            : event(e)
            , type(t)
            , filter(f)
        {
        }

        // Comparison operator for priority queue
        bool operator<(const QueuedEvent& other) const
        {
            // Higher priority value means higher actual priority in std::priority_queue
            return event->GetPriority() < other.event->GetPriority();
        }
    };

    std::unordered_map<std::type_index, std::vector<std::shared_ptr<EventHandlerBase>>> m_handlers;
    std::unordered_map<std::type_index, std::unordered_map<std::string, std::vector<std::shared_ptr<EventHandlerBase>>>> m_filteredHandlers;

    // Priority queue for event processing
    std::priority_queue<QueuedEvent> m_eventQueue;
};

/*
How to use the prioritized event system:

For a critical event that needs immediate attention:
    QuestFailedEvent event;
    event.questId = "main_quest";
    event.reason = "Time limit exceeded";
    m_plugin->GetEventSystem().Publish(event, "", EventPriority::Critical);

For a low-priority notification:
    PlayerLevelUpEvent event;
    event.newLevel = 5;
    m_plugin->GetEventSystem().Publish(event, "", EventPriority::Low);
*/
// ^ EventSystem.h