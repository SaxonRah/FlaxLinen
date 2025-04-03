// v FactionSystem.cpp
#include "FactionSystem.h"
#include "LinenFlax.h"
#include "Engine/Core/Log.h"

FactionSystem::FactionSystem() {
    // Define system dependencies
    m_dependencies.insert("RelationshipSystem");
}

FactionSystem::~FactionSystem() {
    Destroy();
}

void FactionSystem::Initialize() {
    LOG(Info, "Faction System Initialized.");
}

void FactionSystem::Shutdown() {
    m_factions.clear();
    m_reputations.clear();
    m_reputationEffects.clear();
    LOG(Info, "Faction System Shutdown.");
}

void FactionSystem::Update(float deltaTime) {
    // Nothing to update regularly
}

bool FactionSystem::CreateFaction(const std::string& factionId, const std::string& name, const std::string& description) {
    if (m_factions.find(factionId) != m_factions.end()) {
        LOG(Warning, "Faction already exists: {0}", String(factionId.c_str()));
        return false;
    }
    
    Faction faction;
    faction.id = factionId;
    faction.name = name;
    faction.description = description;
    
    m_factions[factionId] = faction;
    LOG(Info, "Created faction: {0} ({1})", String(name.c_str()), String(factionId.c_str()));
    return true;
}

bool FactionSystem::DoesFactionExist(const std::string& factionId) const {
    return m_factions.find(factionId) != m_factions.end();
}

std::string FactionSystem::GetFactionName(const std::string& factionId) const {
    auto it = m_factions.find(factionId);
    if (it == m_factions.end()) {
        return "";
    }
    return it->second.name;
}

std::string FactionSystem::GetFactionDescription(const std::string& factionId) const {
    auto it = m_factions.find(factionId);
    if (it == m_factions.end()) {
        return "";
    }
    return it->second.description;
}

std::vector<std::string> FactionSystem::GetAllFactions() const {
    std::vector<std::string> result;
    result.reserve(m_factions.size());
    
    for (const auto& pair : m_factions) {
        result.push_back(pair.first);
    }
    
    return result;
}

void FactionSystem::SetFactionRelationship(const std::string& faction1, const std::string& faction2, int value) {
    if (!DoesFactionExist(faction1)) {
        LOG(Warning, "Faction does not exist: {0}", String(faction1.c_str()));
        return;
    }
    
    if (!DoesFactionExist(faction2)) {
        LOG(Warning, "Faction does not exist: {0}", String(faction2.c_str()));
        return;
    }
    
    // Clamp value between -100 and 100
    value = value < -100 ? -100 : (value > 100 ? 100 : value);
    
    // Set relationship both ways (symmetrical)
    m_factions[faction1].relations[faction2] = value;
    m_factions[faction2].relations[faction1] = value;
    
    LOG(Info, "Set faction relationship: {0} <-> {1} = {2}", 
        String(faction1.c_str()), String(faction2.c_str()), value);
}

int FactionSystem::GetFactionRelationship(const std::string& faction1, const std::string& faction2) const {
    if (!DoesFactionExist(faction1) || !DoesFactionExist(faction2)) {
        return m_defaultFactionRelation;
    }
    
    // Same faction
    if (faction1 == faction2) {
        return 100;
    }
    
    const auto& faction = m_factions.at(faction1);
    auto it = faction.relations.find(faction2);
    
    if (it == faction.relations.end()) {
        return m_defaultFactionRelation;
    }
    
    return it->second;
}

void FactionSystem::SetReputation(const std::string& characterId, const std::string& factionId, int value) {
    if (!DoesFactionExist(factionId)) {
        LOG(Warning, "Faction does not exist: {0}", String(factionId.c_str()));
        return;
    }
    
    // Clamp value between -1000 and 1000
    value = value < -1000 ? -1000 : (value > 1000 ? 1000 : value);
    
    int previousValue = GetReputation(characterId, factionId);
    m_reputations[characterId][factionId] = value;
    
    // Create and publish event
    FactionReputationChangedEvent event;
    event.characterId = characterId;
    event.factionId = factionId;
    event.previousValue = previousValue;
    event.newValue = value;
    event.reputationLevel = GetReputationLevelName(GetReputationLevel(characterId, factionId));
    
    m_plugin->GetEventSystem().Publish(event);
    
    LOG(Info, "Set reputation: {0} with {1} = {2} ({3})", 
        String(characterId.c_str()), String(factionId.c_str()), 
        value, String(event.reputationLevel.c_str()));
    
    // Apply effects if applicable
    ApplyReputationEffects(characterId);
}

void FactionSystem::ModifyReputation(const std::string& characterId, const std::string& factionId, int delta) {
    if (delta == 0) return; // No change
    
    int currentValue = GetReputation(characterId, factionId);
    SetReputation(characterId, factionId, currentValue + delta);
}

