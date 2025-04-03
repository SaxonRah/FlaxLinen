// v LinenComprehensiveTest.cpp
#include "LinenTest.h"
#include "LinenFlax.h"
#include "LinenSystemIncludes.h"
#include "Engine/Core/Log.h"
#include "Engine/Scripting/Plugins/PluginManager.h"

// The main script for comprehensive RPG system testing and integration
LinenComprehensiveTest::LinenComprehensiveTest(const SpawnParams& params)
    : Script(params)
{
    _tickUpdate = true;
    _simulationTimePassed = 0.0f;
    _simulationInterval = 2.0f;  // Run simulation steps every 2 seconds
    _isSetupComplete = false;
    _currentScenario = 0;
}

void LinenComprehensiveTest::OnEnable()
{
    LOG(Info, "LinenComprehensiveTest::OnEnable : Starting Comprehensive RPG System Test");
    
    // Get the LinenFlax plugin instance
    m_plugin = PluginManager::GetPlugin<LinenFlax>();
    if (!m_plugin || typeid(*m_plugin) != typeid(LinenFlax)) {
        LOG(Error, "LinenComprehensiveTest: Failed to get LinenFlax plugin instance");
        return;
    }
    
    // Subscribe to various events to demonstrate cross-system interactions
    SubscribeToEvents();
    
    // Setup will be performed on the first update to ensure all systems are initialized
}

void LinenComprehensiveTest::OnDisable()
{
    LOG(Info, "LinenComprehensiveTest::OnDisable : Shutting down");
}

void LinenComprehensiveTest::OnUpdate()
{
    if (!m_plugin) return;
    
    // Perform initial setup on first update
    if (!_isSetupComplete) {
        SetupSystems();
        _isSetupComplete = true;
    }
    
    // Update simulation timer
    _simulationTimePassed += GetWorld()->GetDeltaTime();
    
    // Run simulation steps at intervals
    if (_simulationTimePassed >= _simulationInterval) {
        _simulationTimePassed = 0.0f;
        RunNextScenario();
    }
}

