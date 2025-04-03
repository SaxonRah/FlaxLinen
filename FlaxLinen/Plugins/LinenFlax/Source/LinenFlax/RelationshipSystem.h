// v RelationshipSystem.h
#pragma once

#include "RPGSystem.h"
#include "EventSystem.h"
#include "Serialization.h"
#include <string>
#include <unordered_map>
#include <vector>

// Relationship-related events
class RelationshipChangedEvent : public EventType<RelationshipChangedEvent> {
public:
    std::string characterId;
    std::string targetId;
    int previousValue;
    int newValue;
};

// Enum for relationship levels
enum class RelationshipLevel {
    Hostile = -2,
    Unfriendly = -1,
    Neutral = 0,
    Friendly = 1,
    Allied = 2
};

class RelationshipSystem : public RPGSystem {
public:
    // Delete copy constructor and assignment operator
    RelationshipSystem(const RelationshipSystem&) = delete;
    RelationshipSystem& operator=(const RelationshipSystem&) = delete;
    
    // Meyer's Singleton pattern
    static RelationshipSystem* GetInstance() {
        static RelationshipSystem* instance = new RelationshipSystem();
        return instance;
    }
    
    // Cleanup method
    static void Destroy() {
        static RelationshipSystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }
    
    ~RelationshipSystem();

    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetName() const override { return "RelationshipSystem"; }
    
    // Serialization
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);
    
    // Relationship methods
    void SetRelationship(const std::string& characterId, const std::string& targetId, int value);
    void ModifyRelationship(const std::string& characterId, const std::string& targetId, int delta);
    int GetRelationship(const std::string& characterId, const std::string& targetId) const;
    RelationshipLevel GetRelationshipLevel(const std::string& characterId, const std::string& targetId) const;
    
    // Character methods
    bool RegisterCharacter(const std::string& characterId, const std::string& name);
    bool UnregisterCharacter(const std::string& characterId);
    bool IsCharacterRegistered(const std::string& characterId) const;
    std::string GetCharacterName(const std::string& characterId) const;
    
    // Default values
    void SetDefaultRelationship(int value) { m_defaultRelationship = value; }
    int GetDefaultRelationship() const { return m_defaultRelationship; }
    
    // Query methods
    std::vector<std::string> GetAllCharacterIds() const;
    std::vector<std::string> GetAlliedCharacters(const std::string& characterId, int minValue = 50) const;
    std::vector<std::string> GetHostileCharacters(const std::string& characterId, int maxValue = -50) const;

private:
    // Private constructor
    RelationshipSystem();
    
    // Data structures
    struct Character {
        std::string id;
        std::string name;
        std::unordered_map<std::string, int> relationships;
    };
    
    // Character storage
    std::unordered_map<std::string, Character> m_characters;
    
    // Default relationship value when none is set
    int m_defaultRelationship = 0;
    
    // Helper methods
    RelationshipLevel ValueToLevel(int value) const;
};
// ^ RelationshipSystem.h