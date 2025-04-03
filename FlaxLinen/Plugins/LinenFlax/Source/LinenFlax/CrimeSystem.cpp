// v CrimeSystem.cpp
#include "CrimeSystem.h"
#include "LinenFlax.h"
#include "RelationshipSystem.h"
#include "FactionSystem.h"
#include "TimeSystem.h"
#include "Engine/Core/Log.h"

CrimeSystem::CrimeSystem() {
    // Define system dependencies
    m_dependencies.insert("FactionSystem");
    m_dependencies.insert("RelationshipSystem");
    m_dependencies.insert("TimeSystem");
}

CrimeSystem::~CrimeSystem() {
    Destroy();
}

void CrimeSystem::Initialize() {
    // Register default crime definitions
    RegisterCrimeType(CrimeType::Trespassing, "Trespassing", 10);
    RegisterCrimeType(CrimeType::Theft, "Theft", 25);
    RegisterCrimeType(CrimeType::Assault, "Assault", 40);
    RegisterCrimeType(CrimeType::Murder, "Murder", 100);
    RegisterCrimeType(CrimeType::Vandalism, "Vandalism", 20);
    RegisterCrimeType(CrimeType::MagicUsage, "Forbidden Magic", 30);
    
    LOG(Info, "Crime System Initialized");
    
    // Subscribe to time events to process expired crimes
    if (m_plugin) {
        m_plugin->GetEventSystem().Subscribe<HourChangedEvent>(
            [this](const HourChangedEvent& event) {
                this->ProcessExpiredCrimes();
            });
    }
}

void CrimeSystem::Shutdown() {
    m_regions.clear();
    m_crimeDefinitions.clear();
    m_activeCrimes.clear();
    LOG(Info, "Crime System Shutdown");
}

void CrimeSystem::Update(float deltaTime) {
    // Update game time tracking
    m_currentGameTime += deltaTime;
    
    // Process expired crimes every few seconds in real-time
    static float timeSinceLastCheck = 0.0f;
    timeSinceLastCheck += deltaTime;
    
    if (timeSinceLastCheck >= 5.0f) {  // Check every 5 seconds
        timeSinceLastCheck = 0.0f;
        ProcessExpiredCrimes();
    }
}

bool CrimeSystem::RegisterRegion(const std::string& regionId, const std::string& name) {
    if (m_regions.find(regionId) != m_regions.end()) {
        LOG(Warning, "Region already registered: {0}", String(regionId.c_str()));
        return false;
    }
    
    Region region;
    region.id = regionId;
    region.name = name;
    
    m_regions[regionId] = region;
    LOG(Info, "Registered region: {0} ({1})", String(name.c_str()), String(regionId.c_str()));
    return true;
}

bool CrimeSystem::DoesRegionExist(const std::string& regionId) const {
    return m_regions.find(regionId) != m_regions.end();
}

std::string CrimeSystem::GetRegionName(const std::string& regionId) const {
    auto it = m_regions.find(regionId);
    if (it == m_regions.end()) {
        return "";
    }
    return it->second.name;
}

void CrimeSystem::RegisterCrimeType(CrimeType type, const std::string& name, int baseSeverity) {
    CrimeDefinition def;
    def.type = type;
    def.name = name;
    def.baseSeverity = baseSeverity;
    
    m_crimeDefinitions[type] = def;
    LOG(Info, "Registered crime type: {0} (severity {1})", String(name.c_str()), baseSeverity);
}

int CrimeSystem::GetCrimeSeverity(CrimeType type) const {
    auto it = m_crimeDefinitions.find(type);
    if (it == m_crimeDefinitions.end()) {
        return 0;
    }
    return it->second.baseSeverity;
}

std::string CrimeSystem::GetCrimeTypeName(CrimeType type) const {
    auto it = m_crimeDefinitions.find(type);
    if (it == m_crimeDefinitions.end()) {
        return "Unknown";
    }
    return it->second.name;
}