void LinenComprehensiveTest::SubscribeToEvents()
{
    // Time events
    m_plugin->GetEventSystem().Subscribe<HourChangedEvent>(
        [this](const HourChangedEvent& event) {
            LOG(Info, "EVENT: Hour changed to {0}, isDaytime: {1}", 
                event.newHour, String(event.isDayTime ? "Yes" : "No"));
            
            // Link time and weather - change weather chances based on time of day
            auto* weatherSystem = m_plugin->GetSystem<WeatherSystem>();
            if (weatherSystem && !weatherSystem->IsWeatherLocked() && event.newHour % 6 == 0) {
                // Every 6 hours, consider changing weather
                std::string weatherType = "Clear";
                
                if (event.newHour >= 18 || event.newHour < 6) { // Night
                    // More chance of fog at night
                    weatherType = rand() % 10 < 7 ? "Clear" : "Foggy";
                } else { // Day
                    int weatherRoll = rand() % 10;
                    if (weatherRoll < 5) weatherType = "Clear";
                    else if (weatherRoll < 7) weatherType = "Cloudy";
                    else if (weatherRoll < 9) weatherType = "Rain";
                    else weatherType = "Thunderstorm";
                }
                
                WeatherCondition condition = WeatherCondition::Clear;
                if (weatherType == "Foggy") condition = WeatherCondition::Foggy;
                else if (weatherType == "Cloudy") condition = WeatherCondition::Cloudy;
                else if (weatherType == "Rain") condition = WeatherCondition::Rain;
                else if (weatherType == "Thunderstorm") condition = WeatherCondition::Thunderstorm;
                
                weatherSystem->ForceWeatherChange(condition, 0.7f, 2.0f);
            }
        });
    
    // Weather events
    m_plugin->GetEventSystem().Subscribe<WeatherChangedEvent>(
        [this](const WeatherChangedEvent& event) {
            LOG(Info, "EVENT: Weather changed from {0} to {1}, intensity: {2:0.2f}", 
                String(event.previousWeather.c_str()), String(event.newWeather.c_str()), event.intensity);
            
            // Link weather to world state
            auto* worldSystem = m_plugin->GetSystem<WorldProgressionSystem>();
            if (worldSystem) {
                // Apply weather effects to regions
                for (const auto& regionId : {"town", "forest", "mountain"}) {
                    WorldRegion* region = worldSystem->GetRegion(regionId);
                    if (region) {
                        // Dangerous weather affects region stability and danger
                        if (event.isDangerous) {
                            region->SetStability(std::max(0.0f, region->GetStability() - 0.1f));
                            region->SetDanger(std::min(1.0f, region->GetDanger() + 0.1f));
                            
                            LOG(Info, "Dangerous weather affecting region {0}", String(regionId));
                        }
                        
                        // Thunderstorms can trigger world events
                        if (event.newWeather == "Thunderstorm" && !_triggeredStormEvent) {
                            _triggeredStormEvent = true;
                            worldSystem->TriggerWorldEvent("storm_damage", regionId);
                        }
                    }
                }
            }
        });
    
    // Quest events
    m_plugin->GetEventSystem().Subscribe<QuestCompletedEvent>(
        [this](const QuestCompletedEvent& event) {
            LOG(Info, "EVENT: Quest completed: {0}, XP gained: {1}", 
                String(event.questTitle.c_str()), event.experienceGained);
            
            // Link quests to character progression
            auto* progressSystem = m_plugin->GetSystem<CharacterProgressionSystem>();
            if (progressSystem) {
                // Award additional skills based on quest
                if (event.questId == "explore_forest") {
                    progressSystem->IncreaseSkill("survival", 2);
                    LOG(Info, "Increased survival skill from forest exploration quest");
                }
                else if (event.questId == "bandit_quest") {
                    progressSystem->IncreaseSkill("combat", 3);
                    LOG(Info, "Increased combat skill from bandit quest");
                }
            }
            
            // Link quests to faction reputation
            auto* factionSystem = m_plugin->GetSystem<FactionSystem>();
            if (factionSystem) {
                if (event.questId == "town_quest") {
                    factionSystem->ModifyReputation("player", "townspeople", 50);
                    LOG(Info, "Increased reputation with townspeople");
                }
                else if (event.questId == "bandit_quest") {
                    factionSystem->ModifyReputation("player", "bandits", -75);
                    factionSystem->ModifyReputation("player", "townspeople", 25);
                    LOG(Info, "Changed reputation with factions due to bandit quest");
                }
            }
        });
    
    // World events
    m_plugin->GetEventSystem().Subscribe<WorldEventTriggeredEvent>(
        [this](const WorldEventTriggeredEvent& event) {
            LOG(Info, "EVENT: World event triggered: {0} in {1}", 
                String(event.eventName.c_str()), String(event.regionId.c_str()));
            
            // Link world events to economy
            auto* economySystem = m_plugin->GetSystem<EconomySystem>();
            if (economySystem) {
                // Different economic impacts based on event type
                if (event.eventId == "bandit_raid") {
                    economySystem->TriggerEconomicEvent("trade_disruption");
                    LOG(Info, "Bandit raid caused trade disruption");
                }
                else if (event.eventId == "good_harvest") {
                    economySystem->TriggerEconomicEvent("harvest");
                    LOG(Info, "Good harvest boosted economy");
                }
                else if (event.eventId == "storm_damage") {
                    // Reduce stock of items in the affected region's market
                    Market* market = economySystem->GetMarket(event.regionId + "_market");
                    if (market) {
                        market->ModifyItemStock("bread", -5);
                        market->ModifyItemStock("wood", -8);
                        LOG(Info, "Storm reduced goods availability in {0}", 
                            String(event.regionId.c_str()));
                    }
                }
            }
            
            // Create new quests based on world events
            auto* questSystem = m_plugin->GetSystem<QuestSystem>();
            if (questSystem && event.eventId == "bandit_raid") {
                questSystem->AddQuest("bandit_quest", "Bandit Problem", 
                    "Deal with the bandits that have been raiding the region.");
                LOG(Info, "Added new bandit quest based on world event");
            }
        });
    
    // Crime events
    m_plugin->GetEventSystem().Subscribe<CrimeCommittedEvent>(
        [this](const CrimeCommittedEvent& event) {
            LOG(Info, "EVENT: Crime committed: {0} by {1} against {2} in {3}", 
                String(event.crimeType.c_str()), String(event.perpetratorId.c_str()),
                String(event.victimId.c_str()), String(event.regionId.c_str()));
            
            // Link crimes to world state
            auto* worldSystem = m_plugin->GetSystem<WorldProgressionSystem>();
            if (worldSystem) {
                WorldRegion* region = worldSystem->GetRegion(event.regionId);
                if (region) {
                    // Crimes reduce stability and increase danger
                    region->SetStability(std::max(0.0f, region->GetStability() - 0.05f));
                    region->SetDanger(std::min(1.0f, region->GetDanger() + 0.05f));
                    
                    // Serious crimes have greater impact
                    if (event.crimeType == "Murder" || event.crimeType == "Assault") {
                        region->SetStability(std::max(0.0f, region->GetStability() - 0.1f));
                        region->SetDanger(std::min(1.0f, region->GetDanger() + 0.1f));
                    }
                    
                    LOG(Info, "Crime affected {0} region's stability and danger", 
                        String(event.regionId.c_str()));
                }
            }
            
            // Link crimes to relationships
            auto* relationshipSystem = m_plugin->GetSystem<RelationshipSystem>();
            if (relationshipSystem) {
                if (event.victimId != "" && event.witnessed) {
                    relationshipSystem->ModifyRelationship(event.victimId, event.perpetratorId, -20);
                    LOG(Info, "Crime worsened relationship between victim and perpetrator");
                    
                    // Witnesses also think less of the criminal
                    for (const auto& witnessId : event.witnessIds) {
                        relationshipSystem->ModifyRelationship(witnessId, event.perpetratorId, -10);
                    }
                }
            }
        });
    
    // Economic events
    m_plugin->GetEventSystem().Subscribe<TradeCompletedEvent>(
        [this](const TradeCompletedEvent& event) {
            LOG(Info, "EVENT: Trade completed: {0}x {1} {2} for {3} gold", 
                event.quantity, String(event.itemId.c_str()), 
                String(event.playerIsBuyer ? "bought" : "sold"), event.totalValue);
            
            // Link trading to character progression
            auto* progressSystem = m_plugin->GetSystem<CharacterProgressionSystem>();
            if (progressSystem) {
                // Trading improves commerce skill
                progressSystem->IncreaseSkill("commerce", 1);
                
                // Gain experience from significant trades
                if (event.totalValue > 50.0f) {
                    progressSystem->GainExperience(static_cast<int>(event.totalValue * 0.1f));
                    LOG(Info, "Gained XP from valuable trade");
                }
            }
            
            // Link trading to quest progress
            auto* questSystem = m_plugin->GetSystem<QuestSystem>();
            if (questSystem && !_tradingQuestComplete) {
                Quest* quest = questSystem->GetQuest("trading_quest");
                if (quest && quest->GetState() == QuestState::Active) {
                    if (event.totalValue >= 100.0f) {
                        questSystem->CompleteQuest("trading_quest");
                        _tradingQuestComplete = true;
                        LOG(Info, "Completed trading quest from successful trade");
                    }
                }
            }
        });
}

