// v QuestSystem.cpp
#include "QuestSystem.h"
#include "CharacterProgressionSystem.h"
#include "LinenFlax.h"
#include "Engine/Core/Log.h"

// QuestSystem* QuestSystem::s_instance = nullptr;

Quest::Quest(const std::string& id, const std::string& title, const std::string& description)
    : m_id(id)
    , m_title(title)
    , m_description(description)
    , m_state(QuestState::Available)
    , m_experienceReward(0)
{
}


void Quest::AddSkillRequirement(const std::string& skillName, int requiredLevel) {
    m_skillRequirements[skillName] = requiredLevel;
}

bool Quest::CheckRequirements(const std::unordered_map<std::string, int>& playerSkills) const {
    for (const auto& req : m_skillRequirements) {
        auto it = playerSkills.find(req.first);
        if (it == playerSkills.end() || it->second < req.second) {
            return false;
        }
    }
    return true;
}

void Quest::Serialize(BinaryWriter& writer) const {
    writer.Write(m_id);
    writer.Write(m_title);
    writer.Write(m_description);
    writer.Write(static_cast<int32_t>(m_state));
    writer.Write(m_experienceReward);
    
    // Write skill requirements
    writer.Write(static_cast<uint32_t>(m_skillRequirements.size()));
    for (const auto& pair : m_skillRequirements) {
        writer.Write(pair.first);  // Skill name
        writer.Write(pair.second); // Required level
    }
}

void Quest::Deserialize(BinaryReader& reader) {
    reader.Read(m_id);
    reader.Read(m_title);
    reader.Read(m_description);
    
    int32_t stateValue = 0;
    reader.Read(stateValue);
    m_state = static_cast<QuestState>(stateValue);
    
    reader.Read(m_experienceReward);
    
    // Read skill requirements
    uint32_t requirementCount = 0;
    reader.Read(requirementCount);
    
    m_skillRequirements.clear();
    for (uint32_t i = 0; i < requirementCount; ++i) {
        std::string skillName;
        int requiredLevel = 0;
        reader.Read(skillName);
        reader.Read(requiredLevel);
        m_skillRequirements[skillName] = requiredLevel;
    }
}

void Quest::SerializeToText(TextWriter& writer) const {
    writer.Write("questId", m_id);
    writer.Write("questTitle", m_title);
    writer.Write("questDescription", m_description);
    writer.Write("questState", static_cast<int>(m_state));
    writer.Write("questExperienceReward", m_experienceReward);
    
    // Write skill requirements count
    writer.Write("questSkillReqCount", static_cast<int>(m_skillRequirements.size()));
    
    // Write each skill requirement
    int index = 0;
    for (const auto& pair : m_skillRequirements) {
        std::string prefix = "questSkillReq" + std::to_string(index) + "_";
        writer.Write(prefix + "skill", pair.first);
        writer.Write(prefix + "level", pair.second);
        index++;
    }
}

// Quest text deserialization
void Quest::DeserializeFromText(TextReader& reader) {
    reader.Read("questId", m_id);
    reader.Read("questTitle", m_title);
    reader.Read("questDescription", m_description);
    
    int state = 0;
    reader.Read("questState", state);
    m_state = static_cast<QuestState>(state);
    
    reader.Read("questExperienceReward", m_experienceReward);
    
    // Read skill requirements
    int reqCount = 0;
    reader.Read("questSkillReqCount", reqCount);
    
    m_skillRequirements.clear();
    for (int i = 0; i < reqCount; i++) {
        std::string prefix = "questSkillReq" + std::to_string(i) + "_";
        
        std::string skillName;
        int requiredLevel = 0;
        
        reader.Read(prefix + "skill", skillName);
        reader.Read(prefix + "level", requiredLevel);
        
        m_skillRequirements[skillName] = requiredLevel;
    }
}

QuestSystem::QuestSystem() {
    // Define system dependencies
    m_dependencies.insert("CharacterProgressionSystem");
}

QuestSystem::~QuestSystem() {
    Destroy();
    // Shutdown();
}

void QuestSystem::Initialize() {
    LOG(Info, "Quest System Initialized.");
}

void QuestSystem::Shutdown() {
    m_quests.clear();
    LOG(Info, "Quest System Shutdown.");
}

void QuestSystem::Update(float deltaTime) {
    // Check for time-based quest updates
}

