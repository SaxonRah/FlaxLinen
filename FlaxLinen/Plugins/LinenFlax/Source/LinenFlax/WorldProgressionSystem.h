// v WorldProgressionSystem.h
#pragma once

#include "RPGSystem.h"
#include "EventSystem.h"
#include "Serialization.h"
#include "TimeSystem.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <random>

// World progression events
class RegionChangedEvent : public EventType<RegionChangedEvent> {
public:
    std::string regionId;
    std::string oldState;
    std::string newState;
    bool isPlayerInfluenced;
};

class FactionRelationChangedEvent : public EventType<FactionRelationChangedEvent> {
public:
    std::string faction1Id;
    std::string faction2Id;
    float oldRelation;
    float newRelation;
    std::string relationStatus;
};

class WorldEventTriggeredEvent : public EventType<WorldEventTriggeredEvent> {
public:
    std::string eventId;
    std::string eventName;
    std::string regionId;
    std::string description;
    bool affectsPlayer;
};

// Region state
enum class RegionState {
    Peaceful,
    Troubled,
    Dangerous,
    Warzone,
    Abandoned,
    Rebuilding
};

// Faction relationship status
enum class FactionRelationship {
    Allied,
    Friendly,
    Neutral,
    Unfriendly,
    Hostile,
    AtWar
};

// A region in the world
class WorldRegion {
public:
    WorldRegion(const std::string& id, const std::string& name) :
        m_id(id),
        m_name(name),
        m_state(RegionState::Peaceful),
        m_population(1000),
        m_prosperity(1.0f),
        m_stability(1.0f),
        m_playerInfluence(0.0f),
        m_danger(0.0f) {}
    
    // Getters
    std::string GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    RegionState GetState() const { return m_state; }
    int GetPopulation() const { return m_population; }
    float GetProsperity() const { return m_prosperity; }
    float GetStability() const { return m_stability; }
    float GetPlayerInfluence() const { return m_playerInfluence; }
    float GetDanger() const { return m_danger; }
    std::string GetControllingFaction() const { return m_controllingFaction; }
    
    // Setters
    void SetState(RegionState state) { m_state = state; }
    void SetPopulation(int population) { m_population = std::max(0, population); }
    void SetProsperity(float prosperity) { m_prosperity = std::max(0.0f, prosperity); }
    void SetStability(float stability) { m_stability = std::max(0.0f, stability); }
    void SetPlayerInfluence(float influence) { m_playerInfluence = std::max(0.0f, std::min(1.0f, influence)); }
    void SetDanger(float danger) { m_danger = std::max(0.0f, danger); }
    void SetControllingFaction(const std::string& factionId) { m_controllingFaction = factionId; }
    
    // Add/remove faction presence
    void AddFactionPresence(const std::string& factionId, float strength);
    void RemoveFactionPresence(const std::string& factionId);
    float GetFactionPresence(const std::string& factionId) const;
    const std::unordered_map<std::string, float>& GetAllFactionPresence() const { return m_factionPresence; }
    
    // Add/remove connected regions
    void AddConnectedRegion(const std::string& regionId) { m_connectedRegions.insert(regionId); }
    void RemoveConnectedRegion(const std::string& regionId) { m_connectedRegions.erase(regionId); }
    bool IsConnectedTo(const std::string& regionId) const { return m_connectedRegions.find(regionId) != m_connectedRegions.end(); }
    const std::set<std::string>& GetConnectedRegions() const { return m_connectedRegions; }
    
    // Serialization
    void Serialize(BinaryWriter& writer) const;
    void Deserialize(BinaryReader& reader);
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);
    
private:
    std::string m_id;
    std::string m_name;
    RegionState m_state;
    int m_population;
    float m_prosperity;
    float m_stability;
    float m_playerInfluence;
    float m_danger;
    std::string m_controllingFaction;
    
    // Faction presences (faction id -> strength)
    std::unordered_map<std::string, float> m_factionPresence;
    
    // Connected regions
    std::set<std::string> m_connectedRegions;
};

