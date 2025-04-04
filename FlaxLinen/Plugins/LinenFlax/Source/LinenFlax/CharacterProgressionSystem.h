// v CharacterProgressionSystem.h
#pragma once

#include "QuestEvents.h"
#include "RPGSystem.h"
#include "Serialization.h"


#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


class Skill {
public:
    Skill(const std::string& id, const std::string& name, const std::string& description);

    std::string GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    std::string GetDescription() const { return m_description; }

    int GetLevel() const { return m_level; }
    void SetLevel(int level) { m_level = level; }
    void IncreaseLevel(int amount = 1) { m_level += amount; }

    void Serialize(BinaryWriter& writer) const;
    void Deserialize(BinaryReader& reader);
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);

private:
    std::string m_id;
    std::string m_name;
    std::string m_description;
    int m_level = 0;
};

class CharacterProgressionSystem : public RPGSystem {
public:
    // Delete copy constructor and assignment operator
    CharacterProgressionSystem(const CharacterProgressionSystem&) = delete;
    CharacterProgressionSystem& operator=(const CharacterProgressionSystem&) = delete;

    // Meyer's Singleton - thread-safe in C++11 and beyond
    static CharacterProgressionSystem* GetInstance()
    {
        // Thread-safe in C++11 and beyond
        static CharacterProgressionSystem* instance = new CharacterProgressionSystem();
        return instance;
    }

    // Cleanup method
    static void Destroy()
    {
        static CharacterProgressionSystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }

    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;

    // Implement GetName from LinenSystem
    std::string GetName() const override { return "CharacterProgressionSystem"; }

    // Skill management
    bool AddSkill(const std::string& id, const std::string& name, const std::string& description);
    bool IncreaseSkill(const std::string& id, int amount = 1);
    int GetSkillLevel(const std::string& id) const;

    // Requirements checking
    const std::unordered_map<std::string, int>& GetSkills() const;

    // Experience management
    void GainExperience(int amount);
    int GetExperience() const;
    int GetLevel() const;

    // Serialization override
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);

    ~CharacterProgressionSystem();

private:
    // Private constructor
    CharacterProgressionSystem();

    // Event handlers
    void HandleQuestCompleted(const QuestCompletedEvent& event);

    // Character data
    int m_experience = 0;
    int m_level = 1;
    std::unordered_map<std::string, std::unique_ptr<Skill>> m_skills;
    std::unordered_map<std::string, int> m_skillLevels; // Cache for requirements checking
};
// ^ CharacterProgressionSystem.h