QuestResult QuestSystem::AddQuest(const std::string& id, const std::string& title, const std::string& description) {
    
    if (m_quests.find(id) != m_quests.end()) {
        LOG(Warning, "Quest already exists: {0}", String(id.c_str()));
        return QuestResult::AlreadyExists;
    }
    
    try {
        auto quest = std::make_unique<Quest>(id, title, description);
        m_quests[id] = std::move(quest);
        
        LOG(Info, "Added quest: {0}", String(title.c_str()));
        return QuestResult::Success;
    }
    catch (const std::exception& e) {
        LOG(Error, "Failed to add quest: {0}. Error: {1}", 
            String(id.c_str()), String(e.what()));
        return QuestResult::Error;
    }
}

QuestResult QuestSystem::ActivateQuest(const std::string& id) {
    auto it = m_quests.find(id);
    if (it == m_quests.end()) {
        LOG(Warning, "Quest not found: {0}", String(id.c_str()));
        return QuestResult::NotFound;
    }

    Quest* quest = it->second.get();
    if (quest->GetState() != QuestState::Available) {
        LOG(Warning, "Quest not available: {0}", String(id.c_str()));
        return QuestResult::InvalidState;
    }

    // Check character progression requirements
    auto* progressionSystem = m_plugin->GetSystem<CharacterProgressionSystem>();

    if (progressionSystem) {
        if (!quest->CheckRequirements(progressionSystem->GetSkills())) {
            LOG(Info, "Character doesn't meet quest requirements: {0}", String(id.c_str()));
            return QuestResult::RequirementsNotMet;
        }
    }

    // Store old state and update to new state
    QuestState oldState = quest->GetState();
    quest->SetState(QuestState::Active);

    // Create and publish event
    QuestStateChangedEvent event;
    event.questId = id;
    event.questTitle = quest->GetTitle();
    event.oldState = oldState;
    event.newState = QuestState::Active;
    m_plugin->GetEventSystem().Publish(event);
    
    LOG(Info, "Activated quest: {0}", String(id.c_str()));
    return QuestResult::Success;
}

QuestResult QuestSystem::CompleteQuest(const std::string& id) {
    std::string questTitle;
    int experienceReward = 0;
    QuestState oldState;
    bool success = false;
    
        
    auto it = m_quests.find(id);
    if (it == m_quests.end()) {
        LOG(Warning, "Quest not found: {0}", String(id.c_str()));
        return QuestResult::NotFound;
    }
    
    Quest* quest = it->second.get();
    if (quest->GetState() != QuestState::Active) {
        LOG(Warning, "Quest not active: {0}", String(id.c_str()));
        return QuestResult::InvalidState;
    }
    
    oldState = quest->GetState();
    questTitle = quest->GetTitle();
    experienceReward = quest->GetExperienceReward();
    
    quest->SetState(QuestState::Completed);
    success = true;
    
    if (success) {
        // Completion event
        QuestCompletedEvent completedEvent;
        completedEvent.questId = id;
        completedEvent.questTitle = questTitle;
        completedEvent.experienceGained = experienceReward;
        m_plugin->GetEventSystem().Publish(completedEvent);
        
        // State change event 
        QuestStateChangedEvent stateEvent;
        stateEvent.questId = id;
        stateEvent.questTitle = questTitle;
        stateEvent.oldState = oldState;
        stateEvent.newState = QuestState::Completed;
        m_plugin->GetEventSystem().Publish(stateEvent);

        LOG(Info, "Completed quest: {0}", String(id.c_str()));
        return QuestResult::Success;
    }
    
    return QuestResult::Error;
}

QuestResult QuestSystem::FailQuest(const std::string& id) {
    std::string questTitle;
    QuestState oldState;
    bool success = false;
    
    auto it = m_quests.find(id);
    if (it == m_quests.end()) {
        LOG(Warning, "Quest not found: {0}", String(id.c_str()));
        return QuestResult::NotFound;
    }
    
    Quest* quest = it->second.get();
    if (quest->GetState() != QuestState::Active) {
        LOG(Warning, "Quest not active: {0}", String(id.c_str()));
        return QuestResult::InvalidState;
    }
    
    oldState = quest->GetState();
    questTitle = quest->GetTitle();
    quest->SetState(QuestState::Failed);
    success = true;

    if (success) {
        QuestStateChangedEvent event;
        event.questId = id;
        event.questTitle = questTitle;
        event.oldState = oldState;
        event.newState = QuestState::Failed;
        m_plugin->GetEventSystem().Publish(event);

        LOG(Info, "Failed quest: {0}", String(id.c_str()));
        return QuestResult::Success;
    }
    
    return QuestResult::Error;
}