void LinenComprehensiveTest::SetupSystems()
{
    LOG(Info, "LinenComprehensiveTest: Setting up RPG systems");
    
    // Setup Character Progression System
    SetupCharacterProgression();
    
    // Setup Time System
    SetupTimeSystem();
    
    // Setup Relationship and Faction Systems
    SetupRelationshipsAndFactions();
    
    // Setup Crime System
    SetupCrimeSystem();
    
    // Setup Economy System
    SetupEconomySystem();
    
    // Setup Weather System
    SetupWeatherSystem();
    
    // Setup World Progression System
    SetupWorldProgressionSystem();
    
    // Setup Quest System
    SetupQuestSystem();
    
    LOG(Info, "LinenComprehensiveTest: All systems initialized and ready");
}

void LinenComprehensiveTest::SetupCharacterProgression()
{
    auto* system = m_plugin->GetSystem<CharacterProgressionSystem>();
    if (!system) return;
    
    // Add skills relevant to RPG gameplay
    system->AddSkill("strength", "Strength", "Physical power");
    system->AddSkill("intelligence", "Intelligence", "Mental acuity");
    system->AddSkill("agility", "Agility", "Physical dexterity");
    system->AddSkill("survival", "Survival", "Wilderness survival skills");
    system->AddSkill("commerce", "Commerce", "Trading and bargaining");
    system->AddSkill("combat", "Combat", "Fighting ability");
    
    // Set initial skill levels
    system->IncreaseSkill("strength", 5);
    system->IncreaseSkill("intelligence", 5);
    system->IncreaseSkill("agility", 5);
    system->IncreaseSkill("survival", 3);
    system->IncreaseSkill("commerce", 2);
    system->IncreaseSkill("combat", 4);
    
    // Set initial XP
    system->GainExperience(100);
    
    LOG(Info, "Character progression system setup complete");
}

void LinenComprehensiveTest::SetupTimeSystem()
{
    auto* system = m_plugin->GetSystem<TimeSystem>();
    if (!system) return;
    
    // Configure time system
    system->SetTimeScale(30.0f);  // Speed up time for testing
    system->SetHour(8);  // Start at 8 AM
    system->SetDay(1);
    system->SetMonth(1);  // Start in Spring
    
    LOG(Info, "Time system setup complete - Starting at {0} on day {1}/{2}/{3}", 
        String(system->GetFormattedTime().c_str()), 
        system->GetDay(), system->GetMonth(), system->GetYear());
}