void CrimeSystem::ReportCrime(const std::string& perpetratorId, const std::string& victimId, 
                              const std::string& regionId, CrimeType type,
                              const std::vector<std::string>& witnesses) {
    if (!DoesRegionExist(regionId)) {
        LOG(Warning, "Cannot report crime in non-existent region: {0}", String(regionId.c_str()));
        return;
    }
    
    auto it = m_crimeDefinitions.find(type);
    if (it == m_crimeDefinitions.end()) {
        LOG(Warning, "Cannot report unknown crime type");
        return;
    }
    
    Crime crime;
    crime.perpetratorId = perpetratorId;
    crime.victimId = victimId;
    crime.regionId = regionId;
    crime.type = type;
    crime.severity = it->second.baseSeverity;
    crime.reported = !witnesses.empty();
    crime.witnesses = witnesses;
    
    // Get current game time from TimeSystem if available
    if (m_plugin) {
        auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
        if (timeSystem) {
            // Convert to total hours for simpler expiration calculations
            int hours = timeSystem->GetHour();
            int days = timeSystem->GetDay();
            int months = timeSystem->GetMonth();
            int years = timeSystem->GetYear();
            
            // Simplify to total hours
            int totalHours = hours + 
                             (days * 24) + 
                             (months * timeSystem->GetDaysPerMonth() * 24) +
                             (years * timeSystem->GetMonthsPerYear() * timeSystem->GetDaysPerMonth() * 24);
            
            crime.gameTimeCommitted = static_cast<float>(totalHours);
        }
    }
    
    m_activeCrimes.push_back(crime);
    
    // Calculate and apply bounty
    CalculateBounty(crime);
    
    // Create and publish event
    CrimeCommittedEvent event;
    event.perpetratorId = perpetratorId;
    event.victimId = victimId;
    event.regionId = regionId;
    event.crimeType = GetCrimeTypeName(type);
    event.severity = crime.severity;
    event.witnessed = !witnesses.empty();
    event.witnessIds = witnesses;
    
    m_plugin->GetEventSystem().Publish(event);
    
    LOG(Info, "Crime reported: {0} committed by {1} against {2} in {3} (severity: {4})", 
        String(GetCrimeTypeName(type).c_str()), 
        String(perpetratorId.c_str()), 
        String(victimId.c_str()),
        String(GetRegionName(regionId).c_str()),
        crime.severity);
    
    // Process witnesses
    for (const auto& witnessId : witnesses) {
        RegisterWitness(witnessId, perpetratorId, regionId, type);
    }
}

void CrimeSystem::RegisterWitness(const std::string& witnessId, const std::string& perpetratorId, 
                                const std::string& regionId, CrimeType type) {
    
    WitnessReaction reaction = DetermineWitnessReaction(witnessId, perpetratorId, type);
    
    switch (reaction) {
        case WitnessReaction::Report:
            // Modify bounty
            {
                auto* relationshipSystem = m_plugin->GetSystem<RelationshipSystem>();
                int severityMultiplier = 1;
                
                // Increase severity if witness is important (e.g., a guard)
                if (IsGuardFaction(regionId, witnessId)) {
                    severityMultiplier = 2;  // Guards' reports are taken more seriously
                }
                
                // Apply bounty for the reported crime
                int baseSeverity = GetCrimeSeverity(type);
                ModifyBounty(perpetratorId, regionId, baseSeverity * severityMultiplier);
                
                LOG(Info, "Witness {0} reported crime by {1}", 
                    String(witnessId.c_str()), String(perpetratorId.c_str()));
            }
            break;
            
        case WitnessReaction::Flee:
            // Nothing to do in terms of bounty, but log it
            LOG(Info, "Witness {0} fled from crime by {1}", 
                String(witnessId.c_str()), String(perpetratorId.c_str()));
            break;
            
        case WitnessReaction::Attack:
            // Hostile reaction, but not a formal report
            LOG(Info, "Witness {0} attacking criminal {1}", 
                String(witnessId.c_str()), String(perpetratorId.c_str()));
            
            // Update relationship if relationship system is available
            if (m_plugin) {
                auto* relationshipSystem = m_plugin->GetSystem<RelationshipSystem>();
                if (relationshipSystem) {
                    relationshipSystem->SetRelationship(witnessId, perpetratorId, -100);
                }
            }
            break;
            
        case WitnessReaction::Ignore:
            // Nothing to do
            LOG(Info, "Witness {0} ignored crime by {1}", 
                String(witnessId.c_str()), String(perpetratorId.c_str()));
            break;
    }
}

