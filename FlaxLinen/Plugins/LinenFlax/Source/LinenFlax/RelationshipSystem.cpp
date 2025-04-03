// v RelationshipSystem.cpp
#include "RelationshipSystem.h"
#include "LinenFlax.h"
#include "Engine/Core/Log.h"

RelationshipSystem::RelationshipSystem() {
    // Define system dependencies
    m_dependencies.insert("CharacterProgressionSystem");
}

RelationshipSystem::~RelationshipSystem() {
    Destroy();
}

void RelationshipSystem::Initialize() {
    LOG(Info, "Relationship System Initialized.");
}

void RelationshipSystem::Shutdown() {
    m_characters.clear();
    LOG(Info, "Relationship System Shutdown.");
}

void RelationshipSystem::Update(float deltaTime) {
    // Nothing to update regularly
}

bool RelationshipSystem::RegisterCharacter(const std::string& characterId, const std::string& name) {
    if (m_characters.find(characterId) != m_characters.end()) {
        LOG(Warning, "Character already registered: {0}", String(characterId.c_str()));
        return false;
    }
    
    Character character;
    character.id = characterId;
    character.name = name;
    
    m_characters[characterId] = character;
    LOG(Info, "Registered character: {0} ({1})", String(name.c_str()), String(characterId.c_str()));
    return true;
}

bool RelationshipSystem::UnregisterCharacter(const std::string& characterId) {
    auto it = m_characters.find(characterId);
    if (it == m_characters.end()) {
        LOG(Warning, "Character not registered: {0}", String(characterId.c_str()));
        return false;
    }
    
    // Remove from all other characters' relationship maps
    for (auto& pair : m_characters) {
        pair.second.relationships.erase(characterId);
    }
    
    m_characters.erase(it);
    LOG(Info, "Unregistered character: {0}", String(characterId.c_str()));
    return true;
}

bool RelationshipSystem::IsCharacterRegistered(const std::string& characterId) const {
    return m_characters.find(characterId) != m_characters.end();
}

std::string RelationshipSystem::GetCharacterName(const std::string& characterId) const {
    auto it = m_characters.find(characterId);
    if (it == m_characters.end()) {
        return "";
    }
    return it->second.name;
}

void RelationshipSystem::SetRelationship(const std::string& characterId, const std::string& targetId, int value) {
    // Check if both characters are registered
    if (!IsCharacterRegistered(characterId)) {
        LOG(Warning, "Source character not registered: {0}", String(characterId.c_str()));
        return;
    }
    
    if (!IsCharacterRegistered(targetId)) {
        LOG(Warning, "Target character not registered: {0}", String(targetId.c_str()));
        return;
    }
    
    // Clamp value between -100 and 100
    value = value < -100 ? -100 : (value > 100 ? 100 : value);
    
    Character& character = m_characters[characterId];
    int previousValue = GetRelationship(characterId, targetId);
    character.relationships[targetId] = value;
    
    // Notify about relationship change
    RelationshipChangedEvent event;
    event.characterId = characterId;
    event.targetId = targetId;
    event.previousValue = previousValue;
    event.newValue = value;
    
    m_plugin->GetEventSystem().Publish(event);
    
    LOG(Info, "Set relationship: {0} -> {1} = {2}", 
        String(characterId.c_str()), String(targetId.c_str()), value);
}

void RelationshipSystem::ModifyRelationship(const std::string& characterId, const std::string& targetId, int delta) {
    if (delta == 0) return; // No change
    
    int currentValue = GetRelationship(characterId, targetId);
    SetRelationship(characterId, targetId, currentValue + delta);
}

int RelationshipSystem::GetRelationship(const std::string& characterId, const std::string& targetId) const {
    // If either character doesn't exist, return default
    if (!IsCharacterRegistered(characterId) || !IsCharacterRegistered(targetId)) {
        return m_defaultRelationship;
    }
    
    // If characters are the same, return maximum relationship
    if (characterId == targetId) {
        return 100;
    }
    
    const auto& character = m_characters.at(characterId);
    auto it = character.relationships.find(targetId);
    
    if (it == character.relationships.end()) {
        return m_defaultRelationship;
    }
    
    return it->second;
}

RelationshipLevel RelationshipSystem::GetRelationshipLevel(const std::string& characterId, const std::string& targetId) const {
    int value = GetRelationship(characterId, targetId);
    return ValueToLevel(value);
}

RelationshipLevel RelationshipSystem::ValueToLevel(int value) const {
    if (value <= -75) return RelationshipLevel::Hostile;
    if (value <= -25) return RelationshipLevel::Unfriendly;
    if (value < 25) return RelationshipLevel::Neutral;
    if (value < 75) return RelationshipLevel::Friendly;
    return RelationshipLevel::Allied;
}

std::vector<std::string> RelationshipSystem::GetAllCharacterIds() const {
    std::vector<std::string> result;
    result.reserve(m_characters.size());
    
    for (const auto& pair : m_characters) {
        result.push_back(pair.first);
    }
    
    return result;
}