void LinenComprehensiveTest::SetupRelationshipsAndFactions()
{
    auto* relationshipSystem = m_plugin->GetSystem<RelationshipSystem>();
    auto* factionSystem = m_plugin->GetSystem<FactionSystem>();
    
    if (!relationshipSystem || !factionSystem) return;
    
    // Register characters
    relationshipSystem->RegisterCharacter("player", "Player Character");
    relationshipSystem->RegisterCharacter("mayor", "Town Mayor");
    relationshipSystem->RegisterCharacter("merchant", "Market Merchant");
    relationshipSystem->RegisterCharacter("bandit_leader", "Bandit Leader");
    relationshipSystem->RegisterCharacter("guard_captain", "Guard Captain");
    
    // Set initial relationships
    relationshipSystem->SetRelationship("player", "mayor", 25);     // Friendly
    relationshipSystem->SetRelationship("player", "merchant", 20);  // Friendly
    relationshipSystem->SetRelationship("player", "bandit_leader", -50);  // Unfriendly
    relationshipSystem->SetRelationship("player", "guard_captain", 10);  // Neutral
    
    relationshipSystem->SetRelationship("mayor", "guard_captain", 75);  // Allied
    relationshipSystem->SetRelationship("mayor", "bandit_leader", -80);  // Hostile
    relationshipSystem->SetRelationship("guard_captain", "bandit_leader", -90);  // Hostile
    
    // Create factions
    factionSystem->CreateFaction("townspeople", "Townspeople", "Citizens of the town");
    factionSystem->CreateFaction("merchants", "Merchants Guild", "Association of merchants");
    factionSystem->CreateFaction("bandits", "Forest Bandits", "Outlaws hiding in the forest");
    factionSystem->CreateFaction("town_guard", "Town Guard", "Protectors of the town");
    
    // Set faction relationships
    factionSystem->SetFactionRelationship("townspeople", "merchants", 50);
    factionSystem->SetFactionRelationship("townspeople", "town_guard", 75);
    factionSystem->SetFactionRelationship("townspeople", "bandits", -75);
    factionSystem->SetFactionRelationship("merchants", "town_guard", 60);
    factionSystem->SetFactionRelationship("merchants", "bandits", -50);
    factionSystem->SetFactionRelationship("town_guard", "bandits", -90);
    
    // Set player reputation with factions
    factionSystem->SetReputation("player", "townspeople", 25);
    factionSystem->SetReputation("player", "merchants", 15);
    factionSystem->SetReputation("player", "town_guard", 10);
    factionSystem->SetReputation("player", "bandits", -10);
    
    LOG(Info, "Relationship and faction systems setup complete");
}

void LinenComprehensiveTest::SetupCrimeSystem()
{
    auto* system = m_plugin->GetSystem<CrimeSystem>();
    if (!system) return;
    
    // Register regions
    system->RegisterRegion("town", "Town");
    system->RegisterRegion("forest", "Forest");
    system->RegisterRegion("mountain", "Mountain");
    
    // Register guard factions
    system->RegisterGuardFaction("town", "town_guard");
    
    // Set crime expiration time
    system->SetCrimeExpirationTime(48);  // 48 game hours
    
    LOG(Info, "Crime system setup complete");
}