WitnessReaction CrimeSystem::DetermineWitnessReaction(const std::string& witnessId, 
                                                   const std::string& perpetratorId, 
                                                   CrimeType type) const {
    // Default reaction is to report
    WitnessReaction reaction = WitnessReaction::Report;
    
    // If relationship system is available, use relationship to modify reaction
    if (m_plugin) {
        auto* relationshipSystem = m_plugin->GetSystem<RelationshipSystem>();
        if (relationshipSystem) {
            int relationValue = relationshipSystem->GetRelationship(witnessId, perpetratorId);
            
            // Get crime severity
            int severity = GetCrimeSeverity(type);
            
            // Factor in both the crime severity and relationship
            if (severity >= 75) {
                // Very serious crimes are almost always reported
                if (relationValue >= 75) {
                    // Unless they're best friends/allies
                    reaction = WitnessReaction::Ignore;
                } else if (relationValue <= -75) {
                    // Enemies will attack
                    reaction = WitnessReaction::Attack;
                } else {
                    reaction = WitnessReaction::Report;
                }
            } else if (severity >= 40) {
                // Moderate crimes
                if (relationValue >= 50) {
                    reaction = WitnessReaction::Ignore;
                } else if (relationValue <= -50) {
                    reaction = WitnessReaction::Attack;
                } else {
                    reaction = WitnessReaction::Report;
                }
            } else {
                // Minor crimes
                if (relationValue >= 25) {
                    reaction = WitnessReaction::Ignore;
                } else if (relationValue <= -75) {
                    reaction = WitnessReaction::Attack;
                } else if (relationValue <= -25) {
                    reaction = WitnessReaction::Report;
                } else {
                    // Neutral witnesses may ignore minor crimes
                    reaction = WitnessReaction::Ignore;
                }
            }
        }
    }
    
    return reaction;
}

int CrimeSystem::GetBounty(const std::string& characterId, const std::string& regionId) const {
    if (!DoesRegionExist(regionId)) {
        return 0;
    }
    
    const auto& region = m_regions.at(regionId);
    auto it = region.bounties.find(characterId);
    
    if (it == region.bounties.end()) {
        return 0;
    }
    
    return it->second;
}

void CrimeSystem::ModifyBounty(const std::string& characterId, const std::string& regionId, int amount) {
    if (!DoesRegionExist(regionId) || amount == 0) {
        return;
    }
    
    Region& region = m_regions[regionId];
    int previousBounty = GetBounty(characterId, regionId);
    int newBounty = previousBounty + amount;
    
    // Ensure bounty doesn't go negative
    if (newBounty < 0) {
        newBounty = 0;
    }
    
    // Update bounty
    region.bounties[characterId] = newBounty;
    
    // Create and publish event
    BountyChangedEvent event;
    event.characterId = characterId;
    event.regionId = regionId;
    event.previousBounty = previousBounty;
    event.newBounty = newBounty;
    
    m_plugin->GetEventSystem().Publish(event);
    
    LOG(Info, "Bounty changed for {0} in {1}: {2} -> {3}", 
        String(characterId.c_str()), 
        String(GetRegionName(regionId).c_str()),
        previousBounty, newBounty);
    
    // If faction system is available, update guard faction relationship
    if (m_plugin) {
        auto* factionSystem = m_plugin->GetSystem<FactionSystem>();
        if (factionSystem) {
            auto& region = m_regions[regionId];
            if (!region.guardFaction.empty()) {
                // Calculate reputation impact based on bounty
                int repImpact = -(newBounty / 10);  // Simplistic conversion
                
                // Ensure reasonable range
                if (repImpact < -100) repImpact = -100;
                if (repImpact > 0) repImpact = 0;
                
                // Set reputation with guard faction based on bounty
                factionSystem->SetReputation(characterId, region.guardFaction, repImpact);
            }
        }
    }
}

