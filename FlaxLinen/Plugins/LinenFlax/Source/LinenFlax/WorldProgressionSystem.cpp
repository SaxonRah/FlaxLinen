// v WorldProgressionSystem.cpp
#include "WorldProgressionSystem.h"
#include "LinenFlax.h"
#include "TimeSystem.h"
#include "Engine/Core/Log.h"
#include <ctime>
#include <algorithm>

// WorldRegion methods
void WorldRegion::AddFactionPresence(const std::string& factionId, float strength) {
    if (strength <= 0.0f) {
        m_factionPresence.erase(factionId);
        return;
    }
    
    m_factionPresence[factionId] = strength;
}

void WorldRegion::RemoveFactionPresence(const std::string& factionId) {
    m_factionPresence.erase(factionId);
}

float WorldRegion::GetFactionPresence(const std::string& factionId) const {
    auto it = m_factionPresence.find(factionId);
    if (it != m_factionPresence.end()) {
        return it->second;
    }
    return 0.0f;
}

void WorldRegion::Serialize(BinaryWriter& writer) const {
    writer.Write(m_id);
    writer.Write(m_name);
    writer.Write(static_cast<int32_t>(m_state));
    writer.Write(static_cast<int32_t>(m_population));
    writer.Write(m_prosperity);
    writer.Write(m_stability);
    writer.Write(m_playerInfluence);
    writer.Write(m_danger);
    writer.Write(m_controllingFaction);
    
    // Write faction presences
    writer.Write(static_cast<uint32_t>(m_factionPresence.size()));
    for (const auto& pair : m_factionPresence) {
        writer.Write(pair.first);
        writer.Write(pair.second);
    }
    
    // Write connected regions
    writer.Write(static_cast<uint32_t>(m_connectedRegions.size()));
    for (const auto& regionId : m_connectedRegions) {
        writer.Write(regionId);
    }
}

void WorldRegion::Deserialize(BinaryReader& reader) {
    reader.Read(m_id);
    reader.Read(m_name);
    
    int32_t stateValue;
    reader.Read(stateValue);
    m_state = static_cast<RegionState>(stateValue);
    
    int32_t populationValue;
    reader.Read(populationValue);
    m_population = populationValue;
    
    reader.Read(m_prosperity);
    reader.Read(m_stability);
    reader.Read(m_playerInfluence);
    reader.Read(m_danger);
    reader.Read(m_controllingFaction);
    
    // Read faction presences
    uint32_t presenceCount;
    reader.Read(presenceCount);
    
    m_factionPresence.clear();
    for (uint32_t i = 0; i < presenceCount; ++i) {
        std::string factionId;
        float strength;
        
        reader.Read(factionId);
        reader.Read(strength);
        
        m_factionPresence[factionId] = strength;
    }
    
    // Read connected regions
    uint32_t regionCount;
    reader.Read(regionCount);
    
    m_connectedRegions.clear();
    for (uint32_t i = 0; i < regionCount; ++i) {
        std::string regionId;
        reader.Read(regionId);
        m_connectedRegions.insert(regionId);
    }
}

void WorldRegion::SerializeToText(TextWriter& writer) const {
    writer.Write("regionId", m_id);
    writer.Write("regionName", m_name);
    writer.Write("regionState", static_cast<int>(m_state));
    writer.Write("regionPopulation", m_population);
    writer.Write("regionProsperity", m_prosperity);
    writer.Write("regionStability", m_stability);
    writer.Write("regionPlayerInfluence", m_playerInfluence);
    writer.Write("regionDanger", m_danger);
    writer.Write("regionControllingFaction", m_controllingFaction);
    
    // Write faction presences count
    writer.Write("regionFactionCount", static_cast<int>(m_factionPresence.size()));
    
    // Write each faction presence
    int index = 0;
    for (const auto& pair : m_factionPresence) {
        std::string prefix = "regionFaction" + std::to_string(index) + "_";
        writer.Write(prefix + "id", pair.first);
        writer.Write(prefix + "strength", pair.second);
        index++;
    }
    
    // Write connected regions count
    writer.Write("regionConnectionCount", static_cast<int>(m_connectedRegions.size()));
    
    // Write each connection
    index = 0;
    for (const auto& regionId : m_connectedRegions) {
        writer.Write("regionConnection" + std::to_string(index), regionId);
        index++;
    }
}

void WorldRegion::DeserializeFromText(TextReader& reader) {
    reader.Read("regionId", m_id);
    reader.Read("regionName", m_name);
    
    int stateValue;
    if (reader.Read("regionState", stateValue))
        m_state = static_cast<RegionState>(stateValue);
    
    reader.Read("regionPopulation", m_population);
    reader.Read("regionProsperity", m_prosperity);
    reader.Read("regionStability", m_stability);
    reader.Read("regionPlayerInfluence", m_playerInfluence);
    reader.Read("regionDanger", m_danger);
    reader.Read("regionControllingFaction", m_controllingFaction);
    
    // Read faction presences
    int presenceCount = 0;
    reader.Read("regionFactionCount", presenceCount);
    
    m_factionPresence.clear();
    for (int i = 0; i < presenceCount; i++) {
        std::string prefix = "regionFaction" + std::to_string(i) + "_";
        
        std::string factionId;
        float strength = 0.0f;
        
        reader.Read(prefix + "id", factionId);
        reader.Read(prefix + "strength", strength);
        
        m_factionPresence[factionId] = strength;
    }
    
    // Read connected regions
    int connectionCount = 0;
    reader.Read("regionConnectionCount", connectionCount);
    
    m_connectedRegions.clear();
    for (int i = 0; i < connectionCount; i++) {
        std::string regionId;
        reader.Read("regionConnection" + std::to_string(i), regionId);
        m_connectedRegions.insert(regionId);
    }
}

// WorldFaction methods
void WorldFaction::SetRelationship(const std::string& otherFactionId, float value) {
    // Clamp value to valid range
    value = std::max(-1.0f, std::min(1.0f, value));
    m_relationships[otherFactionId] = value;
}

float WorldFaction::GetRelationship(const std::string& otherFactionId) const {
    auto it = m_relationships.find(otherFactionId);
    if (it != m_relationships.end()) {
        return it->second;
    }
    return 0.0f; // Neutral by default
}

