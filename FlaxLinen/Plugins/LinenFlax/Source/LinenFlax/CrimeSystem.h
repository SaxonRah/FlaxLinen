// v CrimeSystem.h
#pragma once

#include "EventSystem.h"
#include "RPGSystem.h"
#include "Serialization.h"
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


// Crime-related events
class CrimeCommittedEvent : public EventType<CrimeCommittedEvent> {
public:
    std::string perpetratorId;
    std::string victimId;
    std::string regionId;
    std::string crimeType;
    int severity;
    bool witnessed;
    std::vector<std::string> witnessIds;
};

class BountyChangedEvent : public EventType<BountyChangedEvent> {
public:
    std::string characterId;
    std::string regionId;
    int previousBounty;
    int newBounty;
};

class CrimeExpiredEvent : public EventType<CrimeExpiredEvent> {
public:
    std::string perpetratorId;
    std::string regionId;
    std::string crimeType;
};

// Enum for crime types
enum class CrimeType {
    Trespassing, // Entering restricted areas
    Theft, // Stealing items
    Assault, // Attacking NPCs
    Murder, // Killing NPCs
    Vandalism, // Destroying property
    MagicUsage // Using forbidden magic
};

// Enum for witness reaction
enum class WitnessReaction {
    Ignore, // Witness doesn't care
    Report, // Witness reports to guards
    Flee, // Witness runs away
    Attack // Witness attacks criminal
};

class CrimeSystem : public RPGSystem {
public:
    // Delete copy constructor and assignment operator
    CrimeSystem(const CrimeSystem&) = delete;
    CrimeSystem& operator=(const CrimeSystem&) = delete;

    // Meyer's Singleton pattern
    static CrimeSystem* GetInstance()
    {
        static CrimeSystem* instance = new CrimeSystem();
        return instance;
    }

    // Cleanup method
    static void Destroy()
    {
        static CrimeSystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }

    ~CrimeSystem();

    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetName() const override { return "CrimeSystem"; }

    // Serialization
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);

    // Region management
    bool RegisterRegion(const std::string& regionId, const std::string& name);
    bool DoesRegionExist(const std::string& regionId) const;
    std::string GetRegionName(const std::string& regionId) const;

    // Crime definitions
    void RegisterCrimeType(CrimeType type, const std::string& name, int baseSeverity);
    int GetCrimeSeverity(CrimeType type) const;
    std::string GetCrimeTypeName(CrimeType type) const;

    // Crime reporting
    void ReportCrime(const std::string& perpetratorId, const std::string& victimId,
        const std::string& regionId, CrimeType type,
        const std::vector<std::string>& witnesses = {});

    // Witness system
    void RegisterWitness(const std::string& characterId, const std::string& perpetratorId,
        const std::string& regionId, CrimeType type);
    WitnessReaction DetermineWitnessReaction(const std::string& witnessId,
        const std::string& perpetratorId,
        CrimeType type) const;

    // Bounty management
    int GetBounty(const std::string& characterId, const std::string& regionId) const;
    void ModifyBounty(const std::string& characterId, const std::string& regionId, int amount);
    void ClearBounty(const std::string& characterId, const std::string& regionId);
    bool HasBounty(const std::string& characterId, const std::string& regionId) const;

    // Guard response
    void RegisterGuardFaction(const std::string& regionId, const std::string& factionId);
    bool IsGuardFaction(const std::string& regionId, const std::string& factionId) const;

    // Time-based bounty reduction/expiration
    void SetCrimeExpirationTime(int gameHours);
    void ProcessExpiredCrimes();

private:
    // Private constructor
    CrimeSystem();

    // Data structures
    struct Crime {
        std::string perpetratorId;
        std::string victimId;
        std::string regionId;
        CrimeType type;
        int severity;
        bool reported;
        float gameTimeCommitted;
        std::vector<std::string> witnesses;
    };

    struct Region {
        std::string id;
        std::string name;
        std::string guardFaction;
        std::unordered_map<std::string, int> bounties; // character -> bounty
    };

    struct CrimeDefinition {
        CrimeType type;
        std::string name;
        int baseSeverity;
    };

    // Storage
    std::unordered_map<std::string, Region> m_regions;
    std::unordered_map<CrimeType, CrimeDefinition> m_crimeDefinitions;
    std::vector<Crime> m_activeCrimes;

    // Settings
    int m_crimeExpirationHours = 72; // Crimes expire after 3 days by default
    float m_currentGameTime = 0.0f;

    // Helper methods
    void CalculateBounty(const Crime& crime);
    void PurgeCrime(size_t index);
};
// ^ CrimeSystem.h