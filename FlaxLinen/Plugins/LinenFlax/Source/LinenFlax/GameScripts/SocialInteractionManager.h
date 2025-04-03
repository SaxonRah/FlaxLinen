// v SocialInteractionManager.h
#pragma once

#include "Engine/Scripting/Script.h"
#include "Engine/Core/Math/Vector3.h"
#include "../RelationshipSystem.h"
#include "../FactionSystem.h"
#include "../CrimeSystem.h"

#include <string>

class LinenFlax;

API_CLASS() class LINENFLAX_API SocialInteractionManager : public Script
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE(SocialInteractionManager);

public:
    // Crime system settings
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"Crime Settings\"), Tooltip(\"How long crimes remain tracked in game hours\")")
    int32 CrimeExpirationHours = 72;
    
    // Default bounty values for common crimes
    API_FIELD(Attributes="EditorOrder(1), EditorDisplay(\"Crime Settings\"), Tooltip(\"Base bounty for trespassing\")")
    int32 TrespassingBounty = 10;
    
    API_FIELD(Attributes="EditorOrder(2), EditorDisplay(\"Crime Settings\"), Tooltip(\"Base bounty for theft\")")
    int32 TheftBounty = 25;
    
    API_FIELD(Attributes="EditorOrder(3), EditorDisplay(\"Crime Settings\"), Tooltip(\"Base bounty for assault\")")
    int32 AssaultBounty = 40;
    
    API_FIELD(Attributes="EditorOrder(4), EditorDisplay(\"Crime Settings\"), Tooltip(\"Base bounty for murder\")")
    int32 MurderBounty = 100;
    
    // Reputation decay settings
    API_FIELD(Attributes="EditorOrder(5), EditorDisplay(\"Faction Settings\"), Tooltip(\"How quickly faction reputation decays towards neutral (in points per day)\")")
    int32 FactionReputationDecayPerDay = 1;
    
    // NPC Relationship settings
    API_FIELD(Attributes="EditorOrder(6), EditorDisplay(\"Relationship Settings\"), Tooltip(\"Default relationship value for new NPCs\")")
    int32 DefaultRelationshipValue = 0;
    
    // Debug options
    API_FIELD(Attributes="EditorOrder(7), EditorDisplay(\"Debug\"), Tooltip(\"Show debug information about social systems\")")
    bool DebugLogging = false;
    
    API_FIELD(Attributes="EditorOrder(8), EditorDisplay(\"Debug\"), Tooltip(\"Test character ID for debug operations\")")
    String DebugCharacterId = String("player");
    
    API_FIELD(Attributes="EditorOrder(9), EditorDisplay(\"Debug\"), Tooltip(\"Test faction ID for debug operations\")")
    String DebugFactionId = String("city_guard");
    
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Debug\"), Tooltip(\"Test region ID for debug operations\")")
    String DebugRegionId = String("city");
    
    API_FIELD(Attributes="EditorOrder(11), EditorDisplay(\"Debug\"), Tooltip(\"Test NPC ID for debug operations\")")
    String DebugNpcId = String("guard_captain");
    
    API_FIELD(Attributes="EditorOrder(12), EditorDisplay(\"Debug\"), Tooltip(\"Add reputation to test relationship between player and faction\")")
    bool DebugAddFactionReputation = false;
    
    API_FIELD(Attributes="EditorOrder(13), EditorDisplay(\"Debug\"), Tooltip(\"Amount of reputation to add\")")
    int32 DebugReputationAmount = 100;
    
    API_FIELD(Attributes="EditorOrder(14), EditorDisplay(\"Debug\"), Tooltip(\"Add a test bounty to the player\")")
    bool DebugAddBounty = false;
    
    API_FIELD(Attributes="EditorOrder(15), EditorDisplay(\"Debug\"), Tooltip(\"Test crime type to report (0-5)\")")
    int32 DebugCrimeType = 0;

private:
    // Track previous debug flags to detect changes
    bool m_prevAddReputation = false;
    bool m_prevAddBounty = false;
    
    // Track systems
    RelationshipSystem* m_relationshipSystem = nullptr;
    FactionSystem* m_factionSystem = nullptr;
    CrimeSystem* m_crimeSystem = nullptr;
    LinenFlax* m_plugin = nullptr;

    std::string FlaxStringToStdString(const String& flaxString);
    String StdStringToFlaxString(const std::string& stdString);

public:
    // Called when script is being initialized
    void OnEnable() override;
    
    // Called when script is being disabled
    void OnDisable() override;
    
    // Called when script is updated (once per frame)
    void OnUpdate() override;
    
    // Initialize test data for debugging
    void InitializeTestData();
    
    // Helper to handle relationship changes
    void OnRelationshipChanged(const std::string& characterId, const std::string& targetId, int newValue);
    
    // Helper to handle reputation changes
    void OnReputationChanged(const std::string& characterId, const std::string& factionId, int newValue);
    
    // Helper to handle bounty changes
    void OnBountyChanged(const std::string& characterId, const std::string& regionId, int newBounty);
};
// ^ SocialInteractionManager.h