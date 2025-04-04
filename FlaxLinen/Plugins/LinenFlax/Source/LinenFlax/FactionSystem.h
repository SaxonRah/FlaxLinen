// v FactionSystem.h
#pragma once

#include "EventSystem.h"
#include "RPGSystem.h"
#include "Serialization.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


// Faction reputation events
class FactionReputationChangedEvent : public EventType<FactionReputationChangedEvent> {
public:
    std::string characterId;
    std::string factionId;
    int previousValue;
    int newValue;
    std::string reputationLevel;
};

// Enum for reputation levels
enum class ReputationLevel {
    Hated = -3,
    Hostile = -2,
    Unfriendly = -1,
    Neutral = 0,
    Friendly = 1,
    Honored = 2,
    Exalted = 3
};

class FactionSystem : public RPGSystem {
public:
    // Delete copy constructor and assignment operator
    FactionSystem(const FactionSystem&) = delete;
    FactionSystem& operator=(const FactionSystem&) = delete;

    // Meyer's Singleton pattern
    static FactionSystem* GetInstance()
    {
        static FactionSystem* instance = new FactionSystem();
        return instance;
    }

    // Cleanup method
    static void Destroy()
    {
        static FactionSystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }

    ~FactionSystem();

    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetName() const override { return "FactionSystem"; }

    // Serialization
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);

    // Faction management
    bool CreateFaction(const std::string& factionId, const std::string& name, const std::string& description);
    bool DoesFactionExist(const std::string& factionId) const;
    std::string GetFactionName(const std::string& factionId) const;
    std::string GetFactionDescription(const std::string& factionId) const;
    std::vector<std::string> GetAllFactions() const;

    // Faction relationships
    void SetFactionRelationship(const std::string& faction1, const std::string& faction2, int value);
    int GetFactionRelationship(const std::string& faction1, const std::string& faction2) const;

    // Character reputation with factions
    void SetReputation(const std::string& characterId, const std::string& factionId, int value);
    void ModifyReputation(const std::string& characterId, const std::string& factionId, int delta);
    int GetReputation(const std::string& characterId, const std::string& factionId) const;
    ReputationLevel GetReputationLevel(const std::string& characterId, const std::string& factionId) const;
    std::string GetReputationLevelName(ReputationLevel level) const;

    // Register faction benefits/penalties based on reputation
    void RegisterReputationEffect(const std::string& factionId, ReputationLevel minLevel,
        std::function<void(const std::string&)> effect);

    // Apply all relevant effects for a character
    void ApplyReputationEffects(const std::string& characterId);

private:
    // Private constructor
    FactionSystem();

    // Data structures
    struct Faction {
        std::string id;
        std::string name;
        std::string description;
        std::unordered_map<std::string, int> relations; // Relations with other factions
    };

    struct ReputationEffect {
        std::string factionId;
        ReputationLevel minLevel;
        std::function<void(const std::string&)> effect;
    };

    // Storage
    std::unordered_map<std::string, Faction> m_factions;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> m_reputations; // character -> faction -> value
    std::vector<ReputationEffect> m_reputationEffects;

    // Default values
    int m_defaultReputation = 0;
    int m_defaultFactionRelation = 0;

    // Helper methods
    ReputationLevel ValueToReputationLevel(int value) const;
};
// ^ FactionSystem.h