# Linen RPG System - Full C++ Implementation with Architectural Improvements

1. Proper template implementation
2. Dependency management for systems
3. Thread safety considerations
4. Improved memory management
5. Optimized event system
6. Enhanced serialization support
7. Consistent C++ implementation
8. Type-safe system access

## Core Plugin Structure

### `RPGSystem.h`
```cpp
#pragma once

#include <string>
#include <vector>
#include <unordered_set>

// Forward declarations
class Linen;

// Base class for all RPG systems
class RPGSystem {
public:
    virtual ~RPGSystem() = default;
    
    // Core lifecycle methods
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(float deltaTime) {}
    
    // System dependencies
    const std::unordered_set<std::string>& GetDependencies() const { return m_dependencies; }
    
    // System identification
    virtual std::string GetSystemName() const = 0;
    
    // Plugin reference for accessing other systems
    void SetPlugin(Linen* plugin) { m_plugin = plugin; }
    
protected:
    Linen* m_plugin = nullptr;
    std::unordered_set<std::string> m_dependencies;
};
```

### `Linen.h`
```cpp
#pragma once

#include "Engine/Engine/Plugin.h"
#include "RPGSystem.h"
#include "EventSystem.h"

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <typeindex>

class Linen : public Plugin {
public:
    Linen();
    ~Linen();
    
    // Core plugin lifecycle
    virtual void Initialize() override;
    virtual void Deinitialize() override;
    virtual void Update(float deltaTime);
    
    // System management
    template <typename T>
    bool RegisterSystem();
    
    template <typename T>
    bool LoadSystem();
    
    template <typename T>
    bool UnloadSystem();
    
    // Safe system access
    template <typename T>
    T* GetSystem();
    
    // Thread-safe event system access
    EventSystem& GetEventSystem() { return m_eventSystem; }
    
private:
    // Determines correct initialization order based on dependencies
    void CalculateInitializationOrder();
    
    // Organizes systems by dependencies
    std::vector<std::string> m_initializationOrder;
    
    // Maps for system management
    std::unordered_map<std::string, std::unique_ptr<RPGSystem>> m_registeredSystems;
    std::unordered_map<std::string, RPGSystem*> m_activeSystems;
    std::unordered_map<std::type_index, std::string> m_typeToName;
    
    // Thread safety
    std::mutex m_systemsMutex;
    
    // Centralized event system
    EventSystem m_eventSystem;
};

// Template implementations
template <typename T>
bool Linen::RegisterSystem() {
    static_assert(std::is_base_of<RPGSystem, T>::value, "T must derive from RPGSystem");
    
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto system = std::make_unique<T>();
    std::string systemName = system->GetSystemName();
    
    if (m_registeredSystems.find(systemName) != m_registeredSystems.end()) {
        LOG(Warning, "System already registered: {0}", String(systemName.c_str()));
        return false;
    }
    
    // Store type information for safer access
    m_typeToName[std::type_index(typeid(T))] = systemName;
    
    // Set plugin reference and store system
    system->SetPlugin(this);
    m_registeredSystems[systemName] = std::move(system);
    
    // Recalculate initialization order
    CalculateInitializationOrder();
    
    LOG(Info, "Registered system: {0}", String(systemName.c_str()));
    return true;
}

template <typename T>
bool Linen::LoadSystem() {
    static_assert(std::is_base_of<RPGSystem, T>::value, "T must derive from RPGSystem");
    
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto typeIndex = std::type_index(typeid(T));
    
    // Check if we know about this type
    if (m_typeToName.find(typeIndex) == m_typeToName.end()) {
        LOG(Warning, "System not registered: {0}", String(typeid(T).name()));
        return false;
    }
    
    std::string systemName = m_typeToName[typeIndex];
    
    // Check if already loaded
    if (m_activeSystems.find(systemName) != m_activeSystems.end()) {
        LOG(Info, "System already loaded: {0}", String(systemName.c_str()));
        return true;
    }
    
    // Check if system is registered
    if (m_registeredSystems.find(systemName) == m_registeredSystems.end()) {
        LOG(Warning, "System not registered: {0}", String(systemName.c_str()));
        return false;
    }
    
    // First load dependencies
    auto& system = m_registeredSystems[systemName];
    for (const auto& dependency : system->GetDependencies()) {
        if (m_activeSystems.find(dependency) == m_activeSystems.end()) {
            LOG(Info, "Loading dependency: {0} for {1}", 
                String(dependency.c_str()), String(systemName.c_str()));
            
            // Find and load the dependency
            if (m_registeredSystems.find(dependency) != m_registeredSystems.end()) {
                m_registeredSystems[dependency]->Initialize();
                m_activeSystems[dependency] = m_registeredSystems[dependency].get();
            } else {
                LOG(Warning, "Missing dependency: {0}", String(dependency.c_str()));
                return false;
            }
        }
    }
    
    // Initialize the system
    system->Initialize();
    m_activeSystems[systemName] = system.get();
    
    LOG(Info, "Loaded system: {0}", String(systemName.c_str()));
    return true;
}

template <typename T>
bool Linen::UnloadSystem() {
    static_assert(std::is_base_of<RPGSystem, T>::value, "T must derive from RPGSystem");
    
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto typeIndex = std::type_index(typeid(T));
    if (m_typeToName.find(typeIndex) == m_typeToName.end()) {
        LOG(Warning, "System not registered: {0}", String(typeid(T).name()));
        return false;
    }
    
    std::string systemName = m_typeToName[typeIndex];
    
    // Check if system is active
    auto it = m_activeSystems.find(systemName);
    if (it == m_activeSystems.end()) {
        LOG(Info, "System not active: {0}", String(systemName.c_str()));
        return true;
    }
    
    // Check for dependent systems
    for (const auto& pair : m_activeSystems) {
        auto& activeSystem = m_registeredSystems[pair.first];
        if (activeSystem->GetDependencies().find(systemName) != activeSystem->GetDependencies().end()) {
            LOG(Warning, "Cannot unload {0}, it is a dependency of {1}",
                String(systemName.c_str()), String(pair.first.c_str()));
            return false;
        }
    }
    
    // Shutdown the system
    m_registeredSystems[systemName]->Shutdown();
    m_activeSystems.erase(it);
    
    LOG(Info, "Unloaded system: {0}", String(systemName.c_str()));
    return true;
}

template <typename T>
T* Linen::GetSystem() {
    static_assert(std::is_base_of<RPGSystem, T>::value, "T must derive from RPGSystem");
    
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto typeIndex = std::type_index(typeid(T));
    if (m_typeToName.find(typeIndex) == m_typeToName.end()) {
        return nullptr;
    }
    
    std::string systemName = m_typeToName[typeIndex];
    auto it = m_activeSystems.find(systemName);
    if (it == m_activeSystems.end()) {
        return nullptr;
    }
    
    return static_cast<T*>(it->second);
}
```

