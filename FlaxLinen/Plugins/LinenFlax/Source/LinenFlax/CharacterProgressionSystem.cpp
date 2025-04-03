// v CharacterProgressionSystem.cpp
#include "CharacterProgressionSystem.h"
#include "QuestEvents.h"
#include "LinenFlax.h"
#include "Engine/Core/Log.h"

Skill::Skill(const std::string& id, const std::string& name, const std::string& description)
    : m_id(id)
    , m_name(name)
    , m_description(description)
    , m_level(0)
{
}
// Skill binary serialization
void Skill::Serialize(BinaryWriter& writer) const {
    writer.Write(m_id);
    writer.Write(m_name);
    writer.Write(m_description);
    writer.Write(m_level);
}

// Skill binary deserialization
void Skill::Deserialize(BinaryReader& reader) {
    reader.Read(m_id);
    reader.Read(m_name);
    reader.Read(m_description);
    reader.Read(m_level);
}

// Skill text serialization
void Skill::SerializeToText(TextWriter& writer) const {
    writer.Write("skillId", m_id);
    writer.Write("skillName", m_name);
    writer.Write("skillDescription", m_description);
    writer.Write("skillLevel", m_level);
}

// Skill text deserialization
void Skill::DeserializeFromText(TextReader& reader) {
    reader.Read("skillId", m_id);
    reader.Read("skillName", m_name);
    reader.Read("skillDescription", m_description);
    reader.Read("skillLevel", m_level);
}

CharacterProgressionSystem::CharacterProgressionSystem() {
    // Initialize member variables if needed
    m_experience = 0;
    m_level = 1;
}

CharacterProgressionSystem::~CharacterProgressionSystem() {
    Destroy();
    // Shutdown();
}

void CharacterProgressionSystem::Initialize() {
    // Subscribe to quest completed events
    m_plugin->GetEventSystem().Subscribe<QuestCompletedEvent>(
        [this](const QuestCompletedEvent& event) {
            this->HandleQuestCompleted(event);
        });
    
    LOG(Info, "Character Progression System Initialized.");
}

void CharacterProgressionSystem::Shutdown() {
    m_skills.clear();
    m_skillLevels.clear();
    LOG(Info, "Character Progression System Shutdown.");
}

void CharacterProgressionSystem::Update(float deltaTime) {
    // Nothing to update regularly
}

bool CharacterProgressionSystem::AddSkill(const std::string& id, const std::string& name, const std::string& description) {    
    if (m_skills.find(id) != m_skills.end()) {
        LOG(Warning, "Skill already exists: {0}", String(id.c_str()));
        return false;
    }
    
    auto skill = std::make_unique<Skill>(id, name, description);
    m_skills[id] = std::move(skill);
    m_skillLevels[id] = 0;
    
    LOG(Info, "Added skill: {0}", String(name.c_str()));
    return true;
}

bool CharacterProgressionSystem::IncreaseSkill(const std::string& id, int amount) {    
    auto it = m_skills.find(id);
    if (it == m_skills.end()) {
        LOG(Warning, "Skill not found: {0}", String(id.c_str()));
        return false;
    }
    
    it->second->IncreaseLevel(amount);
    m_skillLevels[id] = it->second->GetLevel();
    
    LOG(Info, "Increased skill {0} by {1} to level {2}", 
        String(id.c_str()), amount, it->second->GetLevel());
    return true;
}

int CharacterProgressionSystem::GetSkillLevel(const std::string& id) const {    
    auto it = m_skills.find(id);
    if (it == m_skills.end()) {
        return 0;
    }    
    return it->second->GetLevel();
}

void CharacterProgressionSystem::GainExperience(int amount) {
    int oldLevel = m_level;
    m_experience += amount;
    
    // Simple level up formula: level = sqrt(experience / 100)
    m_level = 1 + static_cast<int>(sqrt(m_experience / 100.0));
    
    LOG(Info, "Gained {0} XP. Total XP: {1}", amount, m_experience);
    
    if (m_level > oldLevel) {
        LOG(Info, "Level up! New level: {0}", m_level);
    }
}

int CharacterProgressionSystem::GetExperience() const {
    return m_experience;
}

int CharacterProgressionSystem::GetLevel() const {
    return m_level;
}

const std::unordered_map<std::string, int>& CharacterProgressionSystem::GetSkills() const {
    return m_skillLevels;
}

void CharacterProgressionSystem::HandleQuestCompleted(const QuestCompletedEvent& event) {
    if (event.experienceGained > 0) {
        GainExperience(event.experienceGained);
        LOG(Info, "Gained {0} XP from completed quest: {1}", 
            event.experienceGained, String(event.questTitle.c_str()));
    }
}

