// v SocialInteractionManager.cpp
#include "SocialInteractionManager.h"
#include "../CrimeSystem.h"
#include "../FactionSystem.h"
#include "../LinenFlax.h"
#include "../RelationshipSystem.h"


#include "Engine/Core/Log.h"
#include "Engine/Scripting/Plugins/PluginManager.h"

/**
 * Converts a Flax Engine String to a standard C++ std::string
 * 
 * @param flaxString The Flax Engine String to convert
 * @return The equivalent std::string
 */
std::string SocialInteractionManager::FlaxStringToStdString(const String& flaxString)
{
    // Get the raw character data and length from the Flax String
    const Char* data = flaxString.Get();
    int32 length = flaxString.Length();

    // Create a std::string from the character data
    // Note: This handles the conversion from UTF-16 (Flax) to whatever
    // encoding std::string uses (typically UTF-8 or ASCII)
    return std::string(data, data + length);
}

/**
 * Converts a standard C++ std::string to a Flax Engine String
 * 
 * @param stdString The std::string to convert
 * @return The equivalent Flax Engine String
 */
String SocialInteractionManager::StdStringToFlaxString(const std::string& stdString)
{
    // Create a Flax String from the std::string data
    return String(stdString.c_str());
}

SocialInteractionManager::SocialInteractionManager(const SpawnParams& params)
    : Script(params)
{
    // Enable ticking by default
    _tickUpdate = true;

    m_plugin = PluginManager::GetPlugin<LinenFlax>();

    if (m_plugin) {
        m_relationshipSystem = m_plugin->GetSystem<RelationshipSystem>();
        m_factionSystem = m_plugin->GetSystem<FactionSystem>();
        m_crimeSystem = m_plugin->GetSystem<CrimeSystem>();
    }
}

void SocialInteractionManager::OnEnable()
{
    LOG(Info, "SocialInteractionManager script enabled");

    if (!m_plugin || !m_relationshipSystem || !m_factionSystem || !m_crimeSystem) {
        LOG(Warning, "One or more social systems not found. SocialInteractionManager won't function properly.");
        return;
    }

    // Configure systems based on editor settings
    m_relationshipSystem->SetDefaultRelationship(DefaultRelationshipValue);
    m_crimeSystem->SetCrimeExpirationTime(CrimeExpirationHours);

    // Register for social events
    m_plugin->GetEventSystem().Subscribe<RelationshipChangedEvent>(
        [this](const RelationshipChangedEvent& event) {
            this->OnRelationshipChanged(event.characterId, event.targetId, event.newValue);
        });

    m_plugin->GetEventSystem().Subscribe<FactionReputationChangedEvent>(
        [this](const FactionReputationChangedEvent& event) {
            this->OnReputationChanged(event.characterId, event.factionId, event.newValue);
        });

    m_plugin->GetEventSystem().Subscribe<BountyChangedEvent>(
        [this](const BountyChangedEvent& event) {
            this->OnBountyChanged(event.characterId, event.regionId, event.newBounty);
        });

    // Initialize test data if in debug mode
    if (DebugLogging) {
        InitializeTestData();
    }
}

void SocialInteractionManager::OnDisable()
{
    LOG(Info, "SocialInteractionManager script disabled");
}

void SocialInteractionManager::OnUpdate()
{
    // Handle debug operations when values change
    if (DebugAddFactionReputation && !m_prevAddReputation) {
        m_prevAddReputation = true;

        if (m_factionSystem) {
            m_factionSystem->ModifyReputation(
                FlaxStringToStdString(DebugCharacterId),
                FlaxStringToStdString(DebugFactionId),
                DebugReputationAmount);

            LOG(Info, "Debug: Added {0} reputation to {1} with faction {2}",
                DebugReputationAmount,
                DebugCharacterId,
                DebugFactionId);
        }
    } else if (!DebugAddFactionReputation) {
        m_prevAddReputation = false;
    }

    if (DebugAddBounty && !m_prevAddBounty) {
        m_prevAddBounty = true;

        if (m_crimeSystem) {
            CrimeType type = static_cast<CrimeType>(DebugCrimeType % 6);
            std::vector<std::string> witnesses;
            witnesses.push_back(FlaxStringToStdString(DebugNpcId));

            m_crimeSystem->ReportCrime(
                FlaxStringToStdString(DebugCharacterId),
                "", // No specific victim
                FlaxStringToStdString(DebugRegionId),
                type,
                witnesses);

            LOG(Info, "Debug: Reported crime type {0} by {1} in region {2}",
                DebugCrimeType,
                DebugCharacterId,
                DebugRegionId);
        }
    } else if (!DebugAddBounty) {
        m_prevAddBounty = false;
    }
}