// A faction in the world
class WorldFaction {
public:
    WorldFaction(const std::string& id, const std::string& name) :
        m_id(id),
        m_name(name),
        m_power(1.0f),
        m_aggression(0.5f),
        m_expansionism(0.5f),
        m_playerRelationship(0.0f) {}
    
    // Getters
    std::string GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    float GetPower() const { return m_power; }
    float GetAggression() const { return m_aggression; }
    float GetExpansionism() const { return m_expansionism; }
    float GetPlayerRelationship() const { return m_playerRelationship; }
    
    // Setters
    void SetPower(float power) { m_power = std::max(0.0f, power); }
    void SetAggression(float aggression) { m_aggression = std::max(0.0f, std::min(1.0f, aggression)); }
    void SetExpansionism(float expansionism) { m_expansionism = std::max(0.0f, std::min(1.0f, expansionism)); }
    void SetPlayerRelationship(float relationship) { m_playerRelationship = std::max(-1.0f, std::min(1.0f, relationship)); }
    
    // Faction relationships
    void SetRelationship(const std::string& otherFactionId, float value);
    float GetRelationship(const std::string& otherFactionId) const;
    FactionRelationship GetRelationshipStatus(const std::string& otherFactionId) const;
    const std::unordered_map<std::string, float>& GetAllRelationships() const { return m_relationships; }
    
    // Serialization
    void Serialize(BinaryWriter& writer) const;
    void Deserialize(BinaryReader& reader);
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);
    
private:
    std::string m_id;
    std::string m_name;
    float m_power;
    float m_aggression;
    float m_expansionism;
    float m_playerRelationship;
    
    // Relationships with other factions (-1.0 to 1.0)
    std::unordered_map<std::string, float> m_relationships;
};

// A world event that can occur
class WorldEvent {
public:
    WorldEvent(const std::string& id, const std::string& name, const std::string& description) :
        m_id(id),
        m_name(name),
        m_description(description),
        m_weight(1.0f),
        m_cooldown(0.0f),
        m_lastTriggerTime(-999.0f) {}
    
    // Getters
    std::string GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    std::string GetDescription() const { return m_description; }
    float GetWeight() const { return m_weight; }
    float GetCooldown() const { return m_cooldown; }
    float GetLastTriggerTime() const { return m_lastTriggerTime; }
    
    // Setters
    void SetWeight(float weight) { m_weight = std::max(0.0f, weight); }
    void SetCooldown(float cooldown) { m_cooldown = std::max(0.0f, cooldown); }
    void SetLastTriggerTime(float time) { m_lastTriggerTime = time; }
    
    // Check if event is on cooldown
    bool IsOnCooldown(float currentTime) const { return (currentTime - m_lastTriggerTime) < m_cooldown; }
    
    // Event effects on regions
    void AddRegionEffect(const std::string& regionId, float stabilityChange, float prosperityChange, float dangerChange);
    bool HasEffectForRegion(const std::string& regionId) const;
    void GetRegionEffects(const std::string& regionId, float& stabilityChange, float& prosperityChange, float& dangerChange) const;
    const std::unordered_map<std::string, std::tuple<float, float, float>>& GetAllRegionEffects() const { return m_regionEffects; }
    
    // Event effects on factions
    void AddFactionEffect(const std::string& factionId, float powerChange, float relationshipChange);
    bool HasEffectForFaction(const std::string& factionId) const;
    void GetFactionEffects(const std::string& factionId, float& powerChange, float& relationshipChange) const;
    const std::unordered_map<std::string, std::pair<float, float>>& GetAllFactionEffects() const { return m_factionEffects; }
    
    // Serialization
    void Serialize(BinaryWriter& writer) const;
    void Deserialize(BinaryReader& reader);
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);
    
