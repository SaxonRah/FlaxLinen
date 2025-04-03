# Linen - A Modular RPG System for Flax Engine

Linen is a **Flax Plugin** that acts as a manager for various RPG systems, allowing developers to:
- Load/unload systems dynamically
- Configure individual parameters per system
- Extend or replace components for custom implementations

## Implementation Phases
1. - Phase 1: Core Systems (Fundamental Gameplay Mechanics)
        - Character Progression System
        - Quest System
        - Dialogue System
        - Time System
        - Save/Load System

2. - Phase 2: World and Economy Systems
        - Advanced Weather System
        - Economy/Market System
        - World Progression System

3. - Phase 3: Social and NPC Interaction Systems
        - NPC Relationship System
        - Faction Reputation System
        - Crime and Law System

4. - Phase 4: Gameplay Enhancements and Additional Mechanics
        - Property Ownership System
        - Horse/Mount System
        - Disease and Health System
        - Spell Crafting System
        - Crafting System
        - Religion and Deity System

5. - Phase 5: Additional Systems
        - Survival System
        - Stealth System
        - Archery System
        - Hunting System
        - Guild System
        - Aging System
        - Legacy System


## Phase 1: Core Systems (Fundamental Gameplay Mechanics)
#### Character Progression System
- Skill tree with hierarchical abilities
- Class specializations: *Warrior, Ranger, Alchemist, etc.*
- Skill requirements and prerequisites
- Stat improvements from skills
- Special ability unlocks

#### Quest System
- Main quests and sub-quests
- Requirements based on skills, items, or reputation
- Rewards: *experience, items, faction reputation*
- Quest states: *Available, Active, Completed, Failed*
- Journal-based quest tracking

#### Dialogue System
- Branching dialogue trees
- Character responses based on player stats and knowledge
- Dialogue history tracking
- NPC relationship integration
- Dialogue-triggered effects: *quest activation, reputation changes*

#### Time System
- Day/night cycle
- Seasonal changes
- Time-based events
- Waiting/resting mechanics

#### Save/Load System
- Persistent IDs for object tracking
- Full game state serialization/deserialization
- Error handling and integrity checks
- Version compatibility support

## Phase 2: World and Economy Systems
#### Advanced Weather System
- Weather states: *Clear, Cloudy, Rainy, Stormy, Snowy*
- Region-specific weather patterns
- Seasonal weather probabilities
- Impact on travel, combat, and visibility
- Weather-specific quests and events

#### Economy/Market System
- Dynamic pricing based on supply/demand
- Region-specific price variations and specialty goods
- Market fluctuations tied to world events
- Trade routes that can be disrupted or improved
- Investment opportunities in businesses

#### World Progression System
- Geographic regions with connectivity
- Locations within regions
- Location states that evolve with story progression
- NPCs tied to specific locations
- Location-specific activities
- Access requirements for certain areas

## Phase 3: Social and NPC Interaction Systems
#### NPC Relationship System
- Friendship, rivalry, and romance mechanics
- Gift preferences and conversation options
- Companion loyalty and mood tracking
- NPC schedules and routines
- Background relationship networks

#### Faction Reputation System
- Player standing with various factions
- Reputation tiers: *hostile, unfriendly, neutral, friendly, allied*
- Faction-specific quests, dialogues, and shop prices
- Faction conflicts where aiding one faction damages another
- Political shifts based on player actions

#### Crime and Law System
- Witness-based crime detection
- Bounty accumulation for offenses
- Guard interactions: *arrest, resist, bribe, flee*
- Jail time and punishment mechanics
- Criminal reputation affecting NPC interactions

## Phase 4: Gameplay Enhancements and Additional Mechanics
#### Property Ownership System
- Purchasable properties: *houses, shops, land*
- Property upgrades and maintenance
- Tenant management and income generation
- Storage for items and equipment
- Property-specific quests and events

#### Horse/Mount System
- Mount acquisition and training
- Mount stats: *speed, stamina, carrying capacity*
- Mount equipment and upgrades
- Special movement abilities: *jumping ravines, climbing*
- Mount care: *feeding, stabling, healing*

#### Disease and Health System
- Contractable diseases with progressive states
- Symptom severity based on time and treatment
- Regional disease outbreaks
- Immunity development
- Healing methods: *potions, rest, healer NPCs*

#### Spell Crafting System
- Component-based spell creation
- Effect combination nodes
- Scaling based on skill levels
- Magical research and experimentation
- Spell failure and backfire states

#### Crafting System
- Crafting stations: *blacksmith, alchemy, cooking, etc.*
- Recipe discovery
- Ingredient requirements
- Skill-based crafting mechanics
- Unique item properties for crafted items

#### Religion and Deity System
- Multiple deities with favor tracking
- Temple quests and prayer benefits
- Divine intervention based on devotion
- Religious conflicts between pantheons
- Sacred days with special events

### Optional Additional Systems (For Enhanced Realism & Immersion)
- **Survival System:** Hunger, thirst, fatigue mechanics
- **Stealth System:** Stealth and theft mechanics
- **Archery System:** Bow/Crossbow and Arrow/Bolt mechanics
- **Hunting System:** Hunting wild animals for resources
- **Guild System:** Joinable guilds with ranking and privileges
- **Aging System:** Character aging with stat effects
- **Legacy System:** Passing traits to new characters after permadeath