FactionRelationship WorldFaction::GetRelationshipStatus(const std::string& otherFactionId) const {
    float value = GetRelationship(otherFactionId);
    
    if (value >= 0.75f) return FactionRelationship::Allied;
    if (value >= 0.25f) return FactionRelationship::Friendly;
    if (value >= -0.25f) return FactionRelationship::Neutral;
    if (value >= -0.75f) return FactionRelationship::Unfriendly;
    if (value > -1.0f) return FactionRelationship::Hostile;
    return FactionRelationship::AtWar;
}

void WorldFaction::Serialize(BinaryWriter& writer) const {
    writer.Write(m_id);
    writer.Write(m_name);
    writer.Write(m_power);
    writer.Write(m_aggression);
    writer.Write(m_expansionism);
    writer.Write(m_playerRelationship);
    
    // Write relationships
    writer.Write(static_cast<uint32_t>(m_relationships.size()));
    for (const auto& pair : m_relationships) {
        writer.Write(pair.first);
        writer.Write(pair.second);
    }
}

void WorldFaction::Deserialize(BinaryReader& reader) {
    reader.Read(m_id);
    reader.Read(m_name);
    reader.Read(m_power);
    reader.Read(m_aggression);
    reader.Read(m_expansionism);
    reader.Read(m_playerRelationship);
    
    // Read relationships
    uint32_t relationshipCount;
    reader.Read(relationshipCount);
    
    m_relationships.clear();
    for (uint32_t i = 0; i < relationshipCount; ++i) {
        std::string factionId;
        float value;
        
        reader.Read(factionId);
        reader.Read(value);
        
        m_relationships[factionId] = value;
    }
}

void WorldFaction::SerializeToText(TextWriter& writer) const {
    writer.Write("factionId", m_id);
    writer.Write("factionName", m_name);
    writer.Write("factionPower", m_power);
    writer.Write("factionAggression", m_aggression);
    writer.Write("factionExpansionism", m_expansionism);
    writer.Write("factionPlayerRelationship", m_playerRelationship);
    
    // Write relationships count
    writer.Write("factionRelationshipCount", static_cast<int>(m_relationships.size()));
    
    // Write each relationship
    int index = 0;
    for (const auto& pair : m_relationships) {
        std::string prefix = "factionRelation" + std::to_string(index) + "_";
        writer.Write(prefix + "id", pair.first);
        writer.Write(prefix + "value", pair.second);
        index++;
    }
}

void WorldFaction::DeserializeFromText(TextReader& reader) {
    reader.Read("factionId", m_id);
    reader.Read("factionName", m_name);
    reader.Read("factionPower", m_power);
    reader.Read("factionAggression", m_aggression);
    reader.Read("factionExpansionism", m_expansionism);
    reader.Read("factionPlayerRelationship", m_playerRelationship);
    
    // Read relationships
    int relationshipCount = 0;
    reader.Read("factionRelationshipCount", relationshipCount);
    
    m_relationships.clear();
    for (int i = 0; i < relationshipCount; i++) {
        std::string prefix = "factionRelation" + std::to_string(i) + "_";
        
        std::string factionId;
        float value = 0.0f;
        
        reader.Read(prefix + "id", factionId);
        reader.Read(prefix + "value", value);
        
        m_relationships[factionId] = value;
    }
}

// WorldEvent methods
void WorldEvent::AddRegionEffect(const std::string& regionId, float stabilityChange, float prosperityChange, float dangerChange) {
    m_regionEffects[regionId] = std::make_tuple(stabilityChange, prosperityChange, dangerChange);
}

bool WorldEvent::HasEffectForRegion(const std::string& regionId) const {
    return m_regionEffects.find(regionId) != m_regionEffects.end();
}

void WorldEvent::GetRegionEffects(const std::string& regionId, float& stabilityChange, float& prosperityChange, float& dangerChange) const {
    auto it = m_regionEffects.find(regionId);
    if (it != m_regionEffects.end()) {
        stabilityChange = std::get<0>(it->second);
        prosperityChange = std::get<1>(it->second);
        dangerChange = std::get<2>(it->second);
    } else {
        stabilityChange = 0.0f;
        prosperityChange = 0.0f;
        dangerChange = 0.0f;
    }
}

void WorldEvent::AddFactionEffect(const std::string& factionId, float powerChange, float relationshipChange) {
    m_factionEffects[factionId] = std::make_pair(powerChange, relationshipChange);
}

bool WorldEvent::HasEffectForFaction(const std::string& factionId) const {
    return m_factionEffects.find(factionId) != m_factionEffects.end();
}

void WorldEvent::GetFactionEffects(const std::string& factionId, float& powerChange, float& relationshipChange) const {
    auto it = m_factionEffects.find(factionId);
    if (it != m_factionEffects.end()) {
        powerChange = it->second.first;
        relationshipChange = it->second.second;
    } else {
        powerChange = 0.0f;
        relationshipChange = 0.0f;
    }
}

void WorldEvent::Serialize(BinaryWriter& writer) const {
    writer.Write(m_id);
    writer.Write(m_name);
    writer.Write(m_description);
    writer.Write(m_weight);
    writer.Write(m_cooldown);
    writer.Write(m_lastTriggerTime);
    
    // Write region effects
    writer.Write(static_cast<uint32_t>(m_regionEffects.size()));
    for (const auto& pair : m_regionEffects) {
        writer.Write(pair.first);
        writer.Write(std::get<0>(pair.second)); // Stability change
        writer.Write(std::get<1>(pair.second)); // Prosperity change
        writer.Write(std::get<2>(pair.second)); // Danger change
    }
    
    // Write faction effects
    writer.Write(static_cast<uint32_t>(m_factionEffects.size()));
    for (const auto& pair : m_factionEffects) {
        writer.Write(pair.first);
        writer.Write(pair.second.first);  // Power change
        writer.Write(pair.second.second); // Relationship change
    }
}

void WorldEvent::Deserialize(BinaryReader& reader) {
    reader.Read(m_id);
    reader.Read(m_name);
    reader.Read(m_description);
    reader.Read(m_weight);
    reader.Read(m_cooldown);
    reader.Read(m_lastTriggerTime);
    
    // Read region effects
    uint32_t regionEffectCount;
    reader.Read(regionEffectCount);
    
    m_regionEffects.clear();
    for (uint32_t i = 0; i < regionEffectCount; ++i) {
        std::string regionId;
        float stabilityChange;
        float prosperityChange;
        float dangerChange;
        
        reader.Read(regionId);
        reader.Read(stabilityChange);
        reader.Read(prosperityChange);
        reader.Read(dangerChange);
        
        m_regionEffects[regionId] = std::make_tuple(stabilityChange, prosperityChange, dangerChange);
    }
    
    // Read faction effects
    uint32_t factionEffectCount;
    reader.Read(factionEffectCount);
    
    m_factionEffects.clear();
    for (uint32_t i = 0; i < factionEffectCount; ++i) {
        std::string factionId;
        float powerChange;
        float relationshipChange;
        
        reader.Read(factionId);
        reader.Read(powerChange);
        reader.Read(relationshipChange);
        
        m_factionEffects[factionId] = std::make_pair(powerChange, relationshipChange);
    }
}