void SocialInteractionManager::InitializeTestData()
{
    LOG(Info, "Initializing test data for social systems");

    // Setup test relationships
    if (m_relationshipSystem) {
        // Register test characters
        m_relationshipSystem->RegisterCharacter(FlaxStringToStdString(DebugCharacterId), "Player");
        m_relationshipSystem->RegisterCharacter(FlaxStringToStdString(DebugNpcId), "Guard Captain");
        m_relationshipSystem->RegisterCharacter("innkeeper", "Innkeeper");
        m_relationshipSystem->RegisterCharacter("merchant", "Merchant");

        // Set some initial relationships
        m_relationshipSystem->SetRelationship(FlaxStringToStdString(DebugCharacterId), "innkeeper", 25);
        m_relationshipSystem->SetRelationship("innkeeper", FlaxStringToStdString(DebugCharacterId), 25);

        LOG(Info, "Test characters registered in relationship system");
    }

    // Setup test factions
    if (m_factionSystem) {
        // Create factions
        m_factionSystem->CreateFaction(FlaxStringToStdString(DebugFactionId), "City Guard", "Protectors of the city");
        m_factionSystem->CreateFaction("merchants_guild", "Merchants Guild", "Association of traders");
        m_factionSystem->CreateFaction("thieves_guild", "Thieves Guild", "Underground criminal organization");

        // Set faction relationships
        m_factionSystem->SetFactionRelationship(FlaxStringToStdString(DebugFactionId), "thieves_guild", -75);
        m_factionSystem->SetFactionRelationship(FlaxStringToStdString(DebugFactionId), "merchants_guild", 50);
        m_factionSystem->SetFactionRelationship("merchants_guild", "thieves_guild", -50);

        LOG(Info, "Test factions registered in faction system");
    }

    // Setup test crime regions
    if (m_crimeSystem) {
        // Create regions
        m_crimeSystem->RegisterRegion(FlaxStringToStdString(DebugRegionId), "City");
        m_crimeSystem->RegisterRegion("wilderness", "Wilderness");
        m_crimeSystem->RegisterRegion("castle", "Castle");

        // Set guard factions
        m_crimeSystem->RegisterGuardFaction(FlaxStringToStdString(DebugRegionId), FlaxStringToStdString(DebugFactionId));
        m_crimeSystem->RegisterGuardFaction("castle", FlaxStringToStdString(DebugFactionId));

        // Set crime severity
        m_crimeSystem->RegisterCrimeType(CrimeType::Trespassing, "Trespassing", TrespassingBounty);
        m_crimeSystem->RegisterCrimeType(CrimeType::Theft, "Theft", TheftBounty);
        m_crimeSystem->RegisterCrimeType(CrimeType::Assault, "Assault", AssaultBounty);
        m_crimeSystem->RegisterCrimeType(CrimeType::Murder, "Murder", MurderBounty);

        LOG(Info, "Test regions registered in crime system");
    }
}

void SocialInteractionManager::OnRelationshipChanged(const std::string& characterId, const std::string& targetId, int newValue)
{
    if (!DebugLogging)
        return;

    std::string characterName = m_relationshipSystem->GetCharacterName(characterId);
    std::string targetName = m_relationshipSystem->GetCharacterName(targetId);

    if (characterName.empty())
        characterName = characterId;
    if (targetName.empty())
        targetName = targetId;

    LOG(Info, "Relationship changed: {0} -> {1} = {2}",
        String(characterName.c_str()),
        String(targetName.c_str()),
        newValue);
}

void SocialInteractionManager::OnReputationChanged(const std::string& characterId, const std::string& factionId, int newValue)
{
    if (!DebugLogging)
        return;

    std::string factionName = m_factionSystem->GetFactionName(factionId);

    if (factionName.empty())
        factionName = factionId;

    LOG(Info, "Reputation changed: {0} with {1} = {2} ({3})",
        String(characterId.c_str()),
        String(factionName.c_str()),
        newValue,
        String(m_factionSystem->GetReputationLevelName(
                                  m_factionSystem->GetReputationLevel(characterId, factionId))
                   .c_str()));
}

void SocialInteractionManager::OnBountyChanged(const std::string& characterId, const std::string& regionId, int newBounty)
{
    if (!DebugLogging)
        return;

    std::string regionName = m_crimeSystem->GetRegionName(regionId);

    if (regionName.empty())
        regionName = regionId;

    LOG(Info, "Bounty changed: {0} in {1} = {2}",
        String(characterId.c_str()),
        String(regionName.c_str()),
        newBounty);
}
// ^ SocialInteractionManager.cpp