void LinenComprehensiveTest::SetupEconomySystem()
{
    auto* system = m_plugin->GetSystem<EconomySystem>();
    if (!system) return;
    
    // Create markets
    system->AddMarket("town_market", "Town Market");
    system->AddMarket("forest_market", "Forest Trading Post");
    system->AddMarket("mountain_market", "Mountain Outpost");
    
    // Set market specializations and status
    Market* townMarket = system->GetMarket("town_market");
    Market* forestMarket = system->GetMarket("forest_market");
    Market* mountainMarket = system->GetMarket("mountain_market");
    
    if (townMarket && forestMarket && mountainMarket) {
        townMarket->SetSpecialization(ItemCategory::Food);
        townMarket->SetStatus(MarketStatus::Stable);
        
        forestMarket->SetSpecialization(ItemCategory::Materials);
        forestMarket->SetStatus(MarketStatus::Struggling);
        
        mountainMarket->SetSpecialization(ItemCategory::Tools);
        mountainMarket->SetStatus(MarketStatus::Prospering);
    }
    
    // Register items
    system->RegisterItem("bread", "Bread", 2.0f, ItemCategory::Food);
    system->RegisterItem("apple", "Apple", 1.0f, ItemCategory::Food);
    system->RegisterItem("cheese", "Cheese", 3.0f, ItemCategory::Food);
    system->RegisterItem("wood", "Wood", 5.0f, ItemCategory::Materials);
    system->RegisterItem("iron", "Iron", 10.0f, ItemCategory::Materials);
    system->RegisterItem("axe", "Axe", 20.0f, ItemCategory::Tools);
    system->RegisterItem("sword", "Sword", 40.0f, ItemCategory::Weapons);
    system->RegisterItem("leather", "Leather", 15.0f, ItemCategory::Materials);
    system->RegisterItem("potion", "Healing Potion", 25.0f, ItemCategory::Magic);
    
    // Set initial stock
    if (townMarket) {
        townMarket->SetItemStock("bread", 20);
        townMarket->SetItemStock("apple", 30);
        townMarket->SetItemStock("cheese", 15);
        townMarket->SetItemStock("axe", 3);
        townMarket->SetItemStock("sword", 2);
    }
    
    if (forestMarket) {
        forestMarket->SetItemStock("wood", 50);
        forestMarket->SetItemStock("leather", 20);
        forestMarket->SetItemStock("bread", 5);
        forestMarket->SetItemStock("apple", 10);
    }
    
    if (mountainMarket) {
        mountainMarket->SetItemStock("iron", 30);
        mountainMarket->SetItemStock("axe", 10);
        mountainMarket->SetItemStock("sword", 5);
        mountainMarket->SetItemStock("potion", 8);
    }
    
    LOG(Info, "Economy system setup complete");
}

void LinenComprehensiveTest::SetupWeatherSystem()
{
    auto* system = m_plugin->GetSystem<WeatherSystem>();
    if (!system) return;
    
    // Configure weather system
    system->SetWeatherTransitionSpeed(0.5f);  // Faster transitions for testing
    
    // Start with clear weather
    system->ForceWeatherChange(WeatherCondition::Clear, 0.5f, 1.0f);
    
    LOG(Info, "Weather system setup complete");
}

void LinenComprehensiveTest::SetupWorldProgressionSystem()
{
    auto* system = m_plugin->GetSystem<WorldProgressionSystem>();
    if (!system) return;
    
    // Create regions (matching crime system regions)
    system->AddRegion("town", "Town");
    system->AddRegion("forest", "Forest");
    system->AddRegion("mountain", "Mountain");
    
    // Connect regions
    system->ConnectRegions("town", "forest");
    system->ConnectRegions("forest", "mountain");
    
    // Add factions (matching faction system)
    system->AddFaction("townspeople", "Townspeople");
    system->AddFaction("merchants", "Merchants Guild");
    system->AddFaction("bandits", "Forest Bandits");
    system->AddFaction("town_guard", "Town Guard");
    
    // Set faction relationship values (matching faction system)
    system->SetFactionRelationship("townspeople", "merchants", 0.5f);
    system->SetFactionRelationship("townspeople", "town_guard", 0.75f);
    system->SetFactionRelationship("townspeople", "bandits", -0.75f);
    system->SetFactionRelationship("merchants", "town_guard", 0.6f);
    system->SetFactionRelationship("merchants", "bandits", -0.5f);
    system->SetFactionRelationship("town_guard", "bandits", -0.9f);
    
    // Set up region states
    WorldRegion* townRegion = system->GetRegion("town");
    WorldRegion* forestRegion = system->GetRegion("forest");
    WorldRegion* mountainRegion = system->GetRegion("mountain");
    
    if (townRegion && forestRegion && mountainRegion) {
        // Set faction presence in each region
        townRegion->AddFactionPresence("townspeople", 0.8f);
        townRegion->AddFactionPresence("merchants", 0.6f);
        townRegion->AddFactionPresence("town_guard", 0.7f);
        townRegion->AddFactionPresence("bandits", 0.1f);
        
        forestRegion->AddFactionPresence("townspeople", 0.2f);
        forestRegion->AddFactionPresence("merchants", 0.3f);
        forestRegion->AddFactionPresence("bandits", 0.7f);
        
        mountainRegion->AddFactionPresence("townspeople", 0.1f);
        mountainRegion->AddFactionPresence("merchants", 0.2f);
        mountainRegion->AddFactionPresence("bandits", 0.3f);
        
        // Set initial states
        townRegion->SetStability(0.8f);
        townRegion->SetProsperity(0.7f);
        townRegion->SetDanger(0.2f);
        townRegion->SetPopulation(1000);
        
        forestRegion->SetStability(0.5f);
        forestRegion->SetProsperity(0.4f);
        forestRegion->SetDanger(0.6f);
        forestRegion->SetPopulation(200);
        
        mountainRegion->SetStability(0.6f);
        mountainRegion->SetProsperity(0.5f);
        mountainRegion->SetDanger(0.5f);
        mountainRegion->SetPopulation(100);
    }
    
    // Create world events
    system->AddWorldEvent("bandit_raid", "Bandit Raid", "Bandits raid the local settlements");
    system->AddWorldEvent("good_harvest", "Bountiful Harvest", "The crops yield a bountiful harvest");
    system->AddWorldEvent("storm_damage", "Storm Damage", "A severe storm damages buildings and crops");
    
    // Configure events
    WorldEvent* banditEvent = system->GetWorldEvent("bandit_raid");
    WorldEvent* harvestEvent = system->GetWorldEvent("good_harvest");
    WorldEvent* stormEvent = system->GetWorldEvent("storm_damage");
    
    if (banditEvent && harvestEvent && stormEvent) {
        // Set effects for bandit raid
        banditEvent->AddRegionEffect("town", -0.2f, -0.3f, 0.4f);
        banditEvent->AddRegionEffect("forest", -0.1f, -0.1f, 0.2f);
        banditEvent->AddFactionEffect("bandits", 0.1f, -0.2f);
        banditEvent->AddFactionEffect("town_guard", -0.1f, 0.0f);
        
        // Set effects for harvest
        harvestEvent->AddRegionEffect("town", 0.1f, 0.3f, -0.1f);
        harvestEvent->AddFactionEffect("townspeople", 0.1f, 0.1f);
        harvestEvent->AddFactionEffect("merchants", 0.2f, 0.1f);
        
        // Set effects for storm
        stormEvent->AddRegionEffect("town", -0.1f, -0.2f, 0.2f);
        stormEvent->AddRegionEffect("forest", -0.05f, -0.1f, 0.15f);
        stormEvent->AddRegionEffect("mountain", -0.15f, -0.1f, 0.3f);
    }
    
    LOG(Info, "World progression system setup complete");
}