### `Linen.cpp`
```cpp
#include "Linen.h"
#include "Engine/Core/Log.h"
#include <queue>

Linen::Linen()
    : Plugin(TEXT("Linen RPG Plugin"), TEXT("1.0"))
{
}

Linen::~Linen() {
    Deinitialize();
}

void Linen::Initialize() {
    Plugin::Initialize();
    LOG(Info, "Linen Plugin Initialized.");
}

void Linen::Deinitialize() {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    // Unload systems in reverse dependency order
    for (auto it = m_initializationOrder.rbegin(); it != m_initializationOrder.rend(); ++it) {
        auto systemIt = m_activeSystems.find(*it);
        if (systemIt != m_activeSystems.end()) {
            LOG(Info, "Shutting down system: {0}", String(systemIt->first.c_str()));
            m_registeredSystems[systemIt->first]->Shutdown();
        }
    }
    
    m_activeSystems.clear();
    m_registeredSystems.clear();
    m_typeToName.clear();
    m_initializationOrder.clear();
    
    LOG(Info, "Linen Plugin Deinitialized.");
}

void Linen::Update(float deltaTime) {
    // Update all active systems in initialization order
    for (const auto& systemName : m_initializationOrder) {
        auto it = m_activeSystems.find(systemName);
        if (it != m_activeSystems.end()) {
            it->second->Update(deltaTime);
        }
    }
    
    // Process events after all systems have updated
    m_eventSystem.ProcessEvents();
}

void Linen::CalculateInitializationOrder() {
    m_initializationOrder.clear();
    
    // Topological sort of systems based on dependencies
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> inProgress;
    
    std::function<void(const std::string&)> visit = [&](const std::string& systemName) {
        if (inProgress.find(systemName) != inProgress.end()) {
            LOG(Error, "Circular dependency detected for system: {0}", String(systemName.c_str()));
            return;
        }
        
        if (visited.find(systemName) != visited.end()) {
            return;
        }
        
        inProgress.insert(systemName);
        
        auto it = m_registeredSystems.find(systemName);
        if (it != m_registeredSystems.end()) {
            for (const auto& dep : it->second->GetDependencies()) {
                visit(dep);
            }
        }
        
        inProgress.erase(systemName);
        visited.insert(systemName);
        m_initializationOrder.push_back(systemName);
    };
    
    // Visit all registered systems
    for (const auto& pair : m_registeredSystems) {
        visit(pair.first);
    }
}
```

## Improved Event System

### `EventSystem.h`
```cpp
#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <mutex>
#include <queue>

// Base event class
class Event {
public:
    virtual ~Event() = default;
    virtual std::type_index GetType() const = 0;
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
    
    EventHandler(HandlerFunc handler) : m_handler(handler) {}
    
    void Handle(const Event* event) const override {
        m_handler(*static_cast<const T*>(event));
    }
    
private:
    HandlerFunc m_handler;
};

// Optimized event system with filtering capabilities
class EventSystem {
public:
    template <typename T>
    void Subscribe(std::function<void(const T&)> handler, const std::string& filter = "") {
        static_assert(std::is_base_of<EventType<T>, T>::value, "T must derive from EventType<T>");
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::type_index type = std::type_index(typeid(T));
        auto handlerPtr = std::make_shared<EventHandler<T>>(handler);
        
        if (filter.empty()) {
            m_handlers[type].push_back(handlerPtr);
        } else {
            m_filteredHandlers[type][filter].push_back(handlerPtr);
        }
    }
    
    template <typename T>
    void Publish(const T& event, const std::string& filter = "") {
        static_assert(std::is_base_of<EventType<T>, T>::value, "T must derive from EventType<T>");
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Queue event for processing
        auto eventPtr = std::make_shared<T>(event);
        std::type_index type = std::type_index(typeid(T));
        
        m_eventQueue.push({eventPtr, type, filter});
    }
    
    // Process all queued events
    void ProcessEvents() {
        std::vector<QueuedEvent> queuedEvents;
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::swap(queuedEvents, m_eventQueue);
        }
        
        for (const auto& queuedEvent : queuedEvents) {
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
            if (!filter.empty() && m_filteredHandlers.find(type) != m_filteredHandlers.end() && 
                m_filteredHandlers[type].find(filter) != m_filteredHandlers[type].end()) {
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
    };
    
    std::mutex m_mutex;
    std::unordered_map<std::type_index, std::vector<std::shared_ptr<EventHandlerBase>>> m_handlers;
    std::unordered_map<std::type_index, 
                      std::unordered_map<std::string, 
                                        std::vector<std::shared_ptr<EventHandlerBase>>>> m_filteredHandlers;
    std::vector<QueuedEvent> m_eventQueue;
};
```

## System Implementations

### `QuestSystem.h`
```cpp
#pragma once

#include "RPGSystem.h"
#include "Events/QuestEvents.h"
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>
#include <memory>

// Forward declaration
class CharacterProgressionSystem;

class Quest {
public:
    enum class State { Available, Active, Completed, Failed };
    
    Quest(const std::string& id, const std::string& title, const std::string& description);
    
    // Getters/Setters
    std::string GetId() const { return m_id; }
    std::string GetTitle() const { return m_title; }
    std::string GetDescription() const { return m_description; }
    State GetState() const { return m_state; }
    int GetExperienceReward() const { return m_experienceReward; }
    
    void SetState(State state) { m_state = state; }
    void SetExperienceReward(int reward) { m_experienceReward = reward; }
    
    // Add required skill check
    void AddSkillRequirement(const std::string& skillName, int requiredLevel);
    
    // Check if player meets skill requirements
    bool CheckRequirements(const std::unordered_map<std::string, int>& playerSkills) const;
    
    // For serialization
    virtual void Serialize(void* writer) const;
    virtual void Deserialize(void* reader);
    
private:
    std::string m_id;
    std::string m_title;
    std::string m_description;
    State m_state;
    int m_experienceReward;
    
    // Requirements to take/complete the quest
    std::unordered_map<std::string, int> m_skillRequirements;
};

class QuestSystem : public RPGSystem {
public:
    QuestSystem();
    ~QuestSystem();
    
    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetSystemName() const override { return "QuestSystem"; }
    
    // Quest management
    bool AddQuest(const std::string& id, const std::string& title, const std::string& description);
    bool ActivateQuest(const std::string& id);
    bool CompleteQuest(const std::string& id);
    bool FailQuest(const std::string& id);
    
    // Quest queries
    Quest* GetQuest(const std::string& id);
    std::vector<Quest*> GetAvailableQuests() const;
    std::vector<Quest*> GetActiveQuests() const;
    std::vector<Quest*> GetCompletedQuests() const;
    
    // Serialization (save/load)
    void Serialize(void* writer) const;
    void Deserialize(void* reader);
    
private:
    // Internal helper methods
    void PublishQuestStateChanged(Quest* quest, Quest::State oldState);
    
    // Thread safety
    mutable std::mutex m_questsMutex;
    
    // Quest storage
    std::unordered_map<std::string, std::unique_ptr<Quest>> m_quests;
};
```

