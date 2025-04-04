// v LinenFlax.cpp
#include "LinenFlax.h"
#include "Engine/Core/Log.h"
#include "LinenSystemIncludes.h" // Include all systems


LinenFlax::LinenFlax(const SpawnParams& params)
    : GamePlugin(params)
{
    _description.Name = TEXT("LinenFlax");
#if USE_EDITOR
    _description.Category = TEXT("Gameplay");
    _description.Description = TEXT("LinenFlax plugin");
    _description.Author = TEXT("ParabolicLabs");
    _description.RepositoryUrl = TEXT("");
#endif
    _description.Version = Version(1, 0, 0);
}

void LinenFlax::Initialize()
{
    GamePlugin::Initialize();

    LOG(Info, "LinenFlax::Initialize : ran");

    // Set plugin references first
    TestSystem::GetInstance()->SetPlugin(this);
    CharacterProgressionSystem::GetInstance()->SetPlugin(this);
    QuestSystem::GetInstance()->SetPlugin(this);
    SaveLoadSystem::GetInstance()->SetPlugin(this);
    TimeSystem::GetInstance()->SetPlugin(this);

    EconomySystem::GetInstance()->SetPlugin(this);
    WeatherSystem::GetInstance()->SetPlugin(this);
    WorldProgressionSystem::GetInstance()->SetPlugin(this);

    RelationshipSystem::GetInstance()->SetPlugin(this);
    FactionSystem::GetInstance()->SetPlugin(this);
    CrimeSystem::GetInstance()->SetPlugin(this);

    // Then initialize systems in dependency order
    TestSystem::GetInstance()->Initialize();
    CharacterProgressionSystem::GetInstance()->Initialize();
    QuestSystem::GetInstance()->Initialize();
    SaveLoadSystem::GetInstance()->Initialize();
    TimeSystem::GetInstance()->Initialize();

    EconomySystem::GetInstance()->Initialize();
    WeatherSystem::GetInstance()->Initialize();
    WorldProgressionSystem::GetInstance()->Initialize();

    RelationshipSystem::GetInstance()->Initialize();
    FactionSystem::GetInstance()->Initialize();
    CrimeSystem::GetInstance()->Initialize();

    LOG(Info, "All LinenFlax RPG Systems initialized");
}

void LinenFlax::Deinitialize()
{
    LOG(Info, "LinenFlax::Deinitialize : ran");

    // Shutdown systems in reverse order
    CrimeSystem::GetInstance()->Shutdown();
    FactionSystem::GetInstance()->Shutdown();
    RelationshipSystem::GetInstance()->Shutdown();

    WorldProgressionSystem::GetInstance()->Shutdown();
    WeatherSystem::GetInstance()->Shutdown();
    EconomySystem::GetInstance()->Shutdown();

    TimeSystem::GetInstance()->Shutdown();
    SaveLoadSystem::GetInstance()->Shutdown();
    QuestSystem::GetInstance()->Shutdown();
    CharacterProgressionSystem::GetInstance()->Shutdown();
    TestSystem::GetInstance()->Shutdown();

    LOG(Info, "LinenFlax Plugin Deinitialized.");
    GamePlugin::Deinitialize();
}

void LinenFlax::Update(float deltaTime)
{
    // Update all singleton systems
    TestSystem::GetInstance()->Update(deltaTime);
    CharacterProgressionSystem::GetInstance()->Update(deltaTime);
    QuestSystem::GetInstance()->Update(deltaTime);
    SaveLoadSystem::GetInstance()->Update(deltaTime);
    TimeSystem::GetInstance()->Update(deltaTime);

    EconomySystem::GetInstance()->Update(deltaTime);
    WeatherSystem::GetInstance()->Update(deltaTime);
    WorldProgressionSystem::GetInstance()->Update(deltaTime);

    RelationshipSystem::GetInstance()->Update(deltaTime);
    FactionSystem::GetInstance()->Update(deltaTime);
    CrimeSystem::GetInstance()->Update(deltaTime);

    // Process events after all systems have updated
    m_eventSystem.ProcessEvents();
}

bool LinenFlax::DetectCycle(const std::string& systemName,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& recursionStack)
{
    if (recursionStack.count(systemName))
        return true; // Cycle detected
    if (visited.count(systemName))
        return false;

    visited.insert(systemName);
    recursionStack.insert(systemName);

    for (const auto& dependency : m_registeredSystems[systemName]->GetDependencies()) {
        if (DetectCycle(dependency, visited, recursionStack))
            return true;
    }

    recursionStack.erase(systemName);
    return false;
}

void LinenFlax::CalculateInitializationOrder()
{
    m_initializationOrder.clear();

    // Topological sort of systems based on dependencies
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> inProgress;
    std::unordered_set<std::string> recursionStack;

    for (const auto& pair : m_registeredSystems) {
        if (DetectCycle(pair.first, visited, recursionStack)) {
            LOG(Error, "Cyclic dependency detected in system: {0}", String(pair.first.c_str()));
            return;
        }
    }

    // Reset visited set for the actual traversal
    visited.clear();

    // Visit all registered systems
    for (const auto& pair : m_registeredSystems) {
        if (visited.find(pair.first) == visited.end()) {
            VisitSystem(pair.first, visited, inProgress);
        }
    }
}

// Implement the member function
void LinenFlax::VisitSystem(const std::string& systemName,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& inProgress)
{
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
            VisitSystem(dep, visited, inProgress);
        }
    }

    inProgress.erase(systemName);
    visited.insert(systemName);
    m_initializationOrder.push_back(systemName);
}
// ^ LinenFlax.cpp