void LinenComprehensiveTest::SetupQuestSystem()
{
    auto* system = m_plugin->GetSystem<QuestSystem>();
    if (!system) return;
    
    // Add base quests
    system->AddQuest("town_quest", "Help the Town", "Complete tasks to help the townspeople.");
    system->AddQuest("explore_forest", "Forest Exploration", "Explore the forest and collect samples.");
    system->AddQuest("trading_quest", "Market Trader", "Complete trades worth at least 100 gold.");
    
    // Set quest requirements
    Quest* forestQuest = system->GetQuest("explore_forest");
    if (forestQuest) {
        forestQuest->AddSkillRequirement("survival", 2);
        forestQuest->SetExperienceReward(50);
    }
    
    Quest* tradingQuest = system->GetQuest("trading_quest");
    if (tradingQuest) {
        tradingQuest->AddSkillRequirement("commerce", 1);
        tradingQuest->SetExperienceReward(30);
    }
    
    Quest* townQuest = system->GetQuest("town_quest");
    if (townQuest) {
        townQuest->SetExperienceReward(75);
    }
    
    // Activate initial quests
    system->ActivateQuest("town_quest");
    system->ActivateQuest("trading_quest");
    
    LOG(Info, "Quest system setup complete");
}

void LinenComprehensiveTest::RunNextScenario()
{
    if (!m_plugin) return;
    
    switch (_currentScenario) {
        case 0:
            LOG(Info, "SCENARIO 1: Starting forest exploration");
            RunForestExplorationScenario();
            break;
        case 1:
            LOG(Info, "SCENARIO 2: Trading in town market");
            RunTradingScenario();
            break;
        case 2:
            LOG(Info, "SCENARIO 3: Bandit encounter");
            RunBanditScenario();
            break;
        case 3:
            LOG(Info, "SCENARIO 4: Weather and crime effects");
            RunWeatherAndCrimeScenario();
            break;
        case 4:
            LOG(Info, "SCENARIO 5: World state simulation");
            RunWorldSimulationScenario();
            break;
        case 5:
            LOG(Info, "All scenarios complete, restarting...");
            _currentScenario = -1;  // Will increment to 0
            break;
    }
    
    _currentScenario++;
}