void WorldEvent::SerializeToText(TextWriter& writer) const {
    writer.Write("eventId", m_id);
    writer.Write("eventName", m_name);
    writer.Write("eventDescription", m_description);
    writer.Write("eventWeight", m_weight);
    writer.Write("eventCooldown", m_cooldown);
    writer.Write("eventLastTrigger", m_lastTriggerTime);
    
    // Write region effects count
    writer.Write("eventRegionEffectCount", static_cast<int>(m_regionEffects.size()));
    
    // Write each region effect
    int regionIndex = 0;
    for (const auto& pair : m_regionEffects) {
        std::string prefix = "eventRegionEffect" + std::to_string(regionIndex) + "_";
        writer.Write(prefix + "id", pair.first);
        writer.Write(prefix + "stability", std::get<0>(pair.second));
        writer.Write(prefix + "prosperity", std::get<1>(pair.second));
        writer.Write(prefix + "danger", std::get<2>(pair.second));
        regionIndex++;
    }
    
    // Write faction effects count
    writer.Write("eventFactionEffectCount", static_cast<int>(m_factionEffects.size()));
    
    // Write each faction effect
    int factionIndex = 0;
    for (const auto& pair : m_factionEffects) {
        std::string prefix = "eventFactionEffect" + std::to_string(factionIndex) + "_";
        writer.Write(prefix + "id", pair.first);
        writer.Write(prefix + "power", pair.second.first);
        writer.Write(prefix + "relation", pair.second.second);
        factionIndex++;
    }
}

void WorldEvent::DeserializeFromText(TextReader& reader) {
    reader.Read("eventId", m_id);
    reader.Read("eventName", m_name);
    reader.Read("eventDescription", m_description);
    reader.Read("eventWeight", m_weight);
    reader.Read("eventCooldown", m_cooldown);
    reader.Read("eventLastTrigger", m_lastTriggerTime);
    
    // Read region effects
    int regionEffectCount = 0;
    reader.Read("eventRegionEffectCount", regionEffectCount);
    
    m_regionEffects.clear();
    for (int i = 0; i < regionEffectCount; i++) {
        std::string prefix = "eventRegionEffect" + std::to_string(i) + "_";
        
        std::string regionId;
        float stabilityChange = 0.0f;
        float prosperityChange = 0.0f;
        float dangerChange = 0.0f;
        
        reader.Read(prefix + "id", regionId);
        reader.Read(prefix + "stability", stabilityChange);
        reader.Read(prefix + "prosperity", prosperityChange);
        reader.Read(prefix + "danger", dangerChange);
        
        m_regionEffects[regionId] = std::make_tuple(stabilityChange, prosperityChange, dangerChange);
    }
    
    // Read faction effects
    int factionEffectCount = 0;
    reader.Read("eventFactionEffectCount", factionEffectCount);
    
    m_factionEffects.clear();
    for (int i = 0; i < factionEffectCount; i++) {
        std::string prefix = "eventFactionEffect" + std::to_string(i) + "_";
        
        std::string factionId;
        float powerChange = 0.0f;
        float relationshipChange = 0.0f;
        
        reader.Read(prefix + "id", factionId);
        reader.Read(prefix + "power", powerChange);
        reader.Read(prefix + "relation", relationshipChange);
        
        m_factionEffects[factionId] = std::make_pair(powerChange, relationshipChange);
    }
}

// WorldProgressionSystem methods
WorldProgressionSystem::WorldProgressionSystem() {
    // Define system dependencies
    m_dependencies.insert("TimeSystem");
    
    // Initialize random generator with a time-based seed
    m_randomGenerator.seed(static_cast<unsigned int>(std::time(nullptr)));
}

WorldProgressionSystem::~WorldProgressionSystem() {
    Destroy();
}

void WorldProgressionSystem::Initialize() {
    // Subscribe to day changed events
    if (m_plugin) {
        m_plugin->GetEventSystem().Subscribe<DayChangedEvent>(
            [this](const DayChangedEvent& event) {
                // Simulate world changes on day change
                SimulateWorld();
            });
    }
    
    LOG(Info, "World Progression System Initialized.");
}

void WorldProgressionSystem::Shutdown() {
    m_regions.clear();
    m_factions.clear();
    m_worldEvents.clear();
    LOG(Info, "World Progression System Shutdown.");
}

void WorldProgressionSystem::Update(float deltaTime) {
    // Accumulate game time
    auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
    if (timeSystem) {
        float scaledDelta = deltaTime * timeSystem->GetTimeScale();
        m_gameTime += scaledDelta;
        
        // Accumulate time since last simulation
        m_timeSinceLastSimulation += scaledDelta;
        
        // Check if we should run a simulation
        if (m_timeSinceLastSimulation >= m_worldSimulationInterval) {
            m_timeSinceLastSimulation = 0.0f;
            SimulateWorld();
        }
    }
}

bool WorldProgressionSystem::AddRegion(const std::string& id, const std::string& name) {
    if (m_regions.find(id) != m_regions.end()) {
        LOG(Warning, "Region with ID {0} already exists", String(id.c_str()));
        return false;
    }
    
    auto region = std::make_unique<WorldRegion>(id, name);
    m_regions[id] = std::move(region);
    
    LOG(Info, "Added region: {0} ({1})", String(name.c_str()), String(id.c_str()));
    return true;
}

WorldRegion* WorldProgressionSystem::GetRegion(const std::string& id) {
    auto it = m_regions.find(id);
    if (it == m_regions.end()) {
        return nullptr;
    }
    return it->second.get();
}

