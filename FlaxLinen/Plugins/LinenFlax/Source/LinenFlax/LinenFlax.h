// v LinenFlax.h
#pragma once

#include "Engine/Scripting/Plugins/GamePlugin.h"
#include "Engine/Core/Log.h"
#include "EventSystem.h"
#include "RPGSystem.h"

#include <string>
#include <unordered_set> 
#include <unordered_map>
#include <memory>
#include <vector>
#include <typeindex>
#include <functional>

// Forward declarations
class RPGSystem;
class TestSystem;
class CharacterProgressionSystem;
class QuestSystem;
class SaveLoadSystem;

class CrimeSystem;
class EconomySystem;
class FactionSystem;
class RelationshipSystem;
class WeatherSystem;
class WorldProgressionSystem;

class BinaryReader;
class BinaryWriter;
class TextWriter;
class TextReader;

// Example test system
class TestSystem : public RPGSystem {
private:
    int _testValue;

    // Private constructor to prevent direct instantiation
    TestSystem() : _testValue(0) {}

public:
    // Delete copy constructor and assignment operator
    TestSystem(const TestSystem&) = delete;
    TestSystem& operator=(const TestSystem&) = delete;

    void Initialize() override {
        LOG(Info, "TestSystem Initialized");
        _testValue = 0;
    }
    
    void Shutdown() override {
        LOG(Info, "TestSystem Shutdown");
    }

    ~TestSystem() {
        Destroy();
        // Shutdown();
    }

    // Implement required abstract methods
    std::string GetName() const override { return "TestSystem"; }
    
    void Serialize(BinaryWriter& writer) const override {
        writer.Write(_testValue);
        LOG(Info, "TestSystem serialized with value: {0}", _testValue);
    }
    
    void Deserialize(BinaryReader& reader) override {
        reader.Read(_testValue);
        LOG(Info, "TestSystem deserialized with value: {0}", _testValue);
    }
    
    void SerializeToText(TextWriter& writer) const {
        writer.Write("testValue", _testValue);
        LOG(Info, "TestSystem serialized to text with value: {0}", _testValue);
    }
    
    void DeserializeFromText(TextReader& reader) {
        reader.Read("testValue", _testValue);
        LOG(Info, "TestSystem deserialized from text with value: {0}", _testValue);
    }
    
    // GetInstance method
    static TestSystem* GetInstance() {
        // Thread-safe in C++11 and beyond
        static TestSystem* instance = new TestSystem();
        return instance;
    }

    // Cleanup method (important!)
    static void Destroy() {
        static TestSystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }
    
    void Update(float deltaTime) override {}

    bool AddValue(int value) {
        LOG(Info, "TestSystem::AddValue : starting with value: {0}", value);
        _testValue = value;
        LOG(Info, "TestSystem::AddValue : set value to: {0}", _testValue);
        return true;
    }
    
    int GetValue() const {
        LOG(Info, "TestSystem::GetValue : returning: {0}", _testValue);
        return _testValue;
    }
};

API_CLASS(Namespace="ParabolicLabs") class LINENFLAX_API LinenFlax : public GamePlugin
{
    
DECLARE_SCRIPTING_TYPE(LinenFlax);

public:
        
    // Delete copy operations
    LinenFlax(const LinenFlax&) = delete;
    LinenFlax& operator=(const LinenFlax&) = delete;

    // Core plugin lifecycle
    void Initialize() override;
    void Deinitialize() override;

    /// <summary>
    /// Updates all systems and processes events
    /// </summary>
    void Update(float deltaTime);

    // System management
    template <typename T>
    bool RegisterSystem();
    
    template <typename T>
    bool LoadSystem();
    
    template <typename T>
    bool UnloadSystem();
    
    // Thread-safe event system access
    EventSystem& GetEventSystem() { return m_eventSystem; }

    /// <summary>
    /// Gets a specific RPG system by type
    /// </summary>
    template <typename T>
    T* GetSystem();
    
private:
    // Determines correct initialization order based on dependencies

    void VisitSystem(const std::string& systemName, 
        std::unordered_set<std::string>& visited,
        std::unordered_set<std::string>& inProgress);
    bool DetectCycle(const std::string& systemName, 
        std::unordered_set<std::string>& visited, 
        std::unordered_set<std::string>& recursionStack);
    void CalculateInitializationOrder();

    // Organizes systems by dependencies
    std::vector<std::string> m_initializationOrder;

    // Maps for system management
    std::unordered_map<std::string, std::unique_ptr<RPGSystem>> m_registeredSystems;
    std::unordered_map<std::string, RPGSystem*> m_activeSystems;
    std::unordered_map<std::type_index, std::string> m_typeToName;

    // Centralized event system
    EventSystem m_eventSystem;
};

// Template implementations
template <typename T>
T* LinenFlax::GetSystem() {
    // C++17 compatible implementation
    if constexpr (std::is_same<T, TestSystem>::value) {
        return TestSystem::GetInstance();
    }
    else if constexpr (std::is_same<T, CharacterProgressionSystem>::value) {
        return CharacterProgressionSystem::GetInstance();
    }
    else if constexpr (std::is_same<T, QuestSystem>::value) {
        return QuestSystem::GetInstance();
    }
    else if constexpr (std::is_same<T, EconomySystem>::value) {
        return EconomySystem::GetInstance();
    }
    else if constexpr (std::is_same<T, WeatherSystem>::value) {
        return WeatherSystem::GetInstance();
    }
    else if constexpr (std::is_same<T, WorldProgressionSystem>::value) {
        return WorldProgressionSystem::GetInstance();
    }
    else if constexpr (std::is_same<T, RelationshipSystem>::value) {
        return RelationshipSystem::GetInstance();
    }
    else if constexpr (std::is_same<T, FactionSystem>::value) {
        return FactionSystem::GetInstance();
    }
    else if constexpr (std::is_same<T, CrimeSystem>::value) {
        return CrimeSystem::GetInstance();
    }
    else if constexpr (std::is_same<T, SaveLoadSystem>::value) {
        return SaveLoadSystem::GetInstance();
    }
    else if constexpr (std::is_same<T, TimeSystem>::value) {
        return TimeSystem::GetInstance();
    }
    
    LOG(Warning, "LinenFlax::GetSystem : No matching system found for type {0}", String(typeid(T).name()));
    return nullptr;
}

template <typename T>
bool LinenFlax::RegisterSystem() {
    static_assert(std::is_base_of<RPGSystem, T>::value, "T must derive from RPGSystem");
    
    auto system = std::make_unique<T>();
    std::string systemName = system->GetName();
    
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
bool LinenFlax::LoadSystem() {
    static_assert(std::is_base_of<RPGSystem, T>::value, "T must derive from RPGSystem");
   
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
bool LinenFlax::UnloadSystem() {
    static_assert(std::is_base_of<RPGSystem, T>::value, "T must derive from RPGSystem");
    
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
// ^ LinenFlax.h