void CrimeSystem::ClearBounty(const std::string& characterId, const std::string& regionId) {
    if (!DoesRegionExist(regionId)) {
        return;
    }
    
    Region& region = m_regions[regionId];
    int previousBounty = GetBounty(characterId, regionId);
    
    if (previousBounty > 0) {
        // Remove bounty
        region.bounties.erase(characterId);
        
        // Create and publish event
        BountyChangedEvent event;
        event.characterId = characterId;
        event.regionId = regionId;
        event.previousBounty = previousBounty;
        event.newBounty = 0;
        
        m_plugin->GetEventSystem().Publish(event);
        
        LOG(Info, "Bounty cleared for {0} in {1} (was {2})", 
            String(characterId.c_str()), 
            String(GetRegionName(regionId).c_str()),
            previousBounty);
        
        // If faction system is available, update guard faction relationship
        if (m_plugin) {
            auto* factionSystem = m_plugin->GetSystem<FactionSystem>();
            if (factionSystem && !region.guardFaction.empty()) {
                // Set neutral reputation with guard faction after bounty payment
                factionSystem->SetReputation(characterId, region.guardFaction, 0);
            }
        }
    }
}

bool CrimeSystem::HasBounty(const std::string& characterId, const std::string& regionId) const {
    return GetBounty(characterId, regionId) > 0;
}

void CrimeSystem::RegisterGuardFaction(const std::string& regionId, const std::string& factionId) {
    if (!DoesRegionExist(regionId)) {
        LOG(Warning, "Cannot set guard faction for non-existent region: {0}", String(regionId.c_str()));
        return;
    }
    
    Region& region = m_regions[regionId];
    region.guardFaction = factionId;
    
    LOG(Info, "Set guard faction for region {0} to {1}", 
        String(GetRegionName(regionId).c_str()), String(factionId.c_str()));
}

bool CrimeSystem::IsGuardFaction(const std::string& regionId, const std::string& factionId) const {
    if (!DoesRegionExist(regionId)) {
        return false;
    }
    
    const auto& region = m_regions.at(regionId);
    return region.guardFaction == factionId;
}

void CrimeSystem::SetCrimeExpirationTime(int gameHours) {
    if (gameHours <= 0) {
        LOG(Warning, "Crime expiration time must be positive, using default");
        m_crimeExpirationHours = 72;  // Default 3 days
    } else {
        m_crimeExpirationHours = gameHours;
        LOG(Info, "Crime expiration time set to {0} hours", gameHours);
    }
}

// v CrimeSystem.cpp (continued...)
void CrimeSystem::ProcessExpiredCrimes() {
    // No TimeSystem, skip expiration
    if (!m_plugin) return;
    
    auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
    if (!timeSystem) return;
    
    // Calculate current game time in hours
    int hours = timeSystem->GetHour();
    int days = timeSystem->GetDay();
    int months = timeSystem->GetMonth();
    int years = timeSystem->GetYear();
    
    // Convert to total hours for simpler expiration calculations
    int totalHours = hours + 
                    (days * 24) + 
                    (months * timeSystem->GetDaysPerMonth() * 24) +
                    (years * timeSystem->GetMonthsPerYear() * timeSystem->GetDaysPerMonth() * 24);
    
    float currentGameTime = static_cast<float>(totalHours);
    
    // Check each crime for expiration
    for (int i = static_cast<int>(m_activeCrimes.size()) - 1; i >= 0; --i) {
        const auto& crime = m_activeCrimes[i];
        
        float timeSinceCommitted = currentGameTime - crime.gameTimeCommitted;
        
        if (timeSinceCommitted >= static_cast<float>(m_crimeExpirationHours)) {
            // If it has a bounty, reduce it before removing
            if (HasBounty(crime.perpetratorId, crime.regionId)) {
                int currentBounty = GetBounty(crime.perpetratorId, crime.regionId);
                // Determine how much bounty to reduce based on crime severity
                int reductionAmount = crime.severity;
                
                if (reductionAmount > currentBounty) {
                    reductionAmount = currentBounty;
                }
                
                if (reductionAmount > 0) {
                    ModifyBounty(crime.perpetratorId, crime.regionId, -reductionAmount);
                }
            }
            
            // Create and publish expired crime event
            CrimeExpiredEvent event;
            event.perpetratorId = crime.perpetratorId;
            event.regionId = crime.regionId;
            event.crimeType = GetCrimeTypeName(crime.type);
            
            m_plugin->GetEventSystem().Publish(event);
            
            LOG(Info, "Crime expired: {0} committed by {1} in {2}", 
                String(GetCrimeTypeName(crime.type).c_str()), 
                String(crime.perpetratorId.c_str()),
                String(GetRegionName(crime.regionId).c_str()));
            
            // Remove the crime from the tracking list
            m_activeCrimes.erase(m_activeCrimes.begin() + i);
        }
    }
}