Quest* QuestSystem::GetQuest(const std::string& id) {    
    auto it = m_quests.find(id);
    if (it == m_quests.end()) { return nullptr; }
    return it->second.get();
}

std::vector<Quest*> QuestSystem::GetAvailableQuests() const {
    std::vector<Quest*> result;
    for (const auto& pair : m_quests) {
        if (pair.second->GetState() == QuestState::Available) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

std::vector<Quest*> QuestSystem::GetActiveQuests() const {
    std::vector<Quest*> result;
    for (const auto& pair : m_quests) {
        if (pair.second->GetState() == QuestState::Active) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

std::vector<Quest*> QuestSystem::GetCompletedQuests() const {
    std::vector<Quest*> result;
    for (const auto& pair : m_quests) {
        if (pair.second->GetState() == QuestState::Completed) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

std::vector<Quest*> QuestSystem::GetFailedQuests() const {
    std::vector<Quest*> result;
    for (const auto& pair : m_quests) {
        if (pair.second->GetState() == QuestState::Failed) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

void QuestSystem::Serialize(BinaryWriter& writer) const {
    // Write quests
    writer.Write(static_cast<uint32_t>(m_quests.size()));
    for (const auto& pair : m_quests) {
        writer.Write(pair.first);  // Quest ID
        
        // Write quest data
        writer.Write(pair.second->GetId());
        writer.Write(pair.second->GetTitle());
        writer.Write(pair.second->GetDescription());
        writer.Write(static_cast<int32_t>(pair.second->GetState()));
        writer.Write(pair.second->GetExperienceReward());
        
        // Write skill requirements
        auto quest = pair.second.get();
        const auto& reqMap = quest->GetSkillRequirements();
        writer.Write(static_cast<uint32_t>(reqMap.size()));
        
        for (const auto& reqPair : reqMap) {
            writer.Write(reqPair.first);  // Skill name
            writer.Write(reqPair.second); // Required level
        }
    }
    
    LOG(Info, "QuestSystem serialized");
}

void QuestSystem::Deserialize(BinaryReader& reader) {
    // Clear existing data
    m_quests.clear();
    
    // Read quests
    uint32_t questCount = 0;
    reader.Read(questCount);
    
    for (uint32_t i = 0; i < questCount; ++i) {
        std::string questId;
        reader.Read(questId);
        
        // Read quest data
        std::string id, title, description;
        int32_t stateValue = 0;
        int expReward = 0;
        
        reader.Read(id);
        reader.Read(title);
        reader.Read(description);
        reader.Read(stateValue);
        reader.Read(expReward);
        
        // Create quest
        auto quest = std::make_unique<Quest>(id, title, description);
        quest->SetState(static_cast<QuestState>(stateValue));
        quest->SetExperienceReward(expReward);
        
        // Read skill requirements
        uint32_t reqCount = 0;
        reader.Read(reqCount);
        
        for (uint32_t j = 0; j < reqCount; ++j) {
            std::string skillName;
            int requiredLevel = 0;
            reader.Read(skillName);
            reader.Read(requiredLevel);
            quest->AddSkillRequirement(skillName, requiredLevel);
        }
        
        m_quests[questId] = std::move(quest);
    }
    
    LOG(Info, "QuestSystem deserialized");
}

void QuestSystem::SerializeToText(TextWriter& writer) const {
    // Write quest count
    writer.Write("questCount", static_cast<int>(m_quests.size()));
    
    // Write each quest
    int index = 0;
    for (const auto& pair : m_quests) {
        std::string prefix = "quest" + std::to_string(index) + "_";
        writer.Write(prefix + "id", pair.first);
        
        // Let the quest serialize itself with the prefix
        pair.second->SerializeToText(writer);
        
        index++;
    }
    
    LOG(Info, "QuestSystem serialized to text");
}

void QuestSystem::DeserializeFromText(TextReader& reader) {
    // Clear existing data
    m_quests.clear();
    
    // Read quest count
    int questCount = 0;
    reader.Read("questCount", questCount);
    
    // Read each quest
    for (int i = 0; i < questCount; i++) {
        std::string prefix = "quest" + std::to_string(i) + "_";
        
        std::string questId;
        reader.Read(prefix + "id", questId);
        
        // Create a placeholder quest
        auto quest = std::make_unique<Quest>("", "", "");
        
        // Let the quest deserialize itself
        quest->DeserializeFromText(reader);
        
        m_quests[questId] = std::move(quest);
    }
    
    LOG(Info, "QuestSystem deserialized from text");
}
// ^ QuestSystem.cpp