void LinenComprehensiveTest::RunForestExplorationScenario() {
    // Get needed systems
    auto* progressSystem = m_plugin->GetSystem<CharacterProgressionSystem>();
    auto* questSystem = m_plugin->GetSystem<QuestSystem>();
    auto* worldSystem = m_plugin->GetSystem<WorldProgressionSystem>();
    
    // Check if skill requirements are met
    if (progressSystem && questSystem) {
        Quest* forestQuest = questSystem->GetQuest("explore_forest");
        if (forestQuest) {
            if (!forestQuest->CheckRequirements(progressSystem->GetSkills())) {
                LOG(Info, "Increasing survival skill to meet forest exploration requirements");
                progressSystem->IncreaseSkill("survival", 2);
            }
            
            // Activate the quest now that requirements are met
            QuestResult result = questSystem->ActivateQuest("explore_forest");
            LOG(Info, "Forest exploration quest activated: {0}", static_cast<int>(result));
        }
    }
    
    // Increase player influence in forest region
    if (worldSystem) {
        worldSystem->ModifyPlayerInfluence("forest", 0.2f);
        LOG(Info, "Player influence in forest increased due to exploration");
        
        // Discover something in the forest that affects town
        worldSystem->TriggerWorldEvent("good_harvest", "forest");
    }
    
    // Complete the quest
    if (questSystem) {
        questSystem->CompleteQuest("explore_forest");
        LOG(Info, "Forest exploration quest completed");
    }
}

void LinenComprehensiveTest::RunTradingScenario() {
    // Get needed systems
    auto* economySystem = m_plugin->GetSystem<EconomySystem>();
    auto* relationshipSystem = m_plugin->GetSystem<RelationshipSystem>();
    
    if (!economySystem) return;
    
    // Simulate player buying and selling
    economySystem->BuyItem("wood", "town_market", 5);
    LOG(Info, "Bought 5 wood from town market");
    
    economySystem->BuyItem("bread", "town_market", 3);
    LOG(Info, "Bought 3 bread from town market");
    
    // Improve relationship with merchant through trading
    if (relationshipSystem) {
        relationshipSystem->ModifyRelationship("player", "merchant", 5);
        LOG(Info, "Relationship with merchant improved through trading");
    }
    
    // Sell items at a different market for profit
    economySystem->SellItem("wood", "mountain_market", 5);
    LOG(Info, "Sold 5 wood at mountain market");
    
    // Simulate a large trade to trigger quest completion
    economySystem->BuyItem("sword", "mountain_market", 3);
    LOG(Info, "Bought 3 swords for a significant amount");
}

void LinenComprehensiveTest::RunBanditScenario() {
    // Get needed systems
    auto* crimeSystem = m_plugin->GetSystem<CrimeSystem>();
    auto* worldSystem = m_plugin->GetSystem<WorldProgressionSystem>();
    auto* questSystem = m_plugin->GetSystem<QuestSystem>();
    auto* relationshipSystem = m_plugin->GetSystem<RelationshipSystem>();
    
    // Simulate bandits committing crimes
    if (crimeSystem) {
        std::vector<std::string> witnesses = {"player", "merchant"};
        crimeSystem->ReportCrime("bandit_leader", "merchant", "town", CrimeType::Theft, witnesses);
        LOG(Info, "Bandit leader caught stealing from merchant");
        
        // Player intervenes and fights bandits
        crimeSystem->ReportCrime("player", "bandit_leader", "town", CrimeType::Assault, {"guard_captain"});
        LOG(Info, "Player assaulted bandit leader in defense of town");
    }
    
    // This triggers a world event
    if (worldSystem) {
        worldSystem->TriggerWorldEvent("bandit_raid", "forest");
        LOG(Info, "Bandit raid world event triggered in forest");
    }
    
    // If bandit quest exists, complete it
    if (questSystem && questSystem->GetQuest("bandit_quest")) {
        questSystem->CompleteQuest("bandit_quest");
        LOG(Info, "Bandit quest completed");
    }
    
    // Improve relationships with townspeople but worsen with bandits
    if (relationshipSystem) {
        relationshipSystem->ModifyRelationship("player", "bandit_leader", -30);
        relationshipSystem->ModifyRelationship("player", "guard_captain", 15);
        relationshipSystem->ModifyRelationship("player", "mayor", 10);
        LOG(Info, "Relationships updated based on bandit confrontation");
    }
}