int FactionSystem::GetReputation(const std::string& characterId, const std::string& factionId) const {
    if (!DoesFactionExist(factionId)) {
        return m_defaultReputation;
    }
    
    auto charIt = m_reputations.find(characterId);
    if (charIt == m_reputations.end()) {
        return m_defaultReputation;
    }
    
    auto factionIt = charIt->second.find(factionId);
    if (factionIt == charIt->second.end()) {
        return m_defaultReputation;
    }
    
    return factionIt->second;
}

ReputationLevel FactionSystem::GetReputationLevel(const std::string& characterId, const std::string& factionId) const {
    int value = GetReputation(characterId, factionId);
    return ValueToReputationLevel(value);
}

ReputationLevel FactionSystem::ValueToReputationLevel(int value) const {
    if (value <= -900) return ReputationLevel::Hated;
    if (value <= -500) return ReputationLevel::Hostile;
    if (value <= -100) return ReputationLevel::Unfriendly;
    if (value < 100) return ReputationLevel::Neutral;
    if (value < 500) return ReputationLevel::Friendly;
    if (value < 900) return ReputationLevel::Honored;
    return ReputationLevel::Exalted;
}

std::string FactionSystem::GetReputationLevelName(ReputationLevel level) const {
    switch (level) {
        case ReputationLevel::Hated: return "Hated";
        case ReputationLevel::Hostile: return "Hostile";
        case ReputationLevel::Unfriendly: return "Unfriendly";
        case ReputationLevel::Neutral: return "Neutral";
        case ReputationLevel::Friendly: return "Friendly";
        case ReputationLevel::Honored: return "Honored";
        case ReputationLevel::Exalted: return "Exalted";
        default: return "Unknown";
    }
}

void FactionSystem::RegisterReputationEffect(const std::string& factionId, ReputationLevel minLevel, 
                                          std::function<void(const std::string&)> effect) {
    if (!DoesFactionExist(factionId)) {
        LOG(Warning, "Cannot register effect for non-existent faction: {0}", String(factionId.c_str()));
        return;
    }
    
    ReputationEffect repEffect;
    repEffect.factionId = factionId;
    repEffect.minLevel = minLevel;
    repEffect.effect = effect;
    
    m_reputationEffects.push_back(repEffect);
    
    LOG(Info, "Registered reputation effect for faction {0} at level {1}", 
        String(factionId.c_str()), static_cast<int>(minLevel));
}

void FactionSystem::ApplyReputationEffects(const std::string& characterId) {
    for (const auto& effect : m_reputationEffects) {
        ReputationLevel level = GetReputationLevel(characterId, effect.factionId);
        
        if (static_cast<int>(level) >= static_cast<int>(effect.minLevel)) {
            LOG(Info, "Applying reputation effect for character {0} with faction {1}", 
                String(characterId.c_str()), String(effect.factionId.c_str()));
            
            effect.effect(characterId);
        }
    }
}

void FactionSystem::Serialize(BinaryWriter& writer) const {
    // Write default values
    writer.Write(m_defaultReputation);
    writer.Write(m_defaultFactionRelation);
    
    // Write factions
    writer.Write(static_cast<uint32_t>(m_factions.size()));
    
    for (const auto& pair : m_factions) {
        const Faction& faction = pair.second;
        
        writer.Write(faction.id);
        writer.Write(faction.name);
        writer.Write(faction.description);
        
        // Write faction relationships
        writer.Write(static_cast<uint32_t>(faction.relations.size()));
        
        for (const auto& rel : faction.relations) {
            writer.Write(rel.first);  // Target faction ID
            writer.Write(rel.second); // Relationship value
        }
    }
    
    // Write character reputations
    writer.Write(static_cast<uint32_t>(m_reputations.size()));
    
    for (const auto& charPair : m_reputations) {
        writer.Write(charPair.first); // Character ID
        writer.Write(static_cast<uint32_t>(charPair.second.size())); // Number of faction entries
        
        for (const auto& factionPair : charPair.second) {
            writer.Write(factionPair.first);  // Faction ID
            writer.Write(factionPair.second); // Reputation value
        }
    }
    
    // Note: We don't serialize effects as they are registered at runtime
    
    LOG(Info, "FactionSystem serialized");
}

void FactionSystem::Deserialize(BinaryReader& reader) {
    // Clear existing data
    m_factions.clear();
    m_reputations.clear();
    // We don't clear effects as they are registered at runtime
    
    // Read default values
    reader.Read(m_defaultReputation);
    reader.Read(m_defaultFactionRelation);
    
    // Read factions
    uint32_t factionCount = 0;
    reader.Read(factionCount);
    
    for (uint32_t i = 0; i < factionCount; ++i) {
        Faction faction;
        
        reader.Read(faction.id);
        reader.Read(faction.name);
        reader.Read(faction.description);
        
        // Read faction relationships
        uint32_t relationCount = 0;
        reader.Read(relationCount);
        
        for (uint32_t j = 0; j < relationCount; ++j) {
            std::string targetId;
            int value = 0;
            
            reader.Read(targetId);
            reader.Read(value);
            
            faction.relations[targetId] = value;
        }
        
        m_factions[faction.id] = faction;
    }
    
    // Read character reputations
    uint32_t characterCount = 0;
    reader.Read(characterCount);
    
    for (uint32_t i = 0; i < characterCount; ++i) {
        std::string characterId;
        reader.Read(characterId);
        
        uint32_t factionEntryCount = 0;
        reader.Read(factionEntryCount);
        
        std::unordered_map<std::string, int> reputations;
        
        for (uint32_t j = 0; j < factionEntryCount; ++j) {
            std::string factionId;
            int value = 0;
            
            reader.Read(factionId);
            reader.Read(value);
            
            reputations[factionId] = value;
        }
        
        m_reputations[characterId] = reputations;
    }
    
    LOG(Info, "FactionSystem deserialized");
}