void CharacterProgressionSystem::Serialize(BinaryWriter& writer) const {
    // Write basic character data
    writer.Write(m_experience);
    writer.Write(m_level);
    
    // Write skills
    writer.Write(static_cast<uint32_t>(m_skills.size()));
    for (const auto& pair : m_skills) {
        writer.Write(pair.first);  // Skill ID
        writer.Write(pair.second->GetId());
        writer.Write(pair.second->GetName());
        writer.Write(pair.second->GetDescription());
        writer.Write(pair.second->GetLevel());
    }
    
    // Write skill levels cache
    writer.Write(static_cast<uint32_t>(m_skillLevels.size()));
    for (const auto& pair : m_skillLevels) {
        writer.Write(pair.first);  // Skill ID
        writer.Write(pair.second); // Skill level
    }
    
    LOG(Info, "CharacterProgressionSystem serialized");
}

// CharacterProgressionSystem binary deserialization
void CharacterProgressionSystem::Deserialize(BinaryReader& reader) {
    // Clear existing data
    m_skills.clear();
    m_skillLevels.clear();
    
    // Read basic character data
    reader.Read(m_experience);
    reader.Read(m_level);
    
    // Read skills
    uint32_t skillCount = 0;
    reader.Read(skillCount);
    
    for (uint32_t i = 0; i < skillCount; ++i) {
        std::string skillId;
        reader.Read(skillId);
        
        std::string id, name, description;
        int level;
        reader.Read(id);
        reader.Read(name);
        reader.Read(description);
        reader.Read(level);
        
        auto skill = std::make_unique<Skill>(id, name, description);
        skill->SetLevel(level);
        
        m_skills[skillId] = std::move(skill);
    }
    
    // Read skill levels cache
    uint32_t levelCount = 0;
    reader.Read(levelCount);
    
    for (uint32_t i = 0; i < levelCount; ++i) {
        std::string skillId;
        int level = 0;
        reader.Read(skillId);
        reader.Read(level);
        m_skillLevels[skillId] = level;
    }
    
    LOG(Info, "CharacterProgressionSystem deserialized");
}

// CharacterProgressionSystem text serialization
void CharacterProgressionSystem::SerializeToText(TextWriter& writer) const {
    // Write basic character data
    writer.Write("characterExperience", m_experience);
    writer.Write("characterLevel", m_level);
    
    // Write skill count
    writer.Write("skillCount", static_cast<int>(m_skills.size()));
    
    // Write each skill
    int index = 0;
    for (const auto& pair : m_skills) {
        std::string prefix = "skill" + std::to_string(index) + "_";
        writer.Write(prefix + "id", pair.first);
        writer.Write(prefix + "name", pair.second->GetName());
        writer.Write(prefix + "description", pair.second->GetDescription());
        writer.Write(prefix + "level", pair.second->GetLevel());
        index++;
    }
    
    // Write skill levels count
    writer.Write("skillLevelsCount", static_cast<int>(m_skillLevels.size()));
    
    // Write skill levels
    index = 0;
    for (const auto& pair : m_skillLevels) {
        std::string prefix = "skillLevel" + std::to_string(index) + "_";
        writer.Write(prefix + "id", pair.first);
        writer.Write(prefix + "level", pair.second);
        index++;
    }
    
    LOG(Info, "CharacterProgressionSystem serialized to text");
}

void CharacterProgressionSystem::DeserializeFromText(TextReader& reader) {
    // Clear existing data
    m_skills.clear();
    m_skillLevels.clear();
    
    // Read basic character data
    reader.Read("characterExperience", m_experience);
    reader.Read("characterLevel", m_level);
    
    // Read skill count
    int skillCount = 0;
    reader.Read("skillCount", skillCount);
    
    // Read each skill
    for (int i = 0; i < skillCount; i++) {
        std::string prefix = "skill" + std::to_string(i) + "_";
        
        std::string id, name, description;
        int level = 0;
        
        reader.Read(prefix + "id", id);
        reader.Read(prefix + "name", name);
        reader.Read(prefix + "description", description);
        reader.Read(prefix + "level", level);
        
        auto skill = std::make_unique<Skill>(id, name, description);
        skill->SetLevel(level);
        
        m_skills[id] = std::move(skill);
    }
    
    // Read skill levels count
    int skillLevelsCount = 0;
    reader.Read("skillLevelsCount", skillLevelsCount);
    
    // Read skill levels
    for (int i = 0; i < skillLevelsCount; i++) {
        std::string prefix = "skillLevel" + std::to_string(i) + "_";
        
        std::string id;
        int level = 0;
        
        reader.Read(prefix + "id", id);
        reader.Read(prefix + "level", level);
        
        m_skillLevels[id] = level;
    }
    
    LOG(Info, "CharacterProgressionSystem deserialized from text");
}
// ^ CharacterProgressionSystem.cpp