void WorldProgressionSystem::SetRegionState(const std::string& regionId, RegionState state) {
    WorldRegion* region = GetRegion(regionId);
    if (region) {
        RegionState oldState = region->GetState();
        
        if (oldState != state) {
            region->SetState(state);
            
            // Adjust region attributes based on new state
            AdjustRegionByState(region);
            
            // Fire event
            RegionChangedEvent event;
            event.regionId = regionId;
            event.oldState = RegionStateToString(oldState);
            event.newState = RegionStateToString(state);
            event.isPlayerInfluenced = (region->GetPlayerInfluence() > 0.3f);
            
            m_plugin->GetEventSystem().Publish(event);
            
            LOG(Info, "Region {0} state changed from {1} to {2}",
                String(region->GetName().c_str()),
                String(RegionStateToString(oldState).c_str()),
                String(RegionStateToString(state).c_str()));
        }
    } else {
        LOG(Warning, "Cannot set state for nonexistent region: {0}", String(regionId.c_str()));
    }
}

void WorldProgressionSystem::ConnectRegions(const std::string& region1Id, const std::string& region2Id) {
    if (region1Id == region2Id) {
        LOG(Warning, "Cannot connect region to itself: {0}", String(region1Id.c_str()));
        return;
    }
    
    WorldRegion* region1 = GetRegion(region1Id);
    WorldRegion* region2 = GetRegion(region2Id);
    
    if (!region1 || !region2) {
        LOG(Warning, "Cannot connect regions - one or both regions not found");
        return;
    }
    
    // Connect in both directions
    region1->AddConnectedRegion(region2Id);
    region2->AddConnectedRegion(region1Id);
    
    LOG(Info, "Regions connected: {0} <-> {1}",
        String(region1->GetName().c_str()),
        String(region2->GetName().c_str()));
}

bool WorldProgressionSystem::AddFaction(const std::string& id, const std::string& name) {
    if (m_factions.find(id) != m_factions.end()) {
        LOG(Warning, "Faction with ID {0} already exists", String(id.c_str()));
        return false;
    }
    
    auto faction = std::make_unique<WorldFaction>(id, name);
    m_factions[id] = std::move(faction);
    
    LOG(Info, "Added faction: {0} ({1})", String(name.c_str()), String(id.c_str()));
    return true;
}

WorldFaction* WorldProgressionSystem::GetFaction(const std::string& id) {
    auto it = m_factions.find(id);
    if (it == m_factions.end()) {
        return nullptr;
    }
    return it->second.get();
}

void WorldProgressionSystem::SetFactionRelationship(const std::string& faction1Id, const std::string& faction2Id, float value) {
    if (faction1Id == faction2Id) {
        LOG(Warning, "Cannot set relationship between a faction and itself: {0}", String(faction1Id.c_str()));
        return;
    }
    
    WorldFaction* faction1 = GetFaction(faction1Id);
    WorldFaction* faction2 = GetFaction(faction2Id);
    
    if (!faction1 || !faction2) {
        LOG(Warning, "Cannot set relationship - one or both factions not found");
        return;
    }
    
    // Get old values
    float oldValue1 = faction1->GetRelationship(faction2Id);
    float oldValue2 = faction2->GetRelationship(faction1Id);
    
    // Set new values (bidirectional)
    faction1->SetRelationship(faction2Id, value);
    faction2->SetRelationship(faction1Id, value);
    
    // Check if relationship status changed
    FactionRelationship oldStatus1 = RelationshipValueToStatus(oldValue1);
    FactionRelationship newStatus1 = RelationshipValueToStatus(value);
    
    if (oldStatus1 != newStatus1) {
        // Fire event
        FactionRelationChangedEvent event;
        event.faction1Id = faction1Id;
        event.faction2Id = faction2Id;
        event.oldRelation = oldValue1;
        event.newRelation = value;
        event.relationStatus = FactionRelationshipToString(newStatus1);
        
        m_plugin->GetEventSystem().Publish(event);
        
        LOG(Info, "Relationship changed between {0} and {1}: {2} -> {3} ({4})",
            String(faction1->GetName().c_str()),
            String(faction2->GetName().c_str()),
            oldValue1, value,
            String(FactionRelationshipToString(newStatus1).c_str()));
    }
}

void WorldProgressionSystem::ModifyPlayerInfluence(const std::string& regionId, float change) {
    WorldRegion* region = GetRegion(regionId);
    if (region) {
        float oldInfluence = region->GetPlayerInfluence();
        float newInfluence = std::max(0.0f, std::min(1.0f, oldInfluence + change));
        
        region->SetPlayerInfluence(newInfluence);
        
        LOG(Info, "Player influence in {0} changed: {1:0.2f} -> {2:0.2f}",
            String(region->GetName().c_str()),
            oldInfluence, newInfluence);
            
        // If influence changed significantly, may need to update region controller
        if (std::abs(newInfluence - oldInfluence) > 0.2f) {
            UpdateRegionController(region);
        }
    } else {
        LOG(Warning, "Cannot modify player influence for nonexistent region: {0}", String(regionId.c_str()));
    }
}

bool WorldProgressionSystem::AddWorldEvent(const std::string& id, const std::string& name, const std::string& description) {
    if (m_worldEvents.find(id) != m_worldEvents.end()) {
        LOG(Warning, "World event with ID {0} already exists", String(id.c_str()));
        return false;
    }
    
    auto event = std::make_unique<WorldEvent>(id, name, description);
    m_worldEvents[id] = std::move(event);
    
    LOG(Info, "Added world event: {0} ({1})", String(name.c_str()), String(id.c_str()));
    return true;
}

WorldEvent* WorldProgressionSystem::GetWorldEvent(const std::string& id) {
    auto it = m_worldEvents.find(id);
    if (it == m_worldEvents.end()) {
        return nullptr;
    }
    return it->second.get();
}