void FactionSystem::SerializeToText(TextWriter& writer) const {
    // Write default values
    writer.Write("defaultReputation", m_defaultReputation);
    writer.Write("defaultFactionRelation", m_defaultFactionRelation);
    
    // Write faction count
    writer.Write("factionCount", static_cast<int>(m_factions.size()));
    
    // Write each faction
    int factionIndex = 0;
    for (const auto& pair : m_factions) {
        const Faction& faction = pair.second;
        std::string prefix = "faction" + std::to_string(factionIndex) + "_";
        
        writer.Write(prefix + "id", faction.id);
        writer.Write(prefix + "name", faction.name);
        writer.Write(prefix + "description", faction.description);
        
        // Write faction relationships
        writer.Write(prefix + "relationCount", static_cast<int>(faction.relations.size()));
        
        int relationIndex = 0;
        for (const auto& rel : faction.relations) {
            std::string relPrefix = prefix + "rel" + std::to_string(relationIndex) + "_";
            writer.Write(relPrefix + "targetId", rel.first);
            writer.Write(relPrefix + "value", rel.second);
            relationIndex++;
        }
        
        factionIndex++;
    }
    
    // Write character reputation count
    writer.Write("characterCount", static_cast<int>(m_reputations.size()));
    
    // Write each character's reputations
    int characterIndex = 0;
    for (const auto& charPair : m_reputations) {
        std::string prefix = "character" + std::to_string(characterIndex) + "_";
        
        writer.Write(prefix + "id", charPair.first);
        writer.Write(prefix + "factionCount", static_cast<int>(charPair.second.size()));
        
        int factionEntryIndex = 0;
        for (const auto& factionPair : charPair.second) {
            std::string entryPrefix = prefix + "faction" + std::to_string(factionEntryIndex) + "_";
            writer.Write(entryPrefix + "id", factionPair.first);
            writer.Write(entryPrefix + "value", factionPair.second);
            factionEntryIndex++;
        }
        
        characterIndex++;
    }
    
    LOG(Info, "FactionSystem serialized to text");
}

void FactionSystem::DeserializeFromText(TextReader& reader) {
    // Clear existing data
    m_factions.clear();
    m_reputations.clear();
    
    // Read default values
    reader.Read("defaultReputation", m_defaultReputation);
    reader.Read("defaultFactionRelation", m_defaultFactionRelation);
    
    // Read faction count
    int factionCount = 0;
    reader.Read("factionCount", factionCount);
    
    // Read each faction
    for (int i = 0; i < factionCount; ++i) {
        Faction faction;
        std::string prefix = "faction" + std::to_string(i) + "_";
        
        reader.Read(prefix + "id", faction.id);
        reader.Read(prefix + "name", faction.name);
        reader.Read(prefix + "description", faction.description);
        
        // Read faction relationships
        int relationCount = 0;
        reader.Read(prefix + "relationCount", relationCount);
        
        for (int j = 0; j < relationCount; ++j) {
            std::string relPrefix = prefix + "rel" + std::to_string(j) + "_";
            std::string targetId;
            int value = 0;
            
            reader.Read(relPrefix + "targetId", targetId);
            reader.Read(relPrefix + "value", value);
            
            faction.relations[targetId] = value;
        }
        
        m_factions[faction.id] = faction;
    }
    
    // Read character reputation count
    int characterCount = 0;
    reader.Read("characterCount", characterCount);
    
    // Read each character's reputations
    for (int i = 0; i < characterCount; ++i) {
        std::string prefix = "character" + std::to_string(i) + "_";
        std::string characterId;
        
        reader.Read(prefix + "id", characterId);
        
        int factionEntryCount = 0;
        reader.Read(prefix + "factionCount", factionEntryCount);
        
        std::unordered_map<std::string, int> reputations;
        
        for (int j = 0; j < factionEntryCount; ++j) {
            std::string entryPrefix = prefix + "faction" + std::to_string(j) + "_";
            std::string factionId;
            int value = 0;
            
            reader.Read(entryPrefix + "id", factionId);
            reader.Read(entryPrefix + "value", value);
            
            reputations[factionId] = value;
        }
        
        m_reputations[characterId] = reputations;
    }
    
    LOG(Info, "FactionSystem deserialized from text");
}
// ^ FactionSystem.cpp