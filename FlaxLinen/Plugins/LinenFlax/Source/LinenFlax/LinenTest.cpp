// v LinenTest.cpp
#include "LinenTest.h"
#include "Engine/Core/Log.h"
#include "Engine/Scripting/Plugins/PluginManager.h"
#include "LinenFlax.h"
#include "LinenSystemIncludes.h"


LinenTest::LinenTest(const SpawnParams& params)
    : Script(params)
{
    _tickUpdate = true;
}

void LinenTest::OnEnable()
{
    try {
        LOG(Info, "LinenTest::OnEnable : Starting LinenTest");

        // Try to get the plugin from the PluginManager
        auto* plugin = PluginManager::GetPlugin<LinenFlax>();
        if (plugin && typeid(*plugin) == typeid(LinenFlax)) {

            // Test CharacterProgressionSystem functionality
            auto* characterProgressionSystem = plugin->GetSystem<CharacterProgressionSystem>();
            if (characterProgressionSystem) {
                LOG(Info, "Character Progression System loaded");

                // Skills
                characterProgressionSystem->AddSkill("strength", "Strength", "Physical power");
                characterProgressionSystem->AddSkill("intelligence", "Intelligence", "Mental acuity");
                characterProgressionSystem->IncreaseSkill("strength", 42);
                characterProgressionSystem->IncreaseSkill("intelligence", 42);
                int str_skill_level = characterProgressionSystem->GetSkillLevel("strength");
                LOG(Info, "LinenTest::OnEnable : characterProgressionSystem Retrieved Skill Level: {0}", str_skill_level);
                int int_skill_level = characterProgressionSystem->GetSkillLevel("intelligence");
                LOG(Info, "LinenTest::OnEnable : characterProgressionSystem Retrieved Skill Level: {0}", int_skill_level);

                // Experience
                int experience = characterProgressionSystem->GetExperience();
                LOG(Info, "LinenTest::OnEnable : characterProgressionSystem Retrieved Experience: {0}", experience);
                characterProgressionSystem->GainExperience(42);
                experience = characterProgressionSystem->GetExperience();
                LOG(Info, "LinenTest::OnEnable : characterProgressionSystem Retrieved Experience: {0}", experience);
                int level = characterProgressionSystem->GetLevel();
                LOG(Info, "LinenTest::OnEnable : characterProgressionSystem Retrieved Level: {0}", level);
            } else {
                LOG(Error, "Character Progression System not found!");
            }

            // Test QuestSystem functionality
            auto* questSystem = plugin->GetSystem<QuestSystem>();
            if (questSystem) {
                LOG(Info, "Quest System loaded");

                // Quest Management
                questSystem->AddQuest("test_quest_completed", "Test Quest Complete", "A test quest complete.");
                questSystem->AddQuest("test_quest_failed", "Test Quest Fail", "A test quest failing.");
                questSystem->ActivateQuest("test_quest_completed");
                questSystem->CompleteQuest("test_quest_completed");
                questSystem->ActivateQuest("test_quest_failed");
                questSystem->FailQuest("test_quest_failed");

                // Quest queries
                questSystem->AddQuest("test_quest_query", "Test Quest Query", "A test quest query.");
                questSystem->AddQuest("test_quest_query_2", "Test Quest Query 2", "A test quest query 2.");
                questSystem->ActivateQuest("test_quest_query");
                Quest* quest = questSystem->GetQuest("test_quest_query");

                std::vector<Quest*> availableQuests = questSystem->GetAvailableQuests();
                std::vector<Quest*> activeQuests = questSystem->GetActiveQuests();
                std::vector<Quest*> completedQuests = questSystem->GetCompletedQuests();
                std::vector<Quest*> failedQuests = questSystem->GetFailedQuests();

                // Log just the sizes
                LOG(Info, "LinenTest::OnEnable : questSystem Retrieved Available Quests: {0}", availableQuests.size());
                LOG(Info, "LinenTest::OnEnable : questSystem Retrieved Active Quests: {0}", activeQuests.size());
                LOG(Info, "LinenTest::OnEnable : questSystem Retrieved Completed Quests: {0}", completedQuests.size());
                LOG(Info, "LinenTest::OnEnable : questSystem Retrieved Failed Quests: {0}", failedQuests.size());

                // For available quests
                String availableQuestIds;
                for (size_t i = 0; i < availableQuests.size(); i++) {
                    availableQuestIds += String(availableQuests[i]->GetId().c_str());
                    if (i < availableQuests.size() - 1)
                        availableQuestIds += TEXT(", ");
                }
                LOG(Info, "LinenTest::OnEnable : questSystem Retrieved Available Quests: {0} [{1}]",
                    availableQuests.size(),
                    availableQuestIds);

                // For active quests
                String activeQuestIds;
                for (size_t i = 0; i < activeQuests.size(); i++) {
                    activeQuestIds += String(activeQuests[i]->GetId().c_str());
                    if (i < activeQuests.size() - 1)
                        activeQuestIds += TEXT(", ");
                }
                LOG(Info, "LinenTest::OnEnable : questSystem Retrieved Active Quests: {0} [{1}]",
                    activeQuests.size(),
                    activeQuestIds);

                // For completed quests
                String completedQuestIds;
                for (size_t i = 0; i < completedQuests.size(); i++) {
                    completedQuestIds += String(completedQuests[i]->GetId().c_str());
                    if (i < completedQuests.size() - 1)
                        completedQuestIds += TEXT(", ");
                }
                LOG(Info, "LinenTest::OnEnable : questSystem Retrieved Completed Quests: {0} [{1}]",
                    completedQuests.size(),
                    completedQuestIds);

                // For failed quests
                String failedQuestIds;
                for (size_t i = 0; i < failedQuests.size(); i++) {
                    failedQuestIds += String(failedQuests[i]->GetId().c_str());
                    if (i < failedQuests.size() - 1)
                        failedQuestIds += TEXT(", ");
                }
                LOG(Info, "LinenTest::OnEnable : questSystem Retrieved Failed Quests: {0} [{1}]",
                    failedQuests.size(),
                    failedQuestIds);

            } else {
                LOG(Error, "Quest System not found!");
            }

            // Test TimeSystem functionality
            auto* timeSystem = plugin->GetSystem<TimeSystem>();
            if (timeSystem) {
                LOG(Info, "Time System loaded");

                // Test time information
                LOG(Info, "Current time: {0}", String(timeSystem->GetFormattedTime().c_str()));
                LOG(Info, "Current date: {0}", String(timeSystem->GetFormattedDate().c_str()));
                LOG(Info, "Current season: {0}", String(timeSystem->GetCurrentSeason().c_str()));
                LOG(Info, "Day of season: {0}", timeSystem->GetDayOfSeason());
                LOG(Info, "Is daytime: {0}", String(timeSystem->IsDaytime() ? "Yes" : "No"));

                // Test time manipulation
                LOG(Info, "Testing time advance...");
                timeSystem->SetTimeScale(10.0f);
                LOG(Info, "Testing time advance...");
                timeSystem->SetTimeScale(10.0f); // Speed up time for testing
                LOG(Info, "Time scale set to {0}x", timeSystem->GetTimeScale());

                // Test advancing hours
                LOG(Info, "Before advancing: {0}", String(timeSystem->GetFormattedTime().c_str()));
                timeSystem->AdvanceTimeSeconds(6); // Advance 6 seconds
                LOG(Info, "After advancing 6 seconds: {0}", String(timeSystem->GetFormattedTime().c_str()));
                LOG(Info, "Before advancing: {0}", String(timeSystem->GetFormattedTime().c_str()));
                timeSystem->AdvanceTimeMinutes(6); // Advance 6 minutes
                LOG(Info, "After advancing 6 minutes: {0}", String(timeSystem->GetFormattedTime().c_str()));
                LOG(Info, "Before advancing: {0}", String(timeSystem->GetFormattedTime().c_str()));
                timeSystem->AdvanceTimeHours(6); // Advance 6 hours
                LOG(Info, "After advancing 6 hours: {0}", String(timeSystem->GetFormattedTime().c_str()));

                // Test advancing days
                LOG(Info, "Before advancing days: {0}", String(timeSystem->GetFormattedDate().c_str()));
                timeSystem->AdvanceDays(375); // Advance 10 days
                LOG(Info, "After advancing 375 days: {0}", String(timeSystem->GetFormattedDate().c_str()));

                // Test debug time setting
                timeSystem->DebugSetTime(20, 30); // Set to 8:30 PM
                LOG(Info, "After debug time set: {0}", String(timeSystem->GetFormattedTime().c_str()));

                // Revert timescale
                timeSystem->SetTimeScale(1.0f);

                // Test time of day detection
                TimeOfDay tod = timeSystem->GetTimeOfDay();
                const char* todString = "";
                switch (tod) {
                case TimeOfDay::Dawn:
                    todString = "Dawn";
                    break;
                case TimeOfDay::Morning:
                    todString = "Morning";
                    break;
                case TimeOfDay::Noon:
                    todString = "Noon";
                    break;
                case TimeOfDay::Afternoon:
                    todString = "Afternoon";
                    break;
                case TimeOfDay::Evening:
                    todString = "Evening";
                    break;
                case TimeOfDay::Dusk:
                    todString = "Dusk";
                    break;
                case TimeOfDay::Night:
                    todString = "Night";
                    break;
                case TimeOfDay::Midnight:
                    todString = "Midnight";
                    break;
                }
                LOG(Info, "Current time of day: {0}", String(todString));

                // Test the day progress
                float progress = timeSystem->GetDayProgress();
                LOG(Info, "Day progress: {0:.2f}%", progress * 100.0f);

                // Test seasons
                LOG(Info, "Seasons in game:");
                const auto& seasons = timeSystem->GetSeasons();
                for (size_t i = 0; i < seasons.size(); i++) {
                    LOG(Info, "  Season {0}: {1}", i + 1, String(seasons[i].c_str()));
                }

                // Test season advancement
                int initialMonth = timeSystem->GetMonth();
                LOG(Info, "Current month: {0}", initialMonth);
                int monthsToAdvance = 4 - initialMonth + 1; // Go to next year's first month
                for (int i = 0; i < monthsToAdvance; i++) {
                    timeSystem->AdvanceDays(timeSystem->GetDaysPerMonth());
                    LOG(Info, "Advanced to month {0} ({1})",
                        timeSystem->GetMonth(),
                        String(timeSystem->GetCurrentSeason().c_str()));
                }

                // Test time serialization
                LOG(Info, "Testing TimeSystem serialization...");
                SaveLoadSystem* saveLoadSystem = plugin->GetSystem<SaveLoadSystem>();
                if (saveLoadSystem) {
                    // Register TimeSystem with SaveLoadSystem
                    saveLoadSystem->RegisterSerializableSystem("TimeSystem");

                    // Test binary serialization
                    saveLoadSystem->SaveGame("TestTimeSystem.bin", SerializationFormat::Binary);

                    // Change the time before loading to verify it works
                    timeSystem->SetHour(12);
                    timeSystem->SetDay(15);

                    // Load and check if time was restored
                    saveLoadSystem->LoadGame("TestTimeSystem.bin", SerializationFormat::Binary);

                    // Test text serialization
                    saveLoadSystem->SaveGame("TestTimeSystem.txt", SerializationFormat::Text);

                    // Change the time again
                    timeSystem->SetHour(9);
                    timeSystem->SetDay(5);

                    // Load text version
                    saveLoadSystem->LoadGame("TestTimeSystem.txt", SerializationFormat::Text);
                }
            } else {
                LOG(Error, "Time System not found!");
            }

            // Subscribe to time-related events
            plugin->GetEventSystem().Subscribe<HourChangedEvent>(
                [this](const HourChangedEvent& event) {
                    LOG(Info, "Event: Hour changed from {0} to {1}",
                        event.previousHour, event.newHour);
                });

            plugin->GetEventSystem().Subscribe<DayChangedEvent>(
                [this](const DayChangedEvent& event) {
                    LOG(Info, "Event: Day changed from {0} to {1} in {2}",
                        event.previousDay, event.newDay,
                        String(event.seasonName.c_str()));
                });

            plugin->GetEventSystem().Subscribe<SeasonChangedEvent>(
                [this](const SeasonChangedEvent& event) {
                    LOG(Info, "Event: Season changed from {0} to {1}",
                        String(event.previousSeason.c_str()),
                        String(event.newSeason.c_str()));
                });

            // Test the SaveLoadSystem
            auto* saveLoadSystem = plugin->GetSystem<SaveLoadSystem>();
            if (saveLoadSystem) {
                LOG(Info, "LinenTest::OnEnable : Save Load System loaded");

                // Test binary serialization
                saveLoadSystem->SaveGame("TestSave.bin", SerializationFormat::Binary);
                saveLoadSystem->LoadGame("TestSave.bin", SerializationFormat::Binary);

                // Test text serialization
                saveLoadSystem->SaveGame("TestSave.txt", SerializationFormat::Text);
                saveLoadSystem->LoadGame("TestSave.txt", SerializationFormat::Text);
            } else {
                LOG(Warning, "LinenTest::OnEnable : Save Load System not found");
            }

            // Test the TestSystem
            auto* testSystem = plugin->GetSystem<TestSystem>();
            if (testSystem) {
                LOG(Info, "LinenTest::OnEnable : Test System loaded");
                LOG(Info, "LinenTest::OnEnable : About to add value");
                testSystem->AddValue(42);

                LOG(Info, "LinenTest::OnEnable : About to get value");
                int value = testSystem->GetValue();
                LOG(Info, "LinenTest::OnEnable : Retrieved value: {0}", value);
            } else {
                LOG(Warning, "LinenTest::OnEnable : Test System not found");
            }

            // Test RelationshipSystem
            auto* relationshipSystem = plugin->GetSystem<RelationshipSystem>();
            if (relationshipSystem) {
                LOG(Info, "Relationship System loaded");

                // Register test characters
                relationshipSystem->RegisterCharacter("player", "Player Character");
                relationshipSystem->RegisterCharacter("npc1", "Friendly NPC");
                relationshipSystem->RegisterCharacter("npc2", "Unfriendly NPC");

                // Set relationships
                relationshipSystem->SetRelationship("player", "npc1", 75);
                relationshipSystem->SetRelationship("player", "npc2", -50);

                // Get relationships
                int relation1 = relationshipSystem->GetRelationship("player", "npc1");
                int relation2 = relationshipSystem->GetRelationship("player", "npc2");

                LOG(Info, "Player relationship with npc1: {0} ({1})",
                    relation1,
                    static_cast<int>(relationshipSystem->GetRelationshipLevel("player", "npc1")));

                LOG(Info, "Player relationship with npc2: {0} ({1})",
                    relation2,
                    static_cast<int>(relationshipSystem->GetRelationshipLevel("player", "npc2")));
            } else {
                LOG(Error, "Relationship System not found!");
            }

            // Test FactionSystem
            auto* factionSystem = plugin->GetSystem<FactionSystem>();
            if (factionSystem) {
                LOG(Info, "Faction System loaded");

                // Create test factions
                factionSystem->CreateFaction("town_guard", "Town Guard", "Protectors of the town");
                factionSystem->CreateFaction("merchants", "Merchants Guild", "Association of merchants");
                factionSystem->CreateFaction("thieves", "Thieves Guild", "Secret organization of thieves");

                // Set faction relationships
                factionSystem->SetFactionRelationship("town_guard", "merchants", 50);
                factionSystem->SetFactionRelationship("town_guard", "thieves", -75);

                // Get faction relationships
                int factionRel1 = factionSystem->GetFactionRelationship("town_guard", "merchants");
                int factionRel2 = factionSystem->GetFactionRelationship("town_guard", "thieves");

                LOG(Info, "Town Guard relationship with Merchants Guild: {0}", factionRel1);
                LOG(Info, "Town Guard relationship with Thieves Guild: {0}", factionRel2);

                // Set player reputation with factions
                factionSystem->SetReputation("player", "town_guard", 200);
                factionSystem->SetReputation("player", "merchants", 50);
                factionSystem->SetReputation("player", "thieves", -300);

                // Get player reputation with factions
                LOG(Info, "Player reputation with Town Guard: {0} ({1})",
                    factionSystem->GetReputation("player", "town_guard"),
                    String(factionSystem->GetReputationLevelName(
                                            factionSystem->GetReputationLevel("player", "town_guard"))
                               .c_str()));

                LOG(Info, "Player reputation with Merchants Guild: {0} ({1})",
                    factionSystem->GetReputation("player", "merchants"),
                    String(factionSystem->GetReputationLevelName(
                                            factionSystem->GetReputationLevel("player", "merchants"))
                               .c_str()));

                LOG(Info, "Player reputation with Thieves Guild: {0} ({1})",
                    factionSystem->GetReputation("player", "thieves"),
                    String(factionSystem->GetReputationLevelName(
                                            factionSystem->GetReputationLevel("player", "thieves"))
                               .c_str()));
            } else {
                LOG(Error, "Faction System not found!");
            }

            // Test CrimeSystem
            auto* crimeSystem = plugin->GetSystem<CrimeSystem>();
            if (crimeSystem) {
                LOG(Info, "Crime System loaded");

                // Register regions
                crimeSystem->RegisterRegion("town", "Town");
                crimeSystem->RegisterRegion("wilderness", "Wilderness");

                // Register guard factions
                crimeSystem->RegisterGuardFaction("town", "town_guard");

                // Test crime reporting
                std::vector<std::string> witnesses = { "npc1" };
                crimeSystem->ReportCrime("player", "npc2", "town", CrimeType::Theft, witnesses);

                // Check bounty
                int bounty = crimeSystem->GetBounty("player", "town");
                LOG(Info, "Player bounty in town: {0}", bounty);

                // Test another crime
                crimeSystem->ReportCrime("player", "", "town", CrimeType::Trespassing);

                // Check bounty after second crime
                bounty = crimeSystem->GetBounty("player", "town");
                LOG(Info, "Player bounty in town after second crime: {0}", bounty);

                // Test clearing bounty
                crimeSystem->ClearBounty("player", "town");
                LOG(Info, "Player bounty cleared");

                // Verify bounty cleared
                bounty = crimeSystem->GetBounty("player", "town");
                LOG(Info, "Player bounty in town after clearing: {0}", bounty);
            } else {
                LOG(Error, "Crime System not found!");
            }

            // Test EconomySystem
            auto* economySystem = plugin->GetSystem<EconomySystem>();
            if (economySystem) {
                LOG(Info, "Economy System loaded");

                // Create test markets
                economySystem->AddMarket("town_market", "Town Market");
                economySystem->AddMarket("city_market", "City Market");

                // Set market specializations and status
                Market* townMarket = economySystem->GetMarket("town_market");
                Market* cityMarket = economySystem->GetMarket("city_market");

                if (townMarket && cityMarket) {
                    townMarket->SetSpecialization(ItemCategory::Food);
                    townMarket->SetStatus(MarketStatus::Stable);

                    cityMarket->SetSpecialization(ItemCategory::Luxury);
                    cityMarket->SetStatus(MarketStatus::Prospering);

                    // Add initial stock
                    townMarket->SetItemStock("bread", 20);
                    townMarket->SetItemStock("apple", 30);
                    cityMarket->SetItemStock("silk", 10);
                    cityMarket->SetItemStock("jewel", 5);
                }

                // Register items
                economySystem->RegisterItem("bread", "Bread", 2.0f, ItemCategory::Food);
                economySystem->RegisterItem("apple", "Apple", 1.0f, ItemCategory::Food);
                economySystem->RegisterItem("silk", "Silk", 50.0f, ItemCategory::Luxury);
                economySystem->RegisterItem("jewel", "Jewel", 100.0f, ItemCategory::Luxury);

                // Test price calculations
                float breadPrice = economySystem->GetItemPrice("bread", "town_market");
                float silkPrice = economySystem->GetItemPrice("silk", "city_market");

                LOG(Info, "Bread price in Town Market: {0:0.2f}", breadPrice);
                LOG(Info, "Silk price in City Market: {0:0.2f}", silkPrice);

                // Test buying and selling
                bool buyResult = economySystem->BuyItem("bread", "town_market", 5);
                bool sellResult = economySystem->SellItem("silk", "town_market", 2);

                LOG(Info, "Buy bread result: {0}", String(buyResult ? "Success" : "Failed"));
                LOG(Info, "Sell silk result: {0}", String(sellResult ? "Success" : "Failed"));

                // Test economy event
                economySystem->TriggerEconomicEvent("harvest");
                LOG(Info, "Triggered harvest economic event");
            } else {
                LOG(Error, "Economy System not found!");
            }

            // Test WeatherSystem
            auto* weatherSystem = plugin->GetSystem<WeatherSystem>();
            if (weatherSystem) {
                LOG(Info, "Weather System loaded");

                // Get current weather info
                LOG(Info, "Current weather: {0}", String(weatherSystem->GetWeatherName().c_str()));
                LOG(Info, "Weather intensity: {0:0.2f}", weatherSystem->GetWeatherIntensity());
                LOG(Info, "Weather is dangerous: {0}", String(weatherSystem->IsWeatherDangerous() ? "Yes" : "No"));

                // Test changing weather
                weatherSystem->ForceWeatherChange(WeatherCondition::Thunderstorm, 0.8f, 5.0f);
                LOG(Info, "Forced weather change to Thunderstorm");

                // Get updated weather
                LOG(Info, "New weather: {0}", String(weatherSystem->GetWeatherName().c_str()));
                LOG(Info, "Weather transition progress: {0:0.2f}", weatherSystem->GetTransitionProgress());

                // Set weather duration
                weatherSystem->SetWeatherDuration(4.0f);
                LOG(Info, "Set weather duration to 4 hours");

                // Subscribe to weather changed events
                plugin->GetEventSystem().Subscribe<WeatherChangedEvent>(
                    [](const WeatherChangedEvent& event) {
                        LOG(Info, "Weather changed from {0} to {1} (Intensity: {2:0.2f}, Dangerous: {3})",
                            String(event.previousWeather.c_str()),
                            String(event.newWeather.c_str()),
                            event.intensity,
                            String(event.isDangerous ? "Yes" : "No"));
                    });
            } else {
                LOG(Error, "Weather System not found!");
            }

            // Test WorldProgressionSystem
            auto* worldSystem = plugin->GetSystem<WorldProgressionSystem>();
            if (worldSystem) {
                LOG(Info, "World Progression System loaded");

                // Create test regions
                worldSystem->AddRegion("town", "Small Town");
                worldSystem->AddRegion("forest", "Dark Forest");
                worldSystem->AddRegion("mountain", "High Mountains");

                // Connect regions
                worldSystem->ConnectRegions("town", "forest");
                worldSystem->ConnectRegions("forest", "mountain");

                // Create factions
                worldSystem->AddFaction("townspeople", "Townspeople");
                worldSystem->AddFaction("bandits", "Forest Bandits");
                worldSystem->AddFaction("mountaineers", "Mountain Clan");

                // Set faction relationships
                worldSystem->SetFactionRelationship("townspeople", "bandits", -0.8f);
                worldSystem->SetFactionRelationship("townspeople", "mountaineers", 0.5f);
                worldSystem->SetFactionRelationship("bandits", "mountaineers", -0.3f);

                // Add faction presence to regions
                WorldRegion* townRegion = worldSystem->GetRegion("town");
                WorldRegion* forestRegion = worldSystem->GetRegion("forest");
                WorldRegion* mountainRegion = worldSystem->GetRegion("mountain");

                if (townRegion && forestRegion && mountainRegion) {
                    townRegion->AddFactionPresence("townspeople", 0.9f);
                    townRegion->AddFactionPresence("bandits", 0.1f);

                    forestRegion->AddFactionPresence("bandits", 0.7f);
                    forestRegion->AddFactionPresence("townspeople", 0.2f);

                    mountainRegion->AddFactionPresence("mountaineers", 0.8f);

                    // Set initial states
                    townRegion->SetStability(0.8f);
                    townRegion->SetProsperity(0.7f);

                    forestRegion->SetDanger(0.6f);
                    forestRegion->SetStability(0.4f);

                    // Update states based on values
                    worldSystem->SetRegionState("forest", RegionState::Dangerous);
                }

                // Create world events
                worldSystem->AddWorldEvent("bandit_raid", "Bandit Raid", "Bandits raid the local settlements");
                worldSystem->AddWorldEvent("good_harvest", "Bountiful Harvest", "The crops yield a bountiful harvest");

                // Configure events
                WorldEvent* banditEvent = worldSystem->GetWorldEvent("bandit_raid");
                WorldEvent* harvestEvent = worldSystem->GetWorldEvent("good_harvest");

                if (banditEvent && harvestEvent) {
                    // Set effects for bandit raid
                    banditEvent->AddRegionEffect("town", -0.2f, -0.3f, 0.4f);
                    banditEvent->AddFactionEffect("bandits", 0.1f, -0.2f);

                    // Set effects for harvest
                    harvestEvent->AddRegionEffect("town", 0.1f, 0.3f, -0.1f);
                    harvestEvent->AddFactionEffect("townspeople", 0.1f, 0.1f);

                    // Trigger an event
                    worldSystem->TriggerWorldEvent("good_harvest", "town");
                    LOG(Info, "Triggered good harvest event in town");
                }

                // Get region state
                RegionState townState = worldSystem->GetRegionState("town");
                LOG(Info, "Town region state: {0}",
                    String(worldSystem->RegionStateToString(townState).c_str()));

                // Subscribe to world events
                plugin->GetEventSystem().Subscribe<WorldEventTriggeredEvent>(
                    [](const WorldEventTriggeredEvent& event) {
                        LOG(Info, "World event triggered: {0} in {1} ({2})",
                            String(event.eventName.c_str()),
                            String(event.regionId.c_str()),
                            String(event.description.c_str()));
                    });

                // Subscribe to region state changes
                plugin->GetEventSystem().Subscribe<RegionChangedEvent>(
                    [](const RegionChangedEvent& event) {
                        LOG(Info, "Region state changed: {0} from {1} to {2}",
                            String(event.regionId.c_str()),
                            String(event.oldState.c_str()),
                            String(event.newState.c_str()));
                    });
            } else {
                LOG(Error, "World Progression System not found!");
            }

        } else {
            LOG(Error, "LinenTest::OnEnable : Linen Plugin not found!");
            // Instead of creating a local instance, we should find out why the plugin isn't registered
            LOG(Info, "LinenTest::OnEnable : TODO Checking for all available plugins");
            // This would require additional code to list all plugins
        }
    } catch (...) {
        LOG(Error, "LinenTest::OnEnable : Exception during Linen testing");
    }

    LOG(Info, "LinenTest::OnEnable completed");
}

void LinenTest::OnDisable()
{
    // Minimal implementation
    LOG(Info, "LinenTest::OnDisable : ran.");
}

void LinenTest::OnUpdate()
{
    // Minimal implementation
    // LOG(Info, "LinenTest::OnUpdate : ran.");

    auto* plugin = PluginManager::GetPlugin<LinenFlax>();
    if (plugin && typeid(*plugin) == typeid(LinenFlax)) {
        auto* timeSystem = plugin->GetSystem<TimeSystem>();
        if (timeSystem) {
            // timeSystem->AdvanceTimeHours(1);
        }
    }
}
// ^ LinenTest.cpp