bool WorldProgressionSystem::TriggerWorldEvent(const std::string& eventId, const std::string& regionId) {
    WorldEvent* event = GetWorldEvent(eventId);
    WorldRegion* region = GetRegion(regionId);
    
    if (!event || !region) {
        LOG(Warning, "Cannot trigger event - event or region not found");
        return false;
    }
    
    // Check if event is on cooldown
    if (event->IsOnCooldown(m_gameTime)) {
        LOG(Warning, "Event {0} is on cooldown", String(event->GetName().c_str()));
        return false;
    }
    
    LOG(Info, "Triggering world event: {0} in region {1}",
        String(event->GetName().c_str()),
        String(region->GetName().c_str()));
    
    // Update event trigger time
    event->SetLastTriggerTime(m_gameTime);
    
    // Apply event effects on region
    float stabilityChange = 0.0f;
    float prosperityChange = 0.0f;
    float dangerChange = 0.0f;
    
    if (event->HasEffectForRegion(regionId)) {
        event->GetRegionEffects(regionId, stabilityChange, prosperityChange, dangerChange);
    } else {
        // Random effects if no specific ones defined
        std::uniform_real_distribution<float> dist(-0.2f, 0.2f);
        stabilityChange = dist(m_randomGenerator);
        prosperityChange = dist(m_randomGenerator);
        dangerChange = dist(m_randomGenerator);
    }
    
    // Apply effects
    region->SetStability(region->GetStability() + stabilityChange);
    region->SetProsperity(region->GetProsperity() + prosperityChange);
    region->SetDanger(region->GetDanger() + dangerChange);
    
    // Adjust population based on event impact
    float populationChange = (stabilityChange + prosperityChange - dangerChange) * 100.0f;
    region->SetPopulation(region->GetPopulation() + static_cast<int>(populationChange));
    
    // Apply faction effects
    for (auto& pair : m_factions) {
        float powerChange = 0.0f;
        float relationshipChange = 0.0f;
        
        if (event->HasEffectForFaction(pair.first)) {
            event->GetFactionEffects(pair.first, powerChange, relationshipChange);
            
            // Apply power change
            pair.second->SetPower(pair.second->GetPower() + powerChange);
            
            // Apply relationship changes to all other factions
            for (auto& otherPair : m_factions) {
                if (pair.first != otherPair.first) {
                    float oldRelation = pair.second->GetRelationship(otherPair.first);
                    pair.second->SetRelationship(otherPair.first, oldRelation + relationshipChange);
                }
            }
        }
    }
    
    // Fire event
    WorldEventTriggeredEvent triggerEvent;
    triggerEvent.eventId = eventId;
    triggerEvent.eventName = event->GetName();
    triggerEvent.regionId = regionId;
    triggerEvent.description = event->GetDescription();
    triggerEvent.affectsPlayer = (region->GetPlayerInfluence() > 0.1f);
    
    m_plugin->GetEventSystem().Publish(triggerEvent);
    
    // Recalculate region state if needed
    if (std::abs(stabilityChange) > 0.1f || std::abs(dangerChange) > 0.1f) {
        RegionState newState = CalculateRegionState(region);
        if (newState != region->GetState()) {
            SetRegionState(regionId, newState);
        }
    }
    
    return true;
}

RegionState WorldProgressionSystem::CalculateRegionState(WorldRegion* region) {
    if (!region) return RegionState::Peaceful;
    
    float stability = region->GetStability();
    float danger = region->GetDanger();
    
    if (stability < 0.2f && danger > 0.8f) {
        return RegionState::Abandoned;
    } else if (danger > 0.8f) {
        return RegionState::Warzone;
    } else if (danger > 0.5f) {
        return RegionState::Dangerous;
    } else if (stability < 0.3f) {
        return RegionState::Troubled;
    } else if (stability < 0.5f && region->GetState() == RegionState::Abandoned) {
        return RegionState::Rebuilding;
    } else if (stability >= 0.5f) {
        return RegionState::Peaceful;
    }
    
    // Default to current state if no clear change needed
    return region->GetState();
}

RegionState WorldProgressionSystem::GetRegionState(const std::string& regionId) const {
    auto it = m_regions.find(regionId);
    if (it != m_regions.end()) {
        return it->second->GetState();
    }
    return RegionState::Peaceful;
}

float WorldProgressionSystem::GetFactionRelationship(const std::string& faction1Id, const std::string& faction2Id) const {
    auto it = m_factions.find(faction1Id);
    if (it != m_factions.end()) {
        return it->second->GetRelationship(faction2Id);
    }
    return 0.0f; // Neutral by default
}

std::vector<std::string> WorldProgressionSystem::GetRegionsControlledByFaction(const std::string& factionId) const {
    std::vector<std::string> controlledRegions;
    
    for (const auto& pair : m_regions) {
        if (pair.second->GetControllingFaction() == factionId) {
            controlledRegions.push_back(pair.first);
        }
    }
    
    return controlledRegions;
}

void WorldProgressionSystem::SimulateWorld() {
    LOG(Info, "Simulating world changes");
    
    // Simulate each region
    for (auto& pair : m_regions) {
        SimulateRegion(pair.second.get());
    }
    
    // Simulate faction behaviors
    SimulateFactions();
    
    // Simulate conflicts between factions
    SimulateFactionConflicts();
    
    // Try to trigger random world events
    AttemptRegionalEvents();
    
    LOG(Info, "World simulation complete");
}

void WorldProgressionSystem::SimulateRegion(WorldRegion* region) {
    if (!region) return;
    
    // Natural stabilization/destabilization over time
    float stability = region->GetStability();
    float prosperity = region->GetProsperity();
    float danger = region->GetDanger();
    
    // Regions trend towards stability if not dangerous
    if (danger < 0.3f) {
        stability += 0.01f;
    } else if (danger > 0.7f) {
        stability -= 0.02f;
    }
    
    // Prosperity gradually follows stability
    if (stability > 0.5f) {
        prosperity += 0.01f;
    } else {
        prosperity -= 0.01f;
    }
    
    // Danger naturally decreases over time unless at war
    if (region->GetState() != RegionState::Warzone) {
        danger -= 0.01f;
    }
    
    // Clamp values
    stability = std::max(0.0f, std::min(1.0f, stability));
    prosperity = std::max(0.0f, std::min(1.0f, prosperity));
    danger = std::max(0.0f, std::min(1.0f, danger));
    
    // Apply changes
    region->SetStability(stability);
    region->SetProsperity(prosperity);
    region->SetDanger(danger);
    
    // Population changes over time
    int population = region->GetPopulation();
    float populationGrowthRate = 0.0f;
    
    // Growth rate based on prosperity and stability, reduced by danger
    populationGrowthRate = (prosperity * 0.5f + stability * 0.5f - danger) * 0.01f;
    
    // Apply population change
    int populationChange = static_cast<int>(population * populationGrowthRate);
    region->SetPopulation(population + populationChange);
    
    // Check for state changes
    RegionState currentState = region->GetState();
    RegionState calculatedState = CalculateRegionState(region);
    
    if (currentState != calculatedState) {
        SetRegionState(region->GetId(), calculatedState);
    }
    
    // Update controlling faction based on presence
    UpdateRegionController(region);
}

void WorldProgressionSystem::UpdateRegionController(WorldRegion* region) {
    if (!region) return;
    
    WorldFaction* dominantFaction = GetDominantFaction(region);
    
    if (dominantFaction) {
        std::string oldController = region->GetControllingFaction();
        std::string newController = dominantFaction->GetId();
        
        if (oldController != newController) {
            region->SetControllingFaction(newController);
            
            LOG(Info, "Region {0} control changed from {1} to {2}",
                String(region->GetName().c_str()),
                String(oldController.c_str()),
                String(newController.c_str()));
        }
    }
}