private:
    std::string m_id;
    std::string m_name;
    std::string m_description;
    float m_weight;
    float m_cooldown;
    float m_lastTriggerTime;
    
    // Effects on regions (region id -> (stability change, prosperity change, danger change))
    std::unordered_map<std::string, std::tuple<float, float, float>> m_regionEffects;
    
    // Effects on factions (faction id -> (power change, relationship change))
    std::unordered_map<std::string, std::pair<float, float>> m_factionEffects;
};

class WorldProgressionSystem : public RPGSystem {
public:
    // Delete copy constructor and assignment operator
    WorldProgressionSystem(const WorldProgressionSystem&) = delete;
    WorldProgressionSystem& operator=(const WorldProgressionSystem&) = delete;
    
    // Meyer's Singleton - thread-safe in C++11 and beyond
    static WorldProgressionSystem* GetInstance() {
        static WorldProgressionSystem* instance = new WorldProgressionSystem();
        return instance;
    }
    
    // Cleanup method
    static void Destroy() {
        static WorldProgressionSystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }
    
    ~WorldProgressionSystem();

    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetName() const override { return "WorldProgressionSystem"; }
    
    // Serialization
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);
    
    // Region management
    bool AddRegion(const std::string& id, const std::string& name);
    WorldRegion* GetRegion(const std::string& id);
    void SetRegionState(const std::string& regionId, RegionState state);
    void ConnectRegions(const std::string& region1Id, const std::string& region2Id);
    RegionState CalculateRegionState(WorldRegion* region);
    
    // Faction management
    bool AddFaction(const std::string& id, const std::string& name);
    WorldFaction* GetFaction(const std::string& id);
    void SetFactionRelationship(const std::string& faction1Id, const std::string& faction2Id, float value);
    void ModifyPlayerInfluence(const std::string& regionId, float change);
    void ResolveFactionConflict(const std::string& faction1Id, const std::string& faction2Id);
    
    // World event management
    bool AddWorldEvent(const std::string& id, const std::string& name, const std::string& description);
    WorldEvent* GetWorldEvent(const std::string& id);
    bool TriggerWorldEvent(const std::string& eventId, const std::string& regionId);
    
    // World state queries
    RegionState GetRegionState(const std::string& regionId) const;
    float GetFactionRelationship(const std::string& faction1Id, const std::string& faction2Id) const;
    std::vector<std::string> GetRegionsControlledByFaction(const std::string& factionId) const;
    
    // Utility
    std::string RegionStateToString(RegionState state) const;
    RegionState StringToRegionState(const std::string& str) const;
    std::string FactionRelationshipToString(FactionRelationship relationship) const;
    FactionRelationship StringToFactionRelationship(const std::string& str) const;

private:
    // Private constructor
    WorldProgressionSystem();
    
    // World state
    std::unordered_map<std::string, std::unique_ptr<WorldRegion>> m_regions;
    std::unordered_map<std::string, std::unique_ptr<WorldFaction>> m_factions;
    std::unordered_map<std::string, std::unique_ptr<WorldEvent>> m_worldEvents;
    
    // World simulation parameters
    float m_worldSimulationInterval = 24.0f; // In hours
    float m_timeSinceLastSimulation = 0.0f;
    float m_gameTime = 0.0f;
    float m_eventChance = 0.2f; // 20% chance per region per day
    
    // Random generator
    std::mt19937 m_randomGenerator;
    
    // Simulation methods
    void SimulateWorld();
    void SimulateRegion(WorldRegion* region);
    void SimulateFactions();
    void SimulateFactionConflicts();
    void AttemptRegionalEvents();
    
    // Helper methods
    FactionRelationship RelationshipValueToStatus(float value) const;
    float GetFactionInfluenceWeighted(const std::string& regionId, const std::string& factionId) const;
    void UpdateRegionController(WorldRegion* region);
    WorldFaction* GetDominantFaction(WorldRegion* region);
    bool AreFactionsInConflict(const std::string& faction1Id, const std::string& faction2Id) const;
    void AdjustRegionByState(WorldRegion* region);
};
// ^ WorldProgressionSystem.h