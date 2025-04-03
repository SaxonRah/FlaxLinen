// v QuestTypes.h
#pragma once

// Forward declarations and common types for Quest-related classes
class Quest;

// Quest state enum that can be used by multiple files
enum class QuestState {
    Available,
    Active, 
    Completed,
    Failed
};

enum class QuestResult {
    Success,            
    NotFound,           // Quest ID not found
    AlreadyExists,      // When trying to add a quest that already exists
    InvalidState,       // Quest is in wrong state for the operation
    RequirementsNotMet, // Player doesn't meet skill requirements
    Error               // Generic error
};
// ^ QuestTypes.h