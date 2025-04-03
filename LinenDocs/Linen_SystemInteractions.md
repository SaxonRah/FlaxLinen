### **System Interactions in Linen**

Linen is designed to ensure that various RPG systems interact seamlessly while maintaining their modularity. Each system operates independently but communicates via shared events, messaging, or direct API calls when necessary. This allows for emergent gameplay where one system's state dynamically influences another.

### **Examples of System Interactions**
Here are a few examples of how Linen's RPG systems can interact:

1. **Quest System & Character Progression**
   - A player gains a skill that unlocks a new quest.
   - Completing a quest grants experience, which improves the player's abilities.

2. **Faction Reputation & Economy**
   - A player's standing with a faction influences shop prices.
   - Donating to a faction affects market supply and demand.

3. **Weather & NPC Behavior**
   - Rainy weather causes NPCs to stay indoors.
   - A storm delays trade routes, affecting market prices.

4. **Crime & NPC Relationship**
   - Committing a crime reduces faction reputation.
   - NPCs who witnessed a crime may refuse to trade or offer quests.

---

### **Example: Quest System & Character Progression Interaction**

The **Quest System** and **Character Progression System** interact such that certain quests have skill-based prerequisites, and completing quests grants experience, which affects character progression.

#### **Defining an Event System for Interactions**
To avoid tight coupling between systems, we use an **event-driven approach** where the **QuestSystem** triggers an event that the **CharacterProgressionSystem** listens to.

---

#### **Event System (`EventBus.h`)**
```cpp
#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

class EventBus
{
public:
    template <typename EventType>
    using EventHandler = std::function<void(const EventType&)>;

    template <typename EventType>
    static void Subscribe(EventHandler<EventType> handler)
    {
        GetHandlers<EventType>().push_back(handler);
    }

    template <typename EventType>
    static void Publish(const EventType& event)
    {
        for (const auto& handler : GetHandlers<EventType>())
        {
            handler(event);
        }
    }

private:
    template <typename EventType>
    static std::vector<EventHandler<EventType>>& GetHandlers()
    {
        static std::vector<EventHandler<EventType>> handlers;
        return handlers;
    }
};
```

---

#### **Defining the Quest Completion Event (`QuestEvents.h`)**
```cpp
#pragma once
#include <string>

struct QuestCompletedEvent
{
    std::string QuestName;
    int ExperienceGained;
};
```

---

#### **Quest System Publishing the Event (`QuestSystem.cpp`)**
```cpp
#include "QuestSystem.h"
#include "EventBus.h"
#include "QuestEvents.h"

void QuestSystem::CompleteQuest(const std::string& title)
{
    for (auto& quest : Quests)
    {
        if (quest.Title == title && quest.QuestState == Quest::State::Active)
        {
            quest.QuestState = Quest::State::Completed;
            LOG(Info, "Quest completed: {0}", String(title.c_str()));

            // Publish the event to notify other systems
            EventBus::Publish(QuestCompletedEvent{title, 100}); // Example XP reward
            return;
        }
    }
    LOG(Warning, "Quest not found or not active: {0}", String(title.c_str()));
}
```

---

#### **Character Progression System Subscribing to the Event (`CharacterProgressionSystem.cpp`)**
```cpp
#include "CharacterProgressionSystem.h"
#include "EventBus.h"
#include "QuestEvents.h"

void CharacterProgressionSystem::Initialize()
{
    EventBus::Subscribe<QuestCompletedEvent>([this](const QuestCompletedEvent& event)
    {
        GainExperience(event.ExperienceGained);
    });

    LOG(Info, "Character Progression System Initialized.");
}

void CharacterProgressionSystem::GainExperience(int amount)
{
    Experience += amount;
    LOG(Info, "Gained {0} XP. Total XP: {1}", amount, Experience);
}
```

---

### **How the Interaction Works**
1. When a **quest is completed**, the `QuestSystem` **publishes** a `QuestCompletedEvent`.
2. The `CharacterProgressionSystem` **listens** for this event and responds by adding experience to the player.
3. Both systems remain modular—neither **directly references** the other, making it easy to modify or replace either system independently.

---

### **Expanding System Interactions**
By following this **event-driven approach**, other interactions can be implemented, such as:
- **Factions affecting dialogue options** (`FactionReputationChangedEvent`).
- **Weather influencing NPC schedules** (`WeatherStateChangedEvent`).
- **Crime affecting economy** (`CrimeCommittedEvent`).

This architecture ensures that each system remains **self-contained yet highly interactive**, creating a **rich, dynamic RPG experience**.

---

### **Linen System Interaction Mapping**

This section provides a **complete interaction mapping** of all Linen RPG systems, detailing how they influence each other. Systems interact through **event-driven architecture** (as demonstrated earlier) to keep them modular while enabling deep interconnected gameplay.

---

### **System Interaction Overview**
The following matrix summarizes the relationships between Linen's systems. An **"X"** indicates direct interaction.