void CrimeSystem::CalculateBounty(const Crime& crime) {
    // Base bounty on crime severity
    int bountyAmount = crime.severity;
    
    // If the crime is witnessed, increase the bounty
    if (!crime.witnesses.empty()) {
        bountyAmount *= static_cast<int>(crime.witnesses.size() + 1);
    }
    
    // If the victim is important (e.g., a guard or noble), increase the bounty
    if (IsGuardFaction(crime.regionId, crime.victimId)) {
        bountyAmount *= 2;  // Double bounty for crimes against guards
    }
    
    // Add the bounty to the perpetrator
    if (bountyAmount > 0) {
        ModifyBounty(crime.perpetratorId, crime.regionId, bountyAmount);
    }
}

void CrimeSystem::PurgeCrime(size_t index) {
    if (index >= m_activeCrimes.size()) {
        return;
    }
    
    m_activeCrimes.erase(m_activeCrimes.begin() + index);
}

void CrimeSystem::Serialize(BinaryWriter& writer) const {
    // Write regions
    writer.Write(static_cast<uint32_t>(m_regions.size()));
    
    for (const auto& pair : m_regions) {
        const Region& region = pair.second;
        
        writer.Write(region.id);
        writer.Write(region.name);
        writer.Write(region.guardFaction);
        
        // Write bounties
        writer.Write(static_cast<uint32_t>(region.bounties.size()));
        
        for (const auto& bounty : region.bounties) {
            writer.Write(bounty.first);   // Character ID
            writer.Write(bounty.second);  // Bounty amount
        }
    }
    
    // Write crime definitions
    writer.Write(static_cast<uint32_t>(m_crimeDefinitions.size()));
    
    for (const auto& pair : m_crimeDefinitions) {
        writer.Write(static_cast<int32_t>(pair.first));  // Crime type enum
        writer.Write(pair.second.name);
        writer.Write(pair.second.baseSeverity);
    }
    
    // Write active crimes
    writer.Write(static_cast<uint32_t>(m_activeCrimes.size()));
    
    for (const auto& crime : m_activeCrimes) {
        writer.Write(crime.perpetratorId);
        writer.Write(crime.victimId);
        writer.Write(crime.regionId);
        writer.Write(static_cast<int32_t>(crime.type));
        writer.Write(crime.severity);
        writer.Write(crime.reported);
        writer.Write(crime.gameTimeCommitted);
        
        // Write witnesses
        writer.Write(static_cast<uint32_t>(crime.witnesses.size()));
        
        for (const auto& witness : crime.witnesses) {
            writer.Write(witness);
        }
    }
    
    // Write settings
    writer.Write(m_crimeExpirationHours);
    writer.Write(m_currentGameTime);
    
    LOG(Info, "CrimeSystem serialized");
}