WorldFaction* WorldProgressionSystem::GetDominantFaction(WorldRegion* region) {
    if (!region) return nullptr;
    
    std::string strongestFactionId;
    float strongestInfluence = 0.0f;
    
    // Check each faction's weighted presence
    for (const auto& pair : m_factions) {
        float influence = GetFactionInfluenceWeighted(region->GetId(), pair.first);
        
        if (influence > strongestInfluence) {
            strongestInfluence = influence;
            strongestFactionId = pair.first;
        }
    }
    
    // Player influence overrides if strong enough
    float playerInfluence = region->GetPlayerInfluence();
    if (playerInfluence > 0.5f && playerInfluence > strongestInfluence) {
        return nullptr; // Player controls region
    }
    
    if (!strongestFactionId.empty()) {
        return GetFaction(strongestFactionId);
    }
    
    return nullptr;
}

float WorldProgressionSystem::GetFactionInfluenceWeighted(const std::string& regionId, const std::string& factionId) const {
    auto regionIt = m_regions.find(regionId);
    auto factionIt = m_factions.find(factionId);
    
    if (regionIt == m_regions.end() || factionIt == m_factions.end()) {
        return 0.0f;
    }
    
    const WorldRegion* region = regionIt->second.get();
    const WorldFaction* faction = factionIt->second.get();
    
    // Base influence is faction presence in region
    float influence = region->GetFactionPresence(factionId);
    
    // Weight by faction power
    influence *= faction->GetPower();
    
    // Weight by faction expansionism for non-controlled regions
    if (region->GetControllingFaction() != factionId) {
        influence *= faction->GetExpansionism();
    }
    
    return influence;
}

void WorldProgressionSystem::SimulateFactions() {
    for (auto& pair : m_factions) {
        WorldFaction* faction = pair.second.get();
        
        // Count controlled regions for power calculation
        std::vector<std::string> controlledRegions = GetRegionsControlledByFaction(pair.first);
        
        // Update faction power based on controlled regions and their prosperity
        float totalProsperity = 0.0f;
        for (const auto& regionId : controlledRegions) {
            WorldRegion* region = GetRegion(regionId);
            if (region) {
                totalProsperity += region->GetProsperity();
            }
        }
        
        // Power formula: base + controlled regions + prosperity bonus
        float newPower = 0.5f + (controlledRegions.size() * 0.1f) + (totalProsperity * 0.2f);
        faction->SetPower(newPower);
        
        // Small random changes to aggression and expansionism over time
        std::uniform_real_distribution<float> dist(-0.02f, 0.02f);
        
        float aggression = faction->GetAggression();
        float expansionism = faction->GetExpansionism();
        
        aggression += dist(m_randomGenerator);
        expansionism += dist(m_randomGenerator);
        
        // Clamp values
        aggression = std::max(0.1f, std::min(0.9f, aggression));
        expansionism = std::max(0.1f, std::min(0.9f, expansionism));
        
        faction->SetAggression(aggression);
        faction->SetExpansionism(expansionism);
    }
}

void WorldProgressionSystem::SimulateFactionConflicts() {
    // Check for potential conflicts between factions
    for (auto& pair1 : m_factions) {
        for (auto& pair2 : m_factions) {
            // Skip self comparisons
            if (pair1.first == pair2.first) continue;
            
            // Check relationship
            if (AreFactionsInConflict(pair1.first, pair2.first)) {
                // These factions are hostile or at war
                ResolveFactionConflict(pair1.first, pair2.first);
            }
        }
    }
}

bool WorldProgressionSystem::AreFactionsInConflict(const std::string& faction1Id, const std::string& faction2Id) const {
    WorldFaction* faction1 = m_factions.find(faction1Id)->second.get();
    
    FactionRelationship relationshipStatus = faction1->GetRelationshipStatus(faction2Id);
    return relationshipStatus == FactionRelationship::Hostile || 
           relationshipStatus == FactionRelationship::AtWar;
}

void WorldProgressionSystem::ResolveFactionConflict(const std::string& faction1Id, const std::string& faction2Id) {
    WorldFaction* faction1 = GetFaction(faction1Id);
    WorldFaction* faction2 = GetFaction(faction2Id);
    
    if (!faction1 || !faction2) return;
    
    // Find contested regions (where both factions have presence)
    std::vector<WorldRegion*> contestedRegions;
    
    for (auto& regionPair : m_regions) {
        WorldRegion* region = regionPair.second.get();
        
        float presence1 = region->GetFactionPresence(faction1Id);
        float presence2 = region->GetFactionPresence(faction2Id);
        
        if (presence1 > 0.0f && presence2 > 0.0f) {
            contestedRegions.push_back(region);
        }
    }
    
    // No contested regions, no conflict to resolve
    if (contestedRegions.empty()) return;
    
    // Resolve conflict in each contested region
    for (WorldRegion* region : contestedRegions) {
        // Calculate faction strengths
        float strength1 = faction1->GetPower() * faction1->GetAggression() * region->GetFactionPresence(faction1Id);
        float strength2 = faction2->GetPower() * faction2->GetAggression() * region->GetFactionPresence(faction2Id);
        
        // Determine winner
        std::string winnerId = (strength1 > strength2) ? faction1Id : faction2Id;
        std::string loserId = (strength1 > strength2) ? faction2Id : faction1Id;
        
        // Calculate margin of victory
        float strengthRatio = std::max(strength1, strength2) / std::max(0.1f, std::min(strength1, strength2));
        float conflictIntensity = std::min(1.0f, strengthRatio * 0.2f);
        
        // Winner gains influence, loser loses influence
        float winnerGain = 0.1f * conflictIntensity;
        float loserLoss = 0.2f * conflictIntensity;
        
        float winnerPresence = region->GetFactionPresence(winnerId) + winnerGain;
        float loserPresence = std::max(0.0f, region->GetFactionPresence(loserId) - loserLoss);
        
        region->AddFactionPresence(winnerId, winnerPresence);
        region->AddFactionPresence(loserId, loserPresence);
        
        // Increase danger in the region
        region->SetDanger(std::min(1.0f, region->GetDanger() + conflictIntensity * 0.3f));
        
        // Decrease stability
        region->SetStability(std::max(0.0f, region->GetStability() - conflictIntensity * 0.2f));
        
        // Conflict could change region state to dangerous or warzone
        if (conflictIntensity > 0.5f && region->GetState() != RegionState::Warzone) {
            SetRegionState(region->GetId(), RegionState::Warzone);
        } else if (conflictIntensity > 0.2f && region->GetState() == RegionState::Peaceful) {
            SetRegionState(region->GetId(), RegionState::Dangerous);
        }
        
        LOG(Info, "Conflict between {0} and {1} in {2}: {3} gaining influence",
            String(faction1->GetName().c_str()),
            String(faction2->GetName().c_str()),
            String(region->GetName().c_str()),
            String((winnerId == faction1Id ? faction1->GetName() : faction2->GetName()).c_str()));
    }
}