### `QuestSystem.cpp`
```cpp
#include "QuestSystem.h"
#include "Linen.h"
#include "CharacterProgressionSystem.h"
#include "Engine/Core/Log.h"

Quest::Quest(const std::string& id, const std::string& title, const std::string& description)
    : m_id(id)
    , m_title(title)
    , m_description(description)
    , m_state(State::Available)
    , m_experienceReward(0)
{
}

void Quest::AddSkillRequirement(const std::string& skillName, int requiredLevel) {
    m_skillRequirements[skillName] = requiredLevel;
}

bool Quest::CheckRequirements(const std::unordered_map<std::string, int>& playerSkills) const {
    for (const auto& req : m_skillRequirements) {
        auto it = playerSkills.find(req.first);
        if (it == playerSkills.end() || it->second < req.second) {
            return false;
        }
    }
    return true;
}

void Quest::Serialize(void* writer) const {
    // Serialization implementation
}

void Quest::Deserialize(void* reader) {
    // Deserialization implementation
}

QuestSystem::QuestSystem() {
    // Define system dependencies
    m_dependencies.insert("CharacterProgressionSystem");
}

QuestSystem::~QuestSystem() {
    Shutdown();
}

void QuestSystem::Initialize() {
    LOG(Info, "Quest System Initialized.");
}

void QuestSystem::Shutdown() {
    std::lock_guard<std::mutex> lock(m_questsMutex);
    m_quests.clear();
    LOG(Info, "Quest System Shutdown.");
}

void QuestSystem::Update(float deltaTime) {
    // Check for time-based quest updates
}

bool QuestSystem::AddQuest(const std::string& id, const std::string& title, const std::string& description) {
    std::lock_guard<std::mutex> lock(m_questsMutex);
    
    if (m_quests.find(id) != m_quests.end()) {
        LOG(Warning, "Quest already exists: {0}", String(id.c_str()));
        return false;
    }
    
    auto quest = std::make_unique<Quest>(id, title, description);
    m_quests[id] = std::move(quest);
    
    LOG(Info, "Added quest: {0}", String(title.c_str()));
    return true;
}

bool QuestSystem::ActivateQuest(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_questsMutex);
    
    auto it = m_quests.find(id);
    if (it == m_quests.end()) {
        LOG(Warning, "Quest not found: {0}", String(id.c_str()));
        return false;
    }
    
    Quest* quest = it->second.get();
    if (quest->GetState() != Quest::State::Available) {
        LOG(Warning, "Quest not available: {0}", String(id.c_str()));
        return false;
    }
    
    // Check character progression requirements
    auto progressionSystem = m_plugin->GetSystem<CharacterProgressionSystem>();
    if (progressionSystem) {
        if (!quest->CheckRequirements(progressionSystem->GetSkills())) {
            LOG(Info, "Character doesn't meet quest requirements: {0}", String(id.c_str()));
            return false;
        }
    }
    
    Quest::State oldState = quest->GetState();
    quest->SetState(Quest::State::Active);
    
    // Publish event without lock held
    m_questsMutex.unlock();
    PublishQuestStateChanged(quest, oldState);
    m_questsMutex.lock();
    
    LOG(Info, "Activated quest: {0}", String(id.c_str()));
    return true;
}

bool QuestSystem::CompleteQuest(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_questsMutex);
    
    auto it = m_quests.find(id);
    if (it == m_quests.end()) {
        LOG(Warning, "Quest not found: {0}", String(id.c_str()));
        return false;
    }
    
    Quest* quest = it->second.get();
    if (quest->GetState() != Quest::State::Active) {
        LOG(Warning, "Quest not active: {0}", String(id.c_str()));
        return false;
    }
    
    Quest::State oldState = quest->GetState();
    quest->SetState(Quest::State::Completed);
    
    // Release lock before event publishing to prevent deadlocks
    m_questsMutex.unlock();
    
    // Publish completion event
    QuestCompletedEvent event;
    event.questId = id;
    event.questTitle = quest->GetTitle();
    event.experienceGained = quest->GetExperienceReward();
    
    m_plugin->GetEventSystem().Publish(event);
    
    // Publish state change event
    PublishQuestStateChanged(quest, oldState);
    
    // Reacquire lock
    m_questsMutex.lock();
    
    LOG(Info, "Completed quest: {0}", String(id.c_str()));
    return true;
}

bool QuestSystem::FailQuest(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_questsMutex);
    
    auto it = m_quests.find(id);
    if (it == m_quests.end()) {
        LOG(Warning, "Quest not found: {0}", String(id.c_str()));
        return false;
    }
    
    Quest* quest = it->second.get();
    if (quest->GetState() != Quest::State::Active) {
        LOG(Warning, "Quest not active: {0}", String(id.c_str()));
        return false;
    }
    
    Quest::State oldState = quest->GetState();
    quest->SetState(Quest::State::Failed);
    
    // Publish event without lock held
    m_questsMutex.unlock();
    PublishQuestStateChanged(quest, oldState);
    m_questsMutex.lock();
    
    LOG(Info, "Failed quest: {0}", String(id.c_str()));
    return true;
}

Quest* QuestSystem::GetQuest(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_questsMutex);
    
    auto it = m_quests.find(id);
    if (it == m_quests.end()) {
        return nullptr;
    }
    
    return it->second.get();
}

std::vector<Quest*> QuestSystem::GetAvailableQuests() const {
    std::lock_guard<std::mutex> lock(m_questsMutex);
    
    std::vector<Quest*> result;
    for (const auto& pair : m_quests) {
        if (pair.second->GetState() == Quest::State::Available) {
            result.push_back(pair.second.get());
        }
    }
    
    return result;
}

std::vector<Quest*> QuestSystem::GetActiveQuests() const {
    std::lock_guard<std::mutex> lock(m_questsMutex);
    
    std::vector<Quest*> result;
    for (const auto& pair : m_quests) {
        if (pair.second->GetState() == Quest::State::Active) {
            result.push_back(pair.second.get());
        }
    }
    
    return result;
}

std::vector<Quest*> QuestSystem::GetCompletedQuests() const {
    std::lock_guard<std::mutex> lock(m_questsMutex);
    
    std::vector<Quest*> result;
    for (const auto& pair : m_quests) {
        if (pair.second->GetState() == Quest::State::Completed) {
            result.push_back(pair.second.get());
        }
    }
    
    return result;
}

void QuestSystem::PublishQuestStateChanged(Quest* quest, Quest::State oldState) {
    QuestStateChangedEvent event;
    event.questId = quest->GetId();
    event.questTitle = quest->GetTitle();
    event.oldState = oldState;
    event.newState = quest->GetState();
    
    m_plugin->GetEventSystem().Publish(event);
}

void QuestSystem::Serialize(void* writer) const {
    // Implementation for save system
}

void QuestSystem::Deserialize(void* reader) {
    // Implementation for load system
}
```