std::vector<std::string> RelationshipSystem::GetAlliedCharacters(const std::string& characterId, int minValue) const {
    std::vector<std::string> result;
    
    if (!IsCharacterRegistered(characterId)) {
        return result;
    }
    
    const auto& character = m_characters.at(characterId);
    
    for (const auto& pair : m_characters) {
        if (pair.first == characterId) continue; // Skip self
        
        int relation = GetRelationship(characterId, pair.first);
        
        if (relation >= minValue) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

std::vector<std::string> RelationshipSystem::GetHostileCharacters(const std::string& characterId, int maxValue) const {
    std::vector<std::string> result;
    
    if (!IsCharacterRegistered(characterId)) {
        return result;
    }
    
    for (const auto& pair : m_characters) {
        if (pair.first == characterId) continue; // Skip self
        
        int relation = GetRelationship(characterId, pair.first);
        
        if (relation <= maxValue) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

void RelationshipSystem::Serialize(BinaryWriter& writer) const {
    // Write default relationship
    writer.Write(m_defaultRelationship);
    
    // Write character count
    writer.Write(static_cast<uint32_t>(m_characters.size()));
    
    // Write each character
    for (const auto& pair : m_characters) {
        const Character& character = pair.second;
        
        // Write character ID and name
        writer.Write(character.id);
        writer.Write(character.name);
        
        // Write relationships count
        writer.Write(static_cast<uint32_t>(character.relationships.size()));
        
        // Write each relationship
        for (const auto& rel : character.relationships) {
            writer.Write(rel.first);  // Target ID
            writer.Write(rel.second); // Relationship value
        }
    }
    
    LOG(Info, "RelationshipSystem serialized");
}

void RelationshipSystem::Deserialize(BinaryReader& reader) {
    // Clear existing data
    m_characters.clear();
    
    // Read default relationship
    reader.Read(m_defaultRelationship);
    
    // Read character count
    uint32_t characterCount = 0;
    reader.Read(characterCount);
    
    // Read each character
    for (uint32_t i = 0; i < characterCount; ++i) {
        Character character;
        
        // Read character ID and name
        reader.Read(character.id);
        reader.Read(character.name);
        
        // Read relationships count
        uint32_t relationshipCount = 0;
        reader.Read(relationshipCount);
        
        // Read each relationship
        for (uint32_t j = 0; j < relationshipCount; ++j) {
            std::string targetId;
            int value = 0;
            
            reader.Read(targetId);
            reader.Read(value);
            
            character.relationships[targetId] = value;
        }
        
        m_characters[character.id] = character;
    }
    
    LOG(Info, "RelationshipSystem deserialized");
}

void RelationshipSystem::SerializeToText(TextWriter& writer) const {
    // Write default relationship
    writer.Write("defaultRelationship", m_defaultRelationship);
    
    // Write character count
    writer.Write("characterCount", static_cast<int>(m_characters.size()));
    
    // Write each character
    int index = 0;
    for (const auto& pair : m_characters) {
        const Character& character = pair.second;
        std::string prefix = "character" + std::to_string(index) + "_";
        
        // Write character ID and name
        writer.Write(prefix + "id", character.id);
        writer.Write(prefix + "name", character.name);
        
        // Write relationships count
        writer.Write(prefix + "relationshipCount", static_cast<int>(character.relationships.size()));
        
        // Write each relationship
        int relIndex = 0;
        for (const auto& rel : character.relationships) {
            std::string relPrefix = prefix + "rel" + std::to_string(relIndex) + "_";
            writer.Write(relPrefix + "targetId", rel.first);
            writer.Write(relPrefix + "value", rel.second);
            relIndex++;
        }
        
        index++;
    }
    
    LOG(Info, "RelationshipSystem serialized to text");
}

void RelationshipSystem::DeserializeFromText(TextReader& reader) {
    // Clear existing data
    m_characters.clear();
    
    // Read default relationship
    reader.Read("defaultRelationship", m_defaultRelationship);
    
    // Read character count
    int characterCount = 0;
    reader.Read("characterCount", characterCount);
    
    // Read each character
    for (int i = 0; i < characterCount; ++i) {
        Character character;
        std::string prefix = "character" + std::to_string(i) + "_";
        
        // Read character ID and name
        reader.Read(prefix + "id", character.id);
        reader.Read(prefix + "name", character.name);
        
        // Read relationships count
        int relationshipCount = 0;
        reader.Read(prefix + "relationshipCount", relationshipCount);
        
        // Read each relationship
        for (int j = 0; j < relationshipCount; ++j) {
            std::string relPrefix = prefix + "rel" + std::to_string(j) + "_";
            std::string targetId;
            int value = 0;
            
            reader.Read(relPrefix + "targetId", targetId);
            reader.Read(relPrefix + "value", value);
            
            character.relationships[targetId] = value;
        }
        
        m_characters[character.id] = character;
    }
    
    LOG(Info, "RelationshipSystem deserialized from text");
}
// ^ RelationshipSystem.cpp