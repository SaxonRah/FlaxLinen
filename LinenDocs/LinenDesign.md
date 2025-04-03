# Linen: A Modular RPG System for Flax Engine

Linen is a sophisticated and modular RPG framework designed to be engine agnostic while being implemented in the Flax Engine.
The system's architecture emphasizes modularity, extensibility, and clean interaction patterns between components.

## Core Architecture

Linen is built as a **Flax Plugin** that manages various RPG systems through a modular architecture. The key design principles include:
1. **Modularity**: Each RPG system (quests, character progression, economy, etc.) operates independently and can be loaded/unloaded dynamically.
2. **Dependency Management**: Systems can declare dependencies on other systems, ensuring proper initialization order.
3. **Event-Driven Communication**: Systems communicate primarily through an event system rather than direct references, maintaining loose coupling.
4. **Thread Safety**: The implementation includes mutex locks and other mechanisms to ensure thread safety for concurrent operations.
5. **Serialization**: A comprehensive save/load system with binary serialization for efficient storage and loading of game state.

## System Organization

Linen organizes its functionality into five implementation phases:

1. **Core Systems** (Fundamental Gameplay Mechanics)
   - Character Progression, Quests, Dialogue, Time, Save/Load

2. **World and Economy Systems**
   - Weather, Economy/Market, World Progression

3. **Social and NPC Interaction Systems**
   - NPC Relationships, Faction Reputation, Crime and Law

4. **Gameplay Enhancements**
   - Property Ownership, Mounts, Disease, Spell Crafting, Religion

5. **Additional Systems**
   - Survival, Stealth, Archery, Hunting, Guild, Aging, Legacy

## Technical Implementation

The C++ implementation follows modern best practices:

1. **Template-Based System Access**: Type-safe access to systems using templates
   ```cpp
   template <typename T>
   T* GetSystem();
   ```

2. **Smart Pointers**: Consistent use of `std::unique_ptr` for automatic resource management
   ```cpp
   std::unordered_map<std::string, std::unique_ptr<RPGSystem>> m_registeredSystems;
   ```

3. **Event System**: Prioritized event processing with filtering capabilities
   ```cpp
   void Publish(const T& event, const std::string& filter = "", EventPriority priority = EventPriority::Normal)
   ```

4. **Topological Sorting**: Ensures systems initialize in the correct order based on dependencies
   ```cpp
   void CalculateInitializationOrder();
   ```

## System Interactions

One of Linen's key strengths is its well-defined system interaction model:

1. **Event-Driven Communication**: Systems publish events that others can subscribe to
   ```cpp
   // Quest System publishes event
   QuestCompletedEvent event;
   m_plugin->GetEventSystem().Publish(event);
   
   // Character System subscribes to event
   void CharacterProgressionSystem::Initialize() {
       m_plugin->GetEventSystem().Subscribe<QuestCompletedEvent>(
           [this](const QuestCompletedEvent& event) { 
               this->HandleQuestCompleted(event);
           });
   }
   ```

2. **Interaction Matrix**: The documentation includes a comprehensive matrix showing how each system interacts with others, making the architecture easier to understand and maintain.

3. **Emergent Gameplay**: These interactions create emergent gameplay where, for example, weather can affect NPC behavior, which affects the economy, which affects quest availability.

## Design Strengths

1. **Binary Serialization**: Efficient save/load system with versioning for backward compatibility.
2. **Thread Safety**: Careful consideration of concurrency issues with proper mutex usage.
3. **Type Safety**: Extensive use of C++ type system to prevent errors at compile time.
4. **Dependency Resolution**: Automatic calculation of initialization and shutdown order.
5. **Event Filtering**: The event system supports filtered subscriptions for targeted event handling.

---

# Design Patterns in the Linen RPG System

The Linen RPG System incorporates numerous software design patterns that help structure the codebase, promote modularity, and manage complexity.

The extensive use of these design patterns demonstrates that Linen is following established software engineering principles to create a maintainable, extensible, and robust RPG system.

The event-driven architecture in particular is central to the system's design, enabling the rich interactions between subsystems while maintaining modularity.

## Creational Patterns

1. **Factory Method**
   - Used in the system creation and registration process within `Linen`
   - `RegisterSystem<T>()` creates concrete system instances

2. **Singleton** (Limited Usage)
   - The `EventBus` static implementation follows a singleton-like pattern
   - Provides global access to the event system

3. **Builder** (Implicit)
   - Quests and other game entities are constructed incrementally
   - Example: Building quests with requirements, rewards, and descriptions

## Structural Patterns

4. **Facade**
   - `Linen` acts as a facade to the complex underlying systems
   - Provides simplified access to the RPG subsystems

5. **Proxy**
   - Type-safe system access through `GetSystem<T>()` acts as a proxy
   - Handles system retrieval and type checking

6. **Composite**
   - The hierarchical organization of RPG systems
   - Systems can contain subsystems or components

7. **Adapter**
   - The event system adapts different event types to a common interface
   - Allows heterogeneous events to be handled uniformly

8. **Bridge**
   - Separation between system interfaces and implementations
   - Abstract `RPGSystem` base class with concrete implementations

## Behavioral Patterns

9. **Observer**
   - Primary pattern for system interactions via the event system
   - Systems subscribe to events published by other systems
   - Example: Character Progression subscribes to Quest completion events

10. **Command**
    - Event objects encapsulate actions or requests
    - Events are queued and processed later

11. **Chain of Responsibility**
    - Event handling with multiple subscribers forms a chain
    - Each subscriber handles the event independently

12. **Strategy**
    - Different systems can be loaded/unloaded dynamically
    - Allows for different algorithms to be selected at runtime

13. **Template Method**
    - Base `RPGSystem` defines the structure with Initialize/Shutdown/Update methods
    - Derived systems implement the specific behaviors

14. **State**
    - Quest states (Available, Active, Completed, Failed)
    - System states during initialization and runtime

15. **Visitor** (Partial)
    - Serialization mechanism where systems "visit" different components
    - Each system implements its own serialization logic

16. **Mediator**
    - `Linen` acts as a mediator between different systems
    - Manages how systems discover and interact with each other

## Concurrency Patterns

17. **Monitor Object**
    - Mutex protection of shared resources
    - Thread-safe event publishing and system access

18. **Producer-Consumer**
    - Event publishing (producers) and event handling (consumers)
    - Events are queued for processing

## Architectural Patterns

19. **Component-Based Architecture**
    - Systems are modular components that can be composed
    - Enables flexible system configuration

20. **Event-Driven Architecture**
    - Core interaction model through the event system
    - Loose coupling between systems

21. **Dependency Injection**
    - Systems receive references to the plugin and other systems
    - `SetPlugin(Linen* plugin)` to inject dependencies

22. **Service Locator**
    - Systems locate other systems through the plugin
    - `m_plugin->GetSystem<T>()` to locate dependencies

23. **Layered Architecture**
    - Organized into phases/layers (Core Systems, World Systems, etc.)
    - Higher-level systems depend on lower-level ones

24. **Publish-Subscribe**
    - Core event communication pattern
    - Publishers don't know about subscribers

25. **Command Query Separation**
    - Methods either modify state or return data, rarely both
    - Example: Query methods like `GetActiveQuests()` vs. command methods like `CompleteQuest()`