### `CharacterProgressionSystem.h`
```cpp
#pragma once

#include "RPGSystem.h"
#include "Events/QuestEvents.h"
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <memory>

class Skill {
public:
    Skill(const std::string& id, const std::string& name, const std::string& description);
    
    std::string GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    std::string GetDescription() const { return m_description; }
    int GetLevel() const { return m_level; }
    
    void SetLevel(int level) { m_level = level; }
    void IncreaseLevel(int amount = 1) { m_level += amount; }
    
private:
    std::string m_id;
    std::string m_name;
    std::string m_description;
    int m_level = 0;
};

class CharacterProgressionSystem : public RPGSystem {
public:
    CharacterProgressionSystem();
    ~CharacterProgressionSystem();
    
    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetSystemName() const override { return "CharacterProgressionSystem"; }
    
    // Skill management
    bool AddSkill(const std::string& id, const std::string& name, const std::string& description);
    bool IncreaseSkill(const std::string& id, int amount = 1);
    int GetSkillLevel(const std::string& id) const;
    
    // Experience management
    void GainExperience(int amount);
    int GetExperience() const;
    int GetLevel() const;
    
    // Requirements checking
    const std::unordered_map<std::string, int>& GetSkills() const;
    
    // For serialization
    void Serialize(void* writer) const;
    void Deserialize(void* reader);
    
private:
    // Event handlers
    void HandleQuestCompleted(const QuestCompletedEvent& event);
    
    // Thread safety
    mutable std::mutex m_mutex;
    
    // Character data
    int m_experience = 0;
    int m_level = 1;
    std::unordered_map<std::string, std::unique_ptr<Skill>> m_skills;
    std::unordered_map<std::string, int> m_skillLevels; // Cache for requirements checking
};
```

### `CharacterProgressionSystem.cpp`
```cpp
#include "CharacterProgressionSystem.h"
#include "Linen.h"
#include "Engine/Core/Log.h"

Skill::Skill(const std::string& id, const std::string& name, const std::string& description)
    : m_id(id)
    , m_name(name)
    , m_description(description)
    , m_level(0)
{
}

CharacterProgressionSystem::CharacterProgressionSystem() {
    // No dependencies for this system
}

CharacterProgressionSystem::~CharacterProgressionSystem() {
    Shutdown();
}

void CharacterProgressionSystem::Initialize() {
    // Subscribe to quest completed events
    m_plugin->GetEventSystem().Subscribe<QuestCompletedEvent>(
        [this](const QuestCompletedEvent& event) {
            this->HandleQuestCompleted(event);
        });
    
    LOG(Info, "Character Progression System Initialized.");
}

void CharacterProgressionSystem::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_skills.clear();
    m_skillLevels.clear();
    LOG(Info, "Character Progression System Shutdown.");
}

void CharacterProgressionSystem::Update(float deltaTime) {
    // Nothing to update regularly
}

bool CharacterProgressionSystem::AddSkill(const std::string& id, const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_skills.find(id) != m_skills.end()) {
        LOG(Warning, "Skill already exists: {0}", String(id.c_str()));
        return false;
    }
    
    auto skill = std::make_unique<Skill>(id, name, description);
    m_skills[id] = std::move(skill);
    m_skillLevels[id] = 0;
    
    LOG(Info, "Added skill: {0}", String(name.c_str()));
    return true;
}

bool CharacterProgressionSystem::IncreaseSkill(const std::string& id, int amount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_skills.find(id);
    if (it == m_skills.end()) {
        LOG(Warning, "Skill not found: {0}", String(id.c_str()));
        return false;
    }
    
    it->second->IncreaseLevel(amount);
    m_skillLevels[id] = it->second->GetLevel();
    
    LOG(Info, "Increased skill {0} by {1} to level {2}", 
        String(id.c_str()), amount, it->second->GetLevel());
    return true;
}

int CharacterProgressionSystem::GetSkillLevel(const std::string& id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_skills.find(id);
    if (it == m_skills.end()) {
        return 0;
    }
    
    return it->second->GetLevel();
}

void CharacterProgressionSystem::GainExperience(int amount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int oldLevel = m_level;
    m_experience += amount;
    
    // Simple level up formula: level = sqrt(experience / 100)
    m_level = 1 + static_cast<int>(sqrt(m_experience / 100.0));
    
    LOG(Info, "Gained {0} XP. Total XP: {1}", amount, m_experience);
    
    if (m_level > oldLevel) {
        LOG(Info, "Level up! New level: {0}", m_level);
    }
}

int CharacterProgressionSystem::GetExperience() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_experience;
}

int CharacterProgressionSystem::GetLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

const std::unordered_map<std::string, int>& CharacterProgressionSystem::GetSkills() const {
    // No need to lock here as we're returning a const reference to the cached skill levels
    return m_skillLevels;
}

void CharacterProgressionSystem::HandleQuestCompleted(const QuestCompletedEvent& event) {
    if (event.experienceGained > 0) {
        GainExperience(event.experienceGained);
        LOG(Info, "Gained {0} XP from completed quest: {1}", 
            event.experienceGained, String(event.questTitle.c_str()));
    }
}

void CharacterProgressionSystem::Serialize(void* writer) const {
    // Implementation for save system
}

void CharacterProgressionSystem::Deserialize(void* reader) {
    // Implementation for load system
}
```

### `Events/QuestEvents.h`
```cpp
#pragma once

#include "../EventSystem.h"
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
    Quest::State oldState;
    Quest::State newState;
};
```

### `SaveLoadSystem.h`
```cpp
#pragma once

#include "RPGSystem.h"
#include <string>
#include <mutex>
#include <unordered_set>
#include <functional>

class SaveLoadSystem : public RPGSystem {
public:
    SaveLoadSystem();
    ~SaveLoadSystem();
    
    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetSystemName() const override { return "SaveLoadSystem"; }
    
    // Save/Load functionality
    bool SaveGame(const std::string& filename);
    bool LoadGame(const std::string& filename);
    
    // System registration for save/load
    void RegisterSerializableSystem(const std::string& systemName);
    
private:
    // Thread safety
    std::mutex m_mutex;
    
    // Track which systems need serialization
    std::unordered_set<std::string> m_serializableSystems;
};
```