void WorldProgressionSystem::AttemptRegionalEvents() {
    // Try to trigger random events in regions
    for (auto& pair : m_regions) {
        WorldRegion* region = pair.second.get();
        
        // Random chance for an event based on region's stability
        // Less stable regions have more events
        float baseChance = m_eventChance;
        float stabilityFactor = 1.0f - region->GetStability();
        float eventChance = baseChance * (1.0f + stabilityFactor);
        
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (dist(m_randomGenerator) < eventChance) {
            // Region gets an event! Pick a random one
            std::vector<WorldEvent*> eligibleEvents;
            
            for (auto& eventPair : m_worldEvents) {
                WorldEvent* event = eventPair.second.get();
                
                // Skip events on cooldown
                if (!event->IsOnCooldown(m_gameTime)) {
                    eligibleEvents.push_back(event);
                }
            }
            
            if (!eligibleEvents.empty()) {
                // Pick weighted random event
                std::vector<float> weights;
                for (auto* event : eligibleEvents) {
                    weights.push_back(event->GetWeight());
                }
                
                std::discrete_distribution<> weightedDist(weights.begin(), weights.end());
                int eventIndex = weightedDist(m_randomGenerator);
                
                // Trigger the event
                TriggerWorldEvent(eligibleEvents[eventIndex]->GetId(), region->GetId());
            }
        }
    }
}

void WorldProgressionSystem::AdjustRegionByState(WorldRegion* region) {
    if (!region) return;
    
    switch (region->GetState()) {
        case RegionState::Peaceful:
            // Increase prosperity and stability
            region->SetProsperity(std::min(1.0f, region->GetProsperity() + 0.1f));
            region->SetStability(std::min(1.0f, region->GetStability() + 0.1f));
            region->SetDanger(std::max(0.0f, region->GetDanger() - 0.1f));
            break;
            
        case RegionState::Troubled:
            // Decrease prosperity and stability
            region->SetProsperity(std::max(0.0f, region->GetProsperity() - 0.1f));
            region->SetStability(std::max(0.0f, region->GetStability() - 0.1f));
            region->SetDanger(std::min(1.0f, region->GetDanger() + 0.1f));
            break;
            
        case RegionState::Dangerous:
            // Significant decreases to prosperity and stability
            region->SetProsperity(std::max(0.0f, region->GetProsperity() - 0.2f));
            region->SetStability(std::max(0.0f, region->GetStability() - 0.2f));
            region->SetDanger(std::min(1.0f, region->GetDanger() + 0.3f));
            break;
            
        case RegionState::Warzone:
            // Severe decreases to prosperity and stability
            region->SetProsperity(std::max(0.0f, region->GetProsperity() - 0.3f));
            region->SetStability(std::max(0.0f, region->GetStability() - 0.4f));
            region->SetDanger(std::min(1.0f, region->GetDanger() + 0.5f));
            
            // Population loss in warzone
            region->SetPopulation(region->GetPopulation() - static_cast<int>(region->GetPopulation() * 0.05f));
            break;
            
        case RegionState::Abandoned:
            // Complete collapse
            region->SetProsperity(0.0f);
            region->SetStability(0.0f);
            region->SetDanger(1.0f);
            
            // Major population loss in abandoned region
            region->SetPopulation(static_cast<int>(region->GetPopulation() * 0.1f));
            break;
            
        case RegionState::Rebuilding:
            // Slow recovery
            region->SetProsperity(std::min(0.5f, region->GetProsperity() + 0.05f));
            region->SetStability(std::min(0.5f, region->GetStability() + 0.05f));
            region->SetDanger(std::max(0.3f, region->GetDanger() - 0.05f));
            break;
    }
}

std::string WorldProgressionSystem::RegionStateToString(RegionState state) const {
    switch (state) {
        case RegionState::Peaceful: return "Peaceful";
        case RegionState::Troubled: return "Troubled";
        case RegionState::Dangerous: return "Dangerous";
        case RegionState::Warzone: return "Warzone";
        case RegionState::Abandoned: return "Abandoned";
        case RegionState::Rebuilding: return "Rebuilding";
        default: return "Unknown";
    }
}

RegionState WorldProgressionSystem::StringToRegionState(const std::string& str) const {
    if (str == "Peaceful") return RegionState::Peaceful;
    if (str == "Troubled") return RegionState::Troubled;
    if (str == "Dangerous") return RegionState::Dangerous;
    if (str == "Warzone") return RegionState::Warzone;
    if (str == "Abandoned") return RegionState::Abandoned;
    if (str == "Rebuilding") return RegionState::Rebuilding;
    
    return RegionState::Peaceful; // Default
}

std::string WorldProgressionSystem::FactionRelationshipToString(FactionRelationship relationship) const {
    switch (relationship) {
        case FactionRelationship::Allied: return "Allied";
        case FactionRelationship::Friendly: return "Friendly";
        case FactionRelationship::Neutral: return "Neutral";
        case FactionRelationship::Unfriendly: return "Unfriendly";
        case FactionRelationship::Hostile: return "Hostile";
        case FactionRelationship::AtWar: return "At War";
        default: return "Unknown";
    }
}

FactionRelationship WorldProgressionSystem::StringToFactionRelationship(const std::string& str) const {
    if (str == "Allied") return FactionRelationship::Allied;
    if (str == "Friendly") return FactionRelationship::Friendly;
    if (str == "Neutral") return FactionRelationship::Neutral;
    if (str == "Unfriendly") return FactionRelationship::Unfriendly;
    if (str == "Hostile") return FactionRelationship::Hostile;
    if (str == "At War") return FactionRelationship::AtWar;
    
    return FactionRelationship::Neutral; // Default
}

