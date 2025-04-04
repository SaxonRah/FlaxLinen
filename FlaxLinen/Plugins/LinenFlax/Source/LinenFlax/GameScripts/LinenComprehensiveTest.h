// v LinenComprehensiveTest.h
#pragma once
#include "Engine/Scripting/Script.h"

// Forward declarations
class LinenFlax;

API_CLASS() class LINENFLAX_API LinenComprehensiveTest : public Script
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE(LinenComprehensiveTest);

public:
    // LinenComprehensiveTest(const SpawnParams& params);
    
    // Script interface
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

private:
    // Plugin reference
    LinenFlax* m_plugin = nullptr;
    
    // Setup state
    bool _isSetupComplete;
    float _simulationTimePassed;
    float _simulationInterval;
    int _currentScenario;
    
    // Scenario flag states
    bool _tradingQuestComplete = false;
    bool _triggeredStormEvent = false;
    
    // Setup methods
    void SubscribeToEvents();
    void SetupSystems();
    void SetupCharacterProgression();
    void SetupTimeSystem();
    void SetupRelationshipsAndFactions();
    void SetupCrimeSystem();
    void SetupEconomySystem();
    void SetupWeatherSystem();
    void SetupWorldProgressionSystem();
    void SetupQuestSystem();
    
    // Scenario methods
    void RunNextScenario();
    void RunForestExplorationScenario();
    void RunTradingScenario();
    void RunBanditScenario();
    void RunWeatherAndCrimeScenario();
    void RunWorldSimulationScenario();
};
// ^ LinenComprehensiveTest.h