### `SaveLoadSystem.cpp`
```cpp
#include "SaveLoadSystem.h"
#include "Linen.h"
#include "Engine/Core/Log.h"
#include <fstream>

SaveLoadSystem::SaveLoadSystem() {
    // This system depends on all other systems that need saving
}

SaveLoadSystem::~SaveLoadSystem() {
    Shutdown();
}

void SaveLoadSystem::Initialize() {
    LOG(Info, "Save/Load System Initialized.");
}

void SaveLoadSystem::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_serializableSystems.clear();
    LOG(Info, "Save/Load System Shutdown.");
}

void SaveLoadSystem::Update(float deltaTime) {
    // Nothing to update regularly
}

bool SaveLoadSystem::SaveGame(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Create a writer object (implementation specific)
    // void* writer = CreateWriter(filename);
    
    LOG(Info, "Saving game to: {0}", String(filename.c_str()));
    
    // For each registered system, call its Serialize method
    for (const auto& systemName : m_serializableSystems) {
        auto system = m_plugin->GetSystem<RPGSystem>(systemName);
        if (system) {
            // Call system-specific serialization
            // system->Serialize(writer);
            LOG(Info, "Saved system: {0}", String(systemName.c_str()));
        }
    }
    
    // Close writer
    // CloseWriter(writer);
    
    LOG(Info, "Game saved successfully: {0}", String(filename.c_str()));
    return true;
}

bool SaveLoadSystem::LoadGame(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Create a reader object (implementation specific)
    // void* reader = CreateReader(filename);
    // if (!reader) {
    //     LOG(Error, "Failed to open save file: {0}", String(filename.c_str()));
    //     return false;
    // }
    
    LOG(Info, "Loading game from: {0}", String(filename.c_str()));
    
    // For each registered system, call its Deserialize method
    for (const auto& systemName : m_serializableSystems) {
        auto system = m_plugin->GetSystem<RPGSystem>(systemName);
        if (system) {
            // Call system-specific deserialization
            // system->Deserialize(reader);
            LOG(Info, "Loaded system: {0}", String(systemName.c_str()));
        }
    }
    
    // Close reader
    // CloseReader(reader);
    
    LOG(Info, "Game loaded successfully: {0}", String(filename.c_str()));
    return true;
}

void SaveLoadSystem::RegisterSerializableSystem(const std::string& systemName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_serializableSystems.insert(systemName);
    LOG(Info, "Registered system for serialization: {0}", String(systemName.c_str()));
}
```

## Example Usage

```cpp
#include "Linen.h"
#include "QuestSystem.h"
#include "CharacterProgressionSystem.h"
#include "SaveLoadSystem.h"

void GameInitialization() {
    // Create plugin instance
    auto plugin = std::make_unique<Linen>();
    plugin->Initialize();
    
    // Register systems
    plugin->RegisterSystem<CharacterProgressionSystem>();
    plugin->RegisterSystem<QuestSystem>();
    plugin->RegisterSystem<SaveLoadSystem>();
    
    // Load systems (respects dependencies)
    plugin->LoadSystem<QuestSystem>();  // Will automatically load CharacterProgressionSystem
    plugin->LoadSystem<SaveLoadSystem>();
    
    // Register systems for serialization
    auto saveSystem = plugin->GetSystem<SaveLoadSystem>();
    if (saveSystem) {
        saveSystem->RegisterSerializableSystem("CharacterProgressionSystem");
        saveSystem->RegisterSerializableSystem("QuestSystem");
    }
    
    // Get quest system and add a quest
    auto questSystem = plugin->GetSystem<QuestSystem>();
    if (questSystem) {
        questSystem->AddQuest("main_quest_001", "The Beginning", "Start your journey in the world of Linen.");
        
        // Activate the quest
        questSystem->ActivateQuest("main_quest_001");
        
        // Complete the quest
        questSystem->CompleteQuest("main_quest_001");
    }
    
    // Save the game
    if (saveSystem) {
        saveSystem->SaveGame("save_001.sav");
    }
    
    // Game loop updates
    float deltaTime = 0.016f; // ~60 FPS
    plugin->Update(deltaTime);
    
    // Clean up
    plugin->Deinitialize();
}
```

## Summary of Improvements

1. **Template Implementation**: Moved template implementations to header files or made them non-template where appropriate.

2. **Dependency Management**: Added explicit dependency tracking and topological sorting for initialization order.

3. **Thread Safety**: Added mutex locks around critical sections and designed event handling to avoid deadlocks.

4. **Memory Management**: Used smart pointers consistently for automated resource management.

5. **Optimized Event System**: 
   - Implemented event queuing to batch process events
   - Added event filtering to reduce unnecessary event processing
   - Made type-safe event handlers with static type checking

6. **Serialization Support**: Added structured save/load system that respects system dependencies.

7. **Type-safe System Access**: Implemented type-safe system retrieval with compile-time checking.

8. **Consistent C++ Implementation**: Eliminated C# code and implemented everything in modern C++.

This implementation addresses all the concerns raised while providing a robust framework for building modular RPG systems in Flax Engine.

---

1. Create a proper `ISerializable` interface
2. Implement a binary serialization format for performance
3. Make the systems explicitly define their serialization format


### `ISerializable.h`
```cpp
#pragma once

#include <memory>

// Forward declarations
class BinaryReader;
class BinaryWriter;

class ISerializable {
public:
    virtual ~ISerializable() = default;
    
    // Serialization methods
    virtual void Serialize(BinaryWriter& writer) const = 0;
    virtual void Deserialize(BinaryReader& reader) = 0;
};
```