void CrimeSystem::Deserialize(BinaryReader& reader) {
    // Clear existing data
    m_regions.clear();
    m_crimeDefinitions.clear();
    m_activeCrimes.clear();
    
    // Read regions
    uint32_t regionCount = 0;
    reader.Read(regionCount);
    
    for (uint32_t i = 0; i < regionCount; ++i) {
        Region region;
        
        reader.Read(region.id);
        reader.Read(region.name);
        reader.Read(region.guardFaction);
        
        // Read bounties
        uint32_t bountyCount = 0;
        reader.Read(bountyCount);
        
        for (uint32_t j = 0; j < bountyCount; ++j) {
            std::string characterId;
            int amount = 0;
            
            reader.Read(characterId);
            reader.Read(amount);
            
            region.bounties[characterId] = amount;
        }
        
        m_regions[region.id] = region;
    }
    
    // Read crime definitions
    uint32_t crimeDefCount = 0;
    reader.Read(crimeDefCount);
    
    for (uint32_t i = 0; i < crimeDefCount; ++i) {
        int32_t typeValue = 0;
        CrimeDefinition def;
        
        reader.Read(typeValue);
        def.type = static_cast<CrimeType>(typeValue);
        reader.Read(def.name);
        reader.Read(def.baseSeverity);
        
        m_crimeDefinitions[def.type] = def;
    }
    
    // Read active crimes
    uint32_t crimeCount = 0;
    reader.Read(crimeCount);
    
    for (uint32_t i = 0; i < crimeCount; ++i) {
        Crime crime;
        int32_t typeValue = 0;
        
        reader.Read(crime.perpetratorId);
        reader.Read(crime.victimId);
        reader.Read(crime.regionId);
        reader.Read(typeValue);
        crime.type = static_cast<CrimeType>(typeValue);
        reader.Read(crime.severity);
        reader.Read(crime.reported);
        reader.Read(crime.gameTimeCommitted);
        
        // Read witnesses
        uint32_t witnessCount = 0;
        reader.Read(witnessCount);
        
        for (uint32_t j = 0; j < witnessCount; ++j) {
            std::string witness;
            reader.Read(witness);
            crime.witnesses.push_back(witness);
        }
        
        m_activeCrimes.push_back(crime);
    }
    
    // Read settings
    reader.Read(m_crimeExpirationHours);
    reader.Read(m_currentGameTime);
    
    LOG(Info, "CrimeSystem deserialized");
}

void CrimeSystem::SerializeToText(TextWriter& writer) const {
    // Write region count
    writer.Write("regionCount", static_cast<int>(m_regions.size()));
    
    // Write each region
    int regionIndex = 0;
    for (const auto& pair : m_regions) {
        const Region& region = pair.second;
        std::string prefix = "region" + std::to_string(regionIndex) + "_";
        
        writer.Write(prefix + "id", region.id);
        writer.Write(prefix + "name", region.name);
        writer.Write(prefix + "guardFaction", region.guardFaction);
        
        // Write bounties
        writer.Write(prefix + "bountyCount", static_cast<int>(region.bounties.size()));
        
        int bountyIndex = 0;
        for (const auto& bounty : region.bounties) {
            std::string bountyPrefix = prefix + "bounty" + std::to_string(bountyIndex) + "_";
            writer.Write(bountyPrefix + "characterId", bounty.first);
            writer.Write(bountyPrefix + "amount", bounty.second);
            bountyIndex++;
        }
        
        regionIndex++;
    }
    
    // Write crime definitions
    writer.Write("crimeDefCount", static_cast<int>(m_crimeDefinitions.size()));
    
    int crimeDefIndex = 0;
    for (const auto& pair : m_crimeDefinitions) {
        std::string prefix = "crimeDef" + std::to_string(crimeDefIndex) + "_";
        
        writer.Write(prefix + "type", static_cast<int>(pair.first));
        writer.Write(prefix + "name", pair.second.name);
        writer.Write(prefix + "severity", pair.second.baseSeverity);
        
        crimeDefIndex++;
    }
    
    // Write active crimes
    writer.Write("crimeCount", static_cast<int>(m_activeCrimes.size()));
    
    int crimeIndex = 0;
    for (const auto& crime : m_activeCrimes) {
        std::string prefix = "crime" + std::to_string(crimeIndex) + "_";
        
        writer.Write(prefix + "perpetratorId", crime.perpetratorId);
        writer.Write(prefix + "victimId", crime.victimId);
        writer.Write(prefix + "regionId", crime.regionId);
        writer.Write(prefix + "type", static_cast<int>(crime.type));
        writer.Write(prefix + "severity", crime.severity);
        writer.Write(prefix + "reported", crime.reported ? 1 : 0);
        writer.Write(prefix + "gameTimeCommitted", crime.gameTimeCommitted);
        
        // Write witnesses
        writer.Write(prefix + "witnessCount", static_cast<int>(crime.witnesses.size()));
        
        for (size_t i = 0; i < crime.witnesses.size(); ++i) {
            writer.Write(prefix + "witness" + std::to_string(i), crime.witnesses[i]);
        }
        
        crimeIndex++;
    }
    
    // Write settings
    writer.Write("crimeExpirationHours", m_crimeExpirationHours);
    writer.Write("currentGameTime", m_currentGameTime);
    
    LOG(Info, "CrimeSystem serialized to text");
}