void LinenComprehensiveTest::RunWeatherAndCrimeScenario() {
    // Get needed systems
    auto* weatherSystem = m_plugin->GetSystem<WeatherSystem>();
    auto* crimeSystem = m_plugin->GetSystem<CrimeSystem>();
    auto* economySystem = m_plugin->GetSystem<EconomySystem>();
    auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
    
    // Force a severe weather event
    if (weatherSystem) {
        weatherSystem->ForceWeatherChange(WeatherCondition::Thunderstorm, 0.9f, 1.0f);
        LOG(Info, "Thunderstorm weather event forced");
    }
    
    // Advance time to see weather effects
    if (timeSystem) {
        timeSystem->AdvanceTimeHours(3);
        LOG(Info, "Time advanced by 3 hours to experience weather effects");
    }
    
    // Simulate opportunistic crimes during the storm
    if (crimeSystem) {
        crimeSystem->ReportCrime("bandit_leader", "", "town", CrimeType::Theft);
        LOG(Info, "Bandits took advantage of the storm to commit theft");
    }
    
    // Weather affects economy
    if (economySystem) {
        Market* townMarket = economySystem->GetMarket("town_market");
        if (townMarket) {
            // Weather causes food shortages
            townMarket->ModifyItemStock("bread", -5);
            townMarket->ModifyItemStock("apple", -8);
            
            // And price increases
            float oldBreadPrice = economySystem->GetItemPrice("bread", "town_market");
            townMarket->SetCustomPrice("bread", oldBreadPrice * 1.5f);
            
            LOG(Info, "Weather caused food shortages and price increases");
        }
    }
    
    // Clear weather after scenario
    if (weatherSystem) {
        weatherSystem->ForceWeatherChange(WeatherCondition::Cloudy, 0.4f, 1.0f);
        LOG(Info, "Weather clearing after storm");
    }
}

void LinenComprehensiveTest::RunWorldSimulationScenario() {
    // Get needed systems
    auto* worldSystem = m_plugin->GetSystem<WorldProgressionSystem>();
    auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
    auto* economySystem = m_plugin->GetSystem<EconomySystem>();
    auto* factionSystem = m_plugin->GetSystem<FactionSystem>();
    
    // Advance time for more significant world changes
    if (timeSystem) {
        timeSystem->AdvanceDays(7);
        LOG(Info, "Advanced time by 7 days to simulate world changes");
    }
    
    // Check region states
    if (worldSystem) {
        WorldRegion* townRegion = worldSystem->GetRegion("town");
        WorldRegion* forestRegion = worldSystem->GetRegion("forest");
        
        if (townRegion && forestRegion) {
            LOG(Info, "Town region state: {0}, stability: {1:0.2f}, danger: {2:0.2f}", 
                String(worldSystem->RegionStateToString(townRegion->GetState()).c_str()),
                townRegion->GetStability(), townRegion->GetDanger());
                
            LOG(Info, "Forest region state: {0}, stability: {1:0.2f}, danger: {2:0.2f}", 
                String(worldSystem->RegionStateToString(forestRegion->GetState()).c_str()),
                forestRegion->GetStability(), forestRegion->GetDanger());
                
            // Force state changes based on scenario results
            if (forestRegion->GetDanger() > 0.7f) {
                worldSystem->SetRegionState("forest", RegionState::Dangerous);
                LOG(Info, "Forest has become dangerous due to accumulated effects");
                
                // This affects economy
                if (economySystem) {
                    Market* forestMarket = economySystem->GetMarket("forest_market");
                    if (forestMarket) {
                        forestMarket->SetStatus(MarketStatus::Struggling);
                        LOG(Info, "Forest market struggling due to dangerous region");
                    }
                }
            }
            
            // Player helps stabilize town
            worldSystem->ModifyPlayerInfluence("town", 0.3f);
            townRegion->SetStability(std::min(1.0f, townRegion->GetStability() + 0.2f));
            townRegion->SetDanger(std::max(0.0f, townRegion->GetDanger() - 0.2f));
            LOG(Info, "Player helped stabilize town");
        }
    }
    
    // Faction changes based on previous scenarios
    if (factionSystem) {
        factionSystem->ModifyReputation("player", "town_guard", 20);
        LOG(Info, "Player reputation with town guard increased");
        
        // Check faction reputations
        int townRep = factionSystem->GetReputation("player", "townspeople");
        int banditRep = factionSystem->GetReputation("player", "bandits");
        
        LOG(Info, "Player reputation with townspeople: {0} ({1})",
            townRep, String(factionSystem->GetReputationLevelName(
                factionSystem->GetReputationLevel("player", "townspeople")).c_str()));
                
        LOG(Info, "Player reputation with bandits: {0} ({1})",
            banditRep, String(factionSystem->GetReputationLevelName(
                factionSystem->GetReputationLevel("player", "bandits")).c_str()));
    }
    
    // Apply economic changes to reflect world state
    if (economySystem) {
        economySystem->SetGlobalEconomyFactor(1.2f);
        LOG(Info, "Global economy boosted by player actions");
    }
    
    // Save game to demonstrate serialization
    auto* saveSystem = m_plugin->GetSystem<SaveLoadSystem>();
    if (saveSystem) {
        saveSystem->SaveGame("ComprehensiveTest.bin");
        saveSystem->SaveGame("ComprehensiveTest.txt", SerializationFormat::Text);
        LOG(Info, "Game state saved in both binary and text formats");
    }
}
// ^ LinenComprehensiveTest.cpp