FactionRelationship WorldProgressionSystem::RelationshipValueToStatus(float value) const {
    if (value >= 0.75f) return FactionRelationship::Allied;
    if (value >= 0.25f) return FactionRelationship::Friendly;
    if (value >= -0.25f) return FactionRelationship::Neutral;
    if (value >= -0.75f) return FactionRelationship::Unfriendly;
    if (value > -1.0f) return FactionRelationship::Hostile;
    return FactionRelationship::AtWar;
}

void WorldProgressionSystem::Serialize(BinaryWriter& writer) const {
    // Write system state
    writer.Write(m_worldSimulationInterval);
    writer.Write(m_timeSinceLastSimulation);
    writer.Write(m_gameTime);
    writer.Write(m_eventChance);
    
    // Write regions
    writer.Write(static_cast<uint32_t>(m_regions.size()));
    for (const auto& pair : m_regions) {
        writer.Write(pair.first);
        pair.second->Serialize(writer);
    }
    
    // Write factions
    writer.Write(static_cast<uint32_t>(m_factions.size()));
    for (const auto& pair : m_factions) {
        writer.Write(pair.first);
        pair.second->Serialize(writer);
    }
    
    // Write world events
    writer.Write(static_cast<uint32_t>(m_worldEvents.size()));
    for (const auto& pair : m_worldEvents) {
        writer.Write(pair.first);
        pair.second->Serialize(writer);
    }
    
    LOG(Info, "WorldProgressionSystem serialized");
}

void WorldProgressionSystem::Deserialize(BinaryReader& reader) {
    // Read system state
    reader.Read(m_worldSimulationInterval);
    reader.Read(m_timeSinceLastSimulation);
    reader.Read(m_gameTime);
    reader.Read(m_eventChance);
    
    // Read regions
    uint32_t regionCount;
    reader.Read(regionCount);
    
    m_regions.clear();
    for (uint32_t i = 0; i < regionCount; ++i) {
        std::string regionId;
        reader.Read(regionId);
        
        auto region = std::make_unique<WorldRegion>("", "");
        region->Deserialize(reader);
        m_regions[regionId] = std::move(region);
    }
    
    // Read factions
    uint32_t factionCount;
    reader.Read(factionCount);
    
    m_factions.clear();
    for (uint32_t i = 0; i < factionCount; ++i) {
        std::string factionId;
        reader.Read(factionId);
        
        auto faction = std::make_unique<WorldFaction>("", "");
        faction->Deserialize(reader);
        m_factions[factionId] = std::move(faction);
    }
    
    // Read world events
    uint32_t eventCount;
    reader.Read(eventCount);
    
    m_worldEvents.clear();
    for (uint32_t i = 0; i < eventCount; ++i) {
        std::string eventId;
        reader.Read(eventId);
        
        auto event = std::make_unique<WorldEvent>("", "", "");
        event->Deserialize(reader);
        m_worldEvents[eventId] = std::move(event);
    }
    
    LOG(Info, "WorldProgressionSystem deserialized");
}

void WorldProgressionSystem::SerializeToText(TextWriter& writer) const {
    // Write system state
    writer.Write("worldSimulationInterval", m_worldSimulationInterval);
    writer.Write("worldTimeSinceLastSimulation", m_timeSinceLastSimulation);
    writer.Write("worldGameTime", m_gameTime);
    writer.Write("worldEventChance", m_eventChance);
    
    // Write region count
    writer.Write("regionCount", static_cast<int>(m_regions.size()));
    
    // Write each region
    int regionIndex = 0;
    for (const auto& pair : m_regions) {
        std::string prefix = "region" + std::to_string(regionIndex) + "_";
        writer.Write(prefix + "id", pair.first);
        
        // Let the region serialize itself
        pair.second->SerializeToText(writer);
        
        regionIndex++;
    }
    
    // Write faction count
    writer.Write("factionCount", static_cast<int>(m_factions.size()));
    
    // Write each faction
    int factionIndex = 0;
    for (const auto& pair : m_factions) {
        std::string prefix = "faction" + std::to_string(factionIndex) + "_";
        writer.Write(prefix + "id", pair.first);
        
        // Let the faction serialize itself
        pair.second->SerializeToText(writer);
        
        factionIndex++;
    }
    
    // Write event count
    writer.Write("worldEventCount", static_cast<int>(m_worldEvents.size()));
    
    // Write each event
    int eventIndex = 0;
    for (const auto& pair : m_worldEvents) {
        std::string prefix = "worldEvent" + std::to_string(eventIndex) + "_";
        writer.Write(prefix + "id", pair.first);
        
        // Let the event serialize itself
        pair.second->SerializeToText(writer);
        
        eventIndex++;
    }
    
    LOG(Info, "WorldProgressionSystem serialized to text");
}

void WorldProgressionSystem::DeserializeFromText(TextReader& reader) {
    // Read system state
    reader.Read("worldSimulationInterval", m_worldSimulationInterval);
    reader.Read("worldTimeSinceLastSimulation", m_timeSinceLastSimulation);
    reader.Read("worldGameTime", m_gameTime);
    reader.Read("worldEventChance", m_eventChance);
    
    // Read regions
    int regionCount = 0;
    reader.Read("regionCount", regionCount);
    
    m_regions.clear();
    for (int i = 0; i < regionCount; i++) {
        std::string prefix = "region" + std::to_string(i) + "_";
        
        std::string regionId;
        reader.Read(prefix + "id", regionId);
        
        auto region = std::make_unique<WorldRegion>("", "");
        region->DeserializeFromText(reader);
        m_regions[regionId] = std::move(region);
    }
    
    // Read factions
    int factionCount = 0;
    reader.Read("factionCount", factionCount);
    
    m_factions.clear();
    for (int i = 0; i < factionCount; i++) {
        std::string prefix = "faction" + std::to_string(i) + "_";
        
        std::string factionId;
        reader.Read(prefix + "id", factionId);
        
        auto faction = std::make_unique<WorldFaction>("", "");
        faction->DeserializeFromText(reader);
        m_factions[factionId] = std::move(faction);
    }
    
    // Read world events
    int eventCount = 0;
    reader.Read("worldEventCount", eventCount);
    
    m_worldEvents.clear();
    for (int i = 0; i < eventCount; i++) {
        std::string prefix = "worldEvent" + std::to_string(i) + "_";
        
        std::string eventId;
        reader.Read(prefix + "id", eventId);
        
        auto event = std::make_unique<WorldEvent>("", "", "");
        event->DeserializeFromText(reader);
        m_worldEvents[eventId] = std::move(event);
    }
    
    LOG(Info, "WorldProgressionSystem deserialized from text");
}
// ^ WorldProgressionSystem.cpp