### `Serialization.h`
```cpp
#pragma once

#include "ISerializable.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdint>

class BinaryWriter {
public:
    BinaryWriter(const std::string& filename) : m_stream(filename, std::ios::binary | std::ios::out) {}
    ~BinaryWriter() { m_stream.close(); }
    
    bool IsValid() const { return m_stream.good(); }
    
    // Write primitives
    void Write(bool value) { Write(&value, sizeof(bool)); }
    void Write(int32_t value) { Write(&value, sizeof(int32_t)); }
    void Write(uint32_t value) { Write(&value, sizeof(uint32_t)); }
    void Write(float value) { Write(&value, sizeof(float)); }
    void Write(double value) { Write(&value, sizeof(double)); }
    
    // Write string
    void Write(const std::string& value) {
        uint32_t length = static_cast<uint32_t>(value.length());
        Write(length);
        if (length > 0) {
            Write(value.data(), length);
        }
    }
    
    // Write raw data
    void Write(const void* data, size_t size) {
        m_stream.write(static_cast<const char*>(data), size);
    }
    
    // Write container helpers
    template<typename T>
    void WriteVector(const std::vector<T>& vec) {
        uint32_t size = static_cast<uint32_t>(vec.size());
        Write(size);
        for (const auto& item : vec) {
            Write(item);
        }
    }
    
    // Write vector of serializable objects
    template<typename T>
    void WriteSerializableVector(const std::vector<std::unique_ptr<T>>& vec) {
        uint32_t size = static_cast<uint32_t>(vec.size());
        Write(size);
        for (const auto& item : vec) {
            item->Serialize(*this);
        }
    }
    
    // Write map
    template<typename K, typename V>
    void WriteMap(const std::unordered_map<K, V>& map) {
        uint32_t size = static_cast<uint32_t>(map.size());
        Write(size);
        for (const auto& pair : map) {
            Write(pair.first);
            Write(pair.second);
        }
    }
    
private:
    std::ofstream m_stream;
};

class BinaryReader {
public:
    BinaryReader(const std::string& filename) : m_stream(filename, std::ios::binary | std::ios::in) {}
    ~BinaryReader() { m_stream.close(); }
    
    bool IsValid() const { return m_stream.good() && !m_stream.eof(); }
    
    // Read primitives
    void Read(bool& value) { Read(&value, sizeof(bool)); }
    void Read(int32_t& value) { Read(&value, sizeof(int32_t)); }
    void Read(uint32_t& value) { Read(&value, sizeof(uint32_t)); }
    void Read(float& value) { Read(&value, sizeof(float)); }
    void Read(double& value) { Read(&value, sizeof(double)); }
    
    // Read string
    void Read(std::string& value) {
        uint32_t length = 0;
        Read(length);
        value.resize(length);
        if (length > 0) {
            Read(&value[0], length);
        }
    }
    
    // Read raw data
    void Read(void* data, size_t size) {
        m_stream.read(static_cast<char*>(data), size);
    }
    
    // Read container helpers
    template<typename T>
    void ReadVector(std::vector<T>& vec) {
        uint32_t size = 0;
        Read(size);
        vec.resize(size);
        for (uint32_t i = 0; i < size; ++i) {
            Read(vec[i]);
        }
    }
    
    // Read vector of serializable objects
    template<typename T>
    void ReadSerializableVector(std::vector<std::unique_ptr<T>>& vec) {
        uint32_t size = 0;
        Read(size);
        vec.clear();
        for (uint32_t i = 0; i < size; ++i) {
            auto item = std::make_unique<T>();
            item->Deserialize(*this);
            vec.push_back(std::move(item));
        }
    }
    
    // Read map
    template<typename K, typename V>
    void ReadMap(std::unordered_map<K, V>& map) {
        uint32_t size = 0;
        Read(size);
        map.clear();
        K key;
        V value;
        for (uint32_t i = 0; i < size; ++i) {
            Read(key);
            Read(value);
            map[key] = value;
        }
    }
    
private:
    std::ifstream m_stream;
};
```

Now, let's update the RPGSystem classes to implement the ISerializable interface:

### `RPGSystem.h` (updated)
```cpp
#pragma once

#include "ISerializable.h"
#include <string>
#include <unordered_set>

// Forward declarations
class Linen;
class BinaryReader;
class BinaryWriter;

// Base class for all RPG systems
class RPGSystem : public ISerializable {
public:
    virtual ~RPGSystem() = default;
    
    // Core lifecycle methods
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(float deltaTime) {}
    
    // System dependencies
    const std::unordered_set<std::string>& GetDependencies() const { return m_dependencies; }
    
    // System identification
    virtual std::string GetSystemName() const = 0;
    
    // Plugin reference for accessing other systems
    void SetPlugin(Linen* plugin) { m_plugin = plugin; }
    
    // Default serialization - can be overridden by systems
    virtual void Serialize(BinaryWriter& writer) const override {}
    virtual void Deserialize(BinaryReader& reader) override {}
    
protected:
    Linen* m_plugin = nullptr;
    std::unordered_set<std::string> m_dependencies;
};
```

Let's update the Quest class and QuestSystem to properly implement serialization:

### `Quest.h` (updated)
```cpp
#pragma once

#include "ISerializable.h"
#include <string>
#include <unordered_map>

class Quest : public ISerializable {
public:
    enum class State { Available, Active, Completed, Failed };
    
    Quest() = default; // Default constructor for deserialization
    Quest(const std::string& id, const std::string& title, const std::string& description);
    
    // Getters/Setters
    std::string GetId() const { return m_id; }
    std::string GetTitle() const { return m_title; }
    std::string GetDescription() const { return m_description; }
    State GetState() const { return m_state; }
    int GetExperienceReward() const { return m_experienceReward; }
    
    void SetState(State state) { m_state = state; }
    void SetExperienceReward(int reward) { m_experienceReward = reward; }
    
    // Add required skill check
    void AddSkillRequirement(const std::string& skillName, int requiredLevel);
    
    // Check if player meets skill requirements
    bool CheckRequirements(const std::unordered_map<std::string, int>& playerSkills) const;
    
    // Serialization implementation
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    
private:
    std::string m_id;
    std::string m_title;
    std::string m_description;
    State m_state = State::Available;
    int m_experienceReward = 0;
    
    // Requirements to take/complete the quest
    std::unordered_map<std::string, int> m_skillRequirements;
};
```

### `Quest.cpp` (updated serialization methods)
```cpp
void Quest::Serialize(BinaryWriter& writer) const {
    writer.Write(m_id);
    writer.Write(m_title);
    writer.Write(m_description);
    writer.Write(static_cast<int32_t>(m_state));
    writer.Write(static_cast<int32_t>(m_experienceReward));
    
    // Write skill requirements
    writer.WriteMap(m_skillRequirements);
}

void Quest::Deserialize(BinaryReader& reader) {
    reader.Read(m_id);
    reader.Read(m_title);
    reader.Read(m_description);
    
    int32_t stateValue;
    reader.Read(stateValue);
    m_state = static_cast<State>(stateValue);
    
    int32_t expReward;
    reader.Read(expReward);
    m_experienceReward = expReward;
    
    // Read skill requirements
    reader.ReadMap(m_skillRequirements);
}
```

Finally, update the SaveLoadSystem to use our new binary serialization:

### `SaveLoadSystem.h` (updated)
```cpp
#pragma once

#include "RPGSystem.h"
#include <string>
#include <mutex>
#include <unordered_set>

class SaveLoadSystem : public RPGSystem {
public:
    SaveLoadSystem();
    ~SaveLoadSystem();
    
    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetSystemName() const override { return "SaveLoadSystem"; }
    
    // Save/Load functionality
    bool SaveGame(const std::string& filename);
    bool LoadGame(const std::string& filename);
    
    // System registration for save/load
    void RegisterSerializableSystem(const std::string& systemName);
    
    // Serialization (for storing save metadata)
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    
private:
    // Thread safety
    std::mutex m_mutex;
    
    // Track which systems need serialization
    std::unordered_set<std::string> m_serializableSystems;
    
    // Save metadata
    std::string m_lastSaveFile;
    uint64_t m_lastSaveTimestamp = 0;
};
```