| **System**                 | **Character Progression** | **Quest** | **Dialogue** | **Time** | **Save/Load** | **Weather** | **Economy** | **World Progression** | **NPC Relations** | **Factions** | **Crime** | **Property** | **Mounts** | **Disease** | **Crafting** | **Religion** |
|----------------------------|--------------------------|-----------|-------------|----------|--------------|------------|------------|----------------|---------------|-----------|--------|-----------|---------|---------|----------|-----------|
| **Character Progression**   | Self  | X (XP Rewards) | X (Dialogue unlocks) |   |   |   |   |   | X (Stats affect NPC opinion) | X (Faction-restricted skills) |   |   |   |   | X (Crafting perks) | X (Divine abilities) |
| **Quest System**           | X (XP Rewards) | Self | X (Quest dialogues) | X (Timed Quests) | X (Saved Quest States) |   | X (Economic quest rewards) | X (World unlocks) | X (NPC requests) | X (Faction-based quests) | X (Crime quests) | X (Property-based quests) |   |   | X (Crafting quests) | X (Religious quests) |
| **Dialogue System**        | X (Stat-based responses) | X (Quest-related) | Self |   | X (Save dialogue history) |   |   |   | X (Relationship-based options) | X (Faction-dependent) |   |   |   |   |   | X (Religious dialogue) |
| **Time System**            |   | X (Timed Quests) |   | Self | X (Save current time) | X (Seasonal weather changes) |   | X (Time-based unlocks) | X (NPC routines) |   | X (Bounty decay) | X (Rent cycles) | X (Horse aging) | X (Disease progression) |   | X (Religious holidays) |
| **Save/Load System**       |   | X (Save quest states) | X (Dialogue history) | X (Save current time) | Self | X (Weather state) | X (Market state) | X (World progression) | X (NPC relations) | X (Faction standing) | X (Bounty level) | X (Property ownership) | X (Mount state) | X (Disease status) | X (Crafted items) | X (Religious standing) |
| **Weather System**         |   |   |   | X (Seasonal effects) | X (Save weather state) | Self | X (Market disruptions) | X (Weather-blocked paths) |   |   |   |   |   |   |   |   |
| **Economy System**         |   | X (Quest-based rewards) |   |   | X (Save market state) | X (Weather impacts) | Self | X (Economic region growth) |   | X (Faction economies) | X (Fines and bribery) | X (Property taxation) | X (Mount pricing) |   | X (Crafting material costs) |   |
| **World Progression**      |   | X (Unlock new locations) |   | X (Time-based area changes) | X (Save world state) | X (Weather zones) | X (Market expansion) | Self |   | X (Faction influence) |   | X (Property zoning) |   |   |   |   |
| **NPC Relationship System**| X (Stats affect opinion) | X (NPC-given quests) | X (NPC response changes) | X (NPC daily schedules) | X (Save NPC relationships) |   |   |   | Self | X (Faction-based opinions) | X (Crime impacts reputation) | X (Tenant relationships) |   |   |   |   |
| **Faction Reputation**     | X (Faction-restricted skills) | X (Faction-based quests) | X (Faction-dependent dialogue) |   | X (Save faction standing) |   | X (Faction economies) | X (Faction-controlled areas) | X (Faction NPCs) | Self | X (Criminal factions) | X (Faction-controlled property) |   |   |   | X (Religious factions) |
| **Crime & Law System**     |   | X (Crime-based quests) |   | X (Bounty decay over time) | X (Save crime status) |   | X (Bribery, stolen goods) | X (Restricted areas) | X (NPC crime witnessing) | X (Faction police) | Self | X (Criminal hideouts) |   |   |   |   |
| **Property Ownership**     |   | X (Property-related quests) |   | X (Rent due cycles) | X (Save property state) |   | X (Real estate pricing) | X (Property unlocks new areas) | X (Tenant relationships) | X (Faction-controlled areas) | X (Criminal safehouses) | Self |   |   |   |   |
| **Mount System**           |   |   |   | X (Mount aging over time) | X (Save mount status) |   | X (Mount prices fluctuate) |   |   |   |   |   | Self |   |   |   |
| **Disease & Health**       |   |   |   | X (Disease progression) | X (Save disease status) |   |   |   |   |   |   |   |   | Self |   |   |
| **Crafting System**        | X (Crafting perks) | X (Crafting-related quests) |   |   | X (Save crafted items) |   | X (Market prices for crafting goods) |   |   |   |   |   |   |   | Self |   |
| **Religion & Deity**       | X (Divine blessings) | X (Religious quests) | X (Faith-based dialogue) | X (Religious festivals) | X (Save devotion levels) |   |   |   |   | X (Religious factions) |   |   |   |   |   | Self |

---

### **🔗 Interaction Examples**
1. **Crime → Faction Reputation → NPC Relations**
   - If the player commits a crime, their faction standing drops.
   - NPCs associated with that faction may refuse to speak to the player.
   - Guards (crime system) may attempt to arrest them.

2. **Time → Economy → World Progression**
   - A **drought (weather)** reduces crop production.
   - **Economy system** raises food prices due to shortages.
   - **World progression system** triggers migration of NPCs to wealthier cities.

3. **Quest System → Character Progression → Crafting**
   - A quest rewards a **new crafting recipe**.
   - The **crafting system** allows creation of unique weapons.
   - The **character progression system** grants perks that improve crafting efficiency.

---

### **📌 Conclusion**
By structuring Linen as an event-driven, modular RPG system, all these systems dynamically interact **without hard dependencies**. Developers can enable/disable interactions as needed, ensuring **scalability and flexibility** for custom RPG experiences.