void CrimeSystem::DeserializeFromText(TextReader& reader) {
    // Clear existing data
    m_regions.clear();
    m_crimeDefinitions.clear();
    m_activeCrimes.clear();
    
    // Read region count
    int regionCount = 0;
    reader.Read("regionCount", regionCount);
    
    // Read each region
    for (int i = 0; i < regionCount; ++i) {
        Region region;
        std::string prefix = "region" + std::to_string(i) + "_";
        
        reader.Read(prefix + "id", region.id);
        reader.Read(prefix + "name", region.name);
        reader.Read(prefix + "guardFaction", region.guardFaction);
        
        // Read bounties
        int bountyCount = 0;
        reader.Read(prefix + "bountyCount", bountyCount);
        
        for (int j = 0; j < bountyCount; ++j) {
            std::string bountyPrefix = prefix + "bounty" + std::to_string(j) + "_";
            std::string characterId;
            int amount = 0;
            
            reader.Read(bountyPrefix + "characterId", characterId);
            reader.Read(bountyPrefix + "amount", amount);
            
            region.bounties[characterId] = amount;
        }
        
        m_regions[region.id] = region;
    }
    
    // Read crime definitions
    int crimeDefCount = 0;
    reader.Read("crimeDefCount", crimeDefCount);
    
    for (int i = 0; i < crimeDefCount; ++i) {
        std::string prefix = "crimeDef" + std::to_string(i) + "_";
        int typeValue = 0;
        CrimeDefinition def;
        
        reader.Read(prefix + "type", typeValue);
        def.type = static_cast<CrimeType>(typeValue);
        reader.Read(prefix + "name", def.name);
        reader.Read(prefix + "severity", def.baseSeverity);
        
        m_crimeDefinitions[def.type] = def;
    }
    
    // Read active crimes
    int crimeCount = 0;
    reader.Read("crimeCount", crimeCount);
    
    for (int i = 0; i < crimeCount; ++i) {
        std::string prefix = "crime" + std::to_string(i) + "_";
        Crime crime;
        int typeValue = 0;
        int reported = 0;
        
        reader.Read(prefix + "perpetratorId", crime.perpetratorId);
        reader.Read(prefix + "victimId", crime.victimId);
        reader.Read(prefix + "regionId", crime.regionId);
        reader.Read(prefix + "type", typeValue);
        crime.type = static_cast<CrimeType>(typeValue);
        reader.Read(prefix + "severity", crime.severity);
        reader.Read(prefix + "reported", reported);
        crime.reported = (reported != 0);
        reader.Read(prefix + "gameTimeCommitted", crime.gameTimeCommitted);
        
        // Read witnesses
        int witnessCount = 0;
        reader.Read(prefix + "witnessCount", witnessCount);
        
        for (int j = 0; j < witnessCount; ++j) {
            std::string witness;
            reader.Read(prefix + "witness" + std::to_string(j), witness);
            crime.witnesses.push_back(witness);
        }
        
        m_activeCrimes.push_back(crime);
    }
    
    // Read settings
    reader.Read("crimeExpirationHours", m_crimeExpirationHours);
    reader.Read("currentGameTime", m_currentGameTime);
    
    LOG(Info, "CrimeSystem deserialized from text");
}
// ^ CrimeSystem.cpp