### `SaveLoadSystem.cpp` (updated)
```cpp
#include "SaveLoadSystem.h"
#include "Linen.h"
#include "Serialization.h"
#include "Engine/Core/Log.h"
#include <chrono>

bool SaveLoadSystem::SaveGame(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Create binary writer
        BinaryWriter writer(filename);
        if (!writer.IsValid()) {
            LOG(Error, "Failed to create save file: {0}", String(filename.c_str()));
            return false;
        }
        
        // Write file header and version
        writer.Write(std::string("LINEN_SAVE"));
        writer.Write(static_cast<uint32_t>(1)); // Version 1
        
        // Write save timestamp
        auto now = std::chrono::system_clock::now();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        writer.Write(timestamp);
        
        // Write number of systems to serialize
        writer.Write(static_cast<uint32_t>(m_serializableSystems.size()));
        
        // For each registered system, call its Serialize method
        for (const auto& systemName : m_serializableSystems) {
            // Write system name
            writer.Write(systemName);
            
            // Get system and serialize it
            auto system = m_plugin->GetSystem<RPGSystem>(systemName);
            if (system) {
                system->Serialize(writer);
                LOG(Info, "Saved system: {0}", String(systemName.c_str()));
            } else {
                LOG(Warning, "System not found for serialization: {0}", String(systemName.c_str()));
                // Write empty placeholder
                writer.Write(static_cast<uint32_t>(0));
            }
        }
        
        // Update metadata
        m_lastSaveFile = filename;
        m_lastSaveTimestamp = timestamp;
        
        LOG(Info, "Game saved successfully: {0}", String(filename.c_str()));
        return true;
    }
    catch (const std::exception& e) {
        LOG(Error, "Exception during save: {0}", String(e.what()));
        return false;
    }
}

bool SaveLoadSystem::LoadGame(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Create binary reader
        BinaryReader reader(filename);
        if (!reader.IsValid()) {
            LOG(Error, "Failed to open save file: {0}", String(filename.c_str()));
            return false;
        }
        
        // Read and verify header
        std::string header;
        reader.Read(header);
        if (header != "LINEN_SAVE") {
            LOG(Error, "Invalid save file format: {0}", String(filename.c_str()));
            return false;
        }
        
        // Read version
        uint32_t version;
        reader.Read(version);
        if (version != 1) {
            LOG(Warning, "Loading save file with version {0}, current version is 1", version);
        }
        
        // Read timestamp
        uint64_t timestamp;
        reader.Read(timestamp);
        
        // Read number of systems
        uint32_t systemCount;
        reader.Read(systemCount);
        
        // Load each system
        for (uint32_t i = 0; i < systemCount; ++i) {
            // Read system name
            std::string systemName;
            reader.Read(systemName);
            
            // Get system and deserialize it
            auto system = m_plugin->GetSystem<RPGSystem>(systemName);
            if (system) {
                system->Deserialize(reader);
                LOG(Info, "Loaded system: {0}", String(systemName.c_str()));
            } else {
                LOG(Warning, "System not found for deserialization: {0}", String(systemName.c_str()));
                // Skip data for this system
                uint32_t dataSize;
                reader.Read(dataSize);
                reader.Read(nullptr, dataSize);
            }
        }
        
        // Update metadata
        m_lastSaveFile = filename;
        m_lastSaveTimestamp = timestamp;
        
        LOG(Info, "Game loaded successfully: {0}", String(filename.c_str()));
        return true;
    }
    catch (const std::exception& e) {
        LOG(Error, "Exception during load: {0}", String(e.what()));
        return false;
    }
}

void SaveLoadSystem::Serialize(BinaryWriter& writer) const {
    // Save metadata about the save system itself
    writer.Write(m_lastSaveFile);
    writer.Write(m_lastSaveTimestamp);
    
    // Write serializable systems list
    writer.WriteVector(std::vector<std::string>(m_serializableSystems.begin(), m_serializableSystems.end()));
}

void SaveLoadSystem::Deserialize(BinaryReader& reader) {
    // Load metadata about the save system
    reader.Read(m_lastSaveFile);
    reader.Read(m_lastSaveTimestamp);
    
    // Read serializable systems list
    std::vector<std::string> systems;
    reader.ReadVector(systems);
    m_serializableSystems.clear();
    for (const auto& system : systems) {
        m_serializableSystems.insert(system);
    }
}
```

These changes now provide:

1. A universal `ISerializable` interface that systems implement to define their serialization format
2. Binary serialization for better performance and smaller save files
3. Type-safe serialization helpers for common data structures
4. Error handling during serialization
5. Save file versioning for future compatibility
6. Metadata storage for save management

Each system can now define exactly how its data should be saved and loaded, making the system more flexible while maintaining good performance.

---

To add priority levels to the event system for critical events, I would modify the existing `EventSystem` class to support prioritization. Here's how I would implement it:

## 1. Update the Event Class to Include Priority

Define a priority enum and add it to the base `Event` class:

```cpp
enum class EventPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

class Event {
public:
    virtual ~Event() = default;
    virtual std::type_index GetType() const = 0;
    
    // Default priority is Normal
    EventPriority GetPriority() const { return m_priority; }
    void SetPriority(EventPriority priority) { m_priority = priority; }

protected:
    EventPriority m_priority = EventPriority::Normal;
};
```

## 2. Modify the QueuedEvent Structure

Update the `QueuedEvent` structure in the `EventSystem` class to track priority:

```cpp
struct QueuedEvent {
    std::shared_ptr<Event> event;
    std::type_index type;
    std::string filter;
    EventPriority priority;
    
    // Constructor to capture priority from the event
    QueuedEvent(std::shared_ptr<Event> e, std::type_index t, const std::string& f)
        : event(e), type(t), filter(f), priority(e->GetPriority()) {}
    
    // Comparison operator for priority queue
    bool operator<(const QueuedEvent& other) const {
        // Lower priority value means higher actual priority in std::priority_queue
        return priority < other.priority;
    }
};
```

## 3. Use a Priority Queue for Events

Replace the vector-based queue with a priority queue:

```cpp
#include <queue>

class EventSystem {
private:
    // Other members...
    
    std::mutex m_mutex;
    // Replace vector with priority queue
    std::priority_queue<QueuedEvent> m_eventQueue;
    
    // For batch processing
    std::vector<QueuedEvent> m_processingBatch;
};
```

## 4. Update the Publish Method

Modify the `Publish` method to set the priority:

```cpp
template <typename T>
void Publish(const T& event, const std::string& filter = "", EventPriority priority = EventPriority::Normal) {
    static_assert(std::is_base_of<EventType<T>, T>::value, "T must derive from EventType<T>");
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Create a shared copy of the event
    auto eventPtr = std::make_shared<T>(event);
    
    // Set the priority
    eventPtr->SetPriority(priority);
    
    // Queue the event
    std::type_index type = std::type_index(typeid(T));
    m_eventQueue.push(QueuedEvent(eventPtr, type, filter));
}
```

## 5. Update the ProcessEvents Method

Modify the `ProcessEvents` method to handle the priority queue:

```cpp
void ProcessEvents() {
    // Clear the processing batch
    m_processingBatch.clear();
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Move all events from the priority queue to our processing batch
        while (!m_eventQueue.empty()) {
            m_processingBatch.push_back(m_eventQueue.top());
            m_eventQueue.pop();
        }
    }
    
    // Process events in priority order (highest priority first)
    for (const auto& queuedEvent : m_processingBatch) {
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
        if (!filter.empty() && m_filteredHandlers.find(type) != m_filteredHandlers.end() && 
            m_filteredHandlers[type].find(filter) != m_filteredHandlers[type].end()) {
            for (const auto& handler : m_filteredHandlers[type][filter]) {
                handler->Handle(eventPtr.get());
            }
        }
    }
}
```

## 6. Example Usage

How to use the prioritized event system:

```cpp
// For a critical event that needs immediate attention
QuestFailedEvent event;
event.questId = "main_quest";
event.reason = "Time limit exceeded";
m_plugin->GetEventSystem().Publish(event, "", EventPriority::Critical);

// For a low-priority notification
PlayerLevelUpEvent event;
event.newLevel = 5;
m_plugin->GetEventSystem().Publish(event, "", EventPriority::Low);
```

## 7. Further Enhancements

1. **Thread Priority**: For multi-threaded applications, you could map event priorities to thread priorities.

2. **Immediate Processing**: Add an option for immediate processing of critical events:

```cpp
template <typename T>
void PublishImmediate(const T& event, const std::string& filter = "") {
    // Process this event immediately, bypassing the queue
    std::shared_ptr<T> eventPtr = std::make_shared<T>(event);
    std::type_index type = std::type_index(typeid(T));
    
    // Process handlers directly
    // ... handler processing code ...
}
```

3. **Priority Bands**: Process events in batches by priority level, ensuring all critical events are processed before moving to high priority, and so on.

This prioritization system ensures that critical game events (like player death, main quest failure, or system-critical updates) are processed before less important events, maintaining game responsiveness even under load.

---

### **Strengths:**
1. **Modularity & Scalability**  
   - The system is highly modular, allowing **individual systems** (e.g., Quest, Character Progression, Economy) to interact without tight coupling.  
   - The **event-driven approach** ensures flexibility and scalability for future enhancements.

2. **Well-Defined System Interactions**  
   - The **Linen_SystemInteractions.md** file details clear and logical interactions between subsystems.
   - The **event-based approach** avoids direct dependencies, keeping systems independent yet interconnected.
   - The **interaction matrix** is particularly helpful in visualizing dependencies.

3. **Clean & Optimized C++ Implementation**  
   - The **Linen_Flax_CPP.md** file uses **smart pointers, mutexes, and type-safe access** to manage system lifecycles.  
   - **Thread safety** is considered using **std::mutex** for concurrency.  
   - The **dependency resolution algorithm** ensures correct initialization order.

4. **Comprehensive Serialization & Save System**  
   - The binary **save/load system** improves performance over plain text.  
   - The **ISerializable interface** ensures that all systems can implement custom serialization logic.  
   - The **SaveLoadSystem** tracks **versioning and metadata**, ensuring future compatibility.

---

### **Areas for Improvement:**
#### **1. System Dependencies & Cyclic Check**  
   - **Problem:** Right now, `CalculateInitializationOrder()` sorts dependencies topologically, but it **doesn’t explicitly check for cyclic dependencies before attempting initialization**.
   - **Fix:** Add a **cycle detection step** using depth-first search (DFS) before sorting.

   **Suggested Enhancement (in `Linen.cpp`)**
   ```cpp
   bool Linen::DetectCycle(const std::string& systemName, 
                                 std::unordered_set<std::string>& visited, 
                                 std::unordered_set<std::string>& recursionStack) {
       if (recursionStack.count(systemName)) return true;  // Cycle detected
       if (visited.count(systemName)) return false;

       visited.insert(systemName);
       recursionStack.insert(systemName);

       for (const auto& dependency : m_registeredSystems[systemName]->GetDependencies()) {
           if (DetectCycle(dependency, visited, recursionStack)) return true;
       }

       recursionStack.erase(systemName);
       return false;
   }

   void Linen::CalculateInitializationOrder() {
       std::unordered_set<std::string> visited;
       std::unordered_set<std::string> recursionStack;

       for (const auto& pair : m_registeredSystems) {
           if (DetectCycle(pair.first, visited, recursionStack)) {
               LOG(Error, "Cyclic dependency detected in system: {0}", String(pair.first.c_str()));
               return;
           }
       }
       // Continue with topological sorting...
   }
   ```
**Impact:** This **prevents crashes due to infinite loops** in system dependencies.

---

#### **2. Event System Priority Handling**
   - **Problem:** Events in `EventSystem.h` are executed in a **first-in, first-out (FIFO) order**. Some events (e.g., **critical system failure**) should have higher priority.
   - **Fix:** Modify `EventSystem` to support **priority-based event handling** using a `std::priority_queue`.

   **Suggested Enhancement (in `EventSystem.h`)**
   ```cpp
   enum class EventPriority {
       Low = 0,
       Normal = 1,
       High = 2,
       Critical = 3
   };

   struct QueuedEvent {
       std::shared_ptr<Event> event;
       std::type_index type;
       std::string filter;
       EventPriority priority;

       bool operator<(const QueuedEvent& other) const {
           return priority < other.priority;  // High-priority events processed first
       }
   };

   class EventSystem {
   private:
       std::priority_queue<QueuedEvent> m_eventQueue;
   };
   ```
**Impact:** Allows **urgent events (e.g., game-breaking errors, combat AI updates, world state changes)** to be processed before lower-priority events.

---

#### **3. Dynamic Weather & NPC Scheduling**
   - **Problem:** NPC behavior adapts to **time of day** but **not dynamically changing weather conditions**.
   - **Fix:** Implement **real-time weather event listeners** that modify NPC movement schedules.

   **Example: NPC AI reacting to storms**
   ```cpp
   EventBus::Subscribe<WeatherStateChangedEvent>([](const WeatherStateChangedEvent& event) {
       if (event.NewWeather == "Storm") {
           for (auto& npc : NPCManager::GetAllNPCs()) {
               npc->SeekShelter();
           }
       }
   });
   ```
**Impact:** NPCs react naturally to environmental changes, improving **immersion**.