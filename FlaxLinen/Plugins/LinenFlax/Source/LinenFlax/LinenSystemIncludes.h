#pragma once

// This file includes all system headers to avoid circular dependencies

// Include all standard library headers first
// #include <string>
// #include <vector>
// #include <unordered_map>
// #include <unordered_set>
// #include <memory>
// #include <functional>

class RPGSystem;
class TestSystem;
class CharacterProgressionSystem;
class TimeSystem;
class QuestSystem;

class EconomySystem;
class WeatherSystem;
class WorldProgressionSystem;

class RelationshipSystem;
class FactionSystem;
class CrimeSystem;

class SaveLoadSystem;

// Then include all system header files in proper order
#include "LinenSystem.h"
#include "RPGSystem.h"
#include "Serialization.h"

#include "CharacterProgressionSystem.h"
#include "TimeSystem.h"
#include "QuestTypes.h"
#include "QuestEvents.h"
#include "QuestSystem.h"

#include "EconomySystem.h"
#include "WeatherSystem.h"
#include "WorldProgressionSystem.h"

#include "RelationshipSystem.h"
#include "FactionSystem.h"
#include "CrimeSystem.h"

#include "SaveLoadSystem.h"