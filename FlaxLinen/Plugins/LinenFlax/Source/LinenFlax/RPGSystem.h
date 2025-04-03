// v RPGSystem.h
#pragma once

#include <string>
#include <vector>
#include <unordered_set>

#include "LinenSystem.h"

// Forward declaration
class LinenFlax;
class BinaryReader;
class BinaryWriter;

// Base class for all RPG systems
class RPGSystem : public LinenSystem {
public:
    virtual ~RPGSystem() = default;
    
    // System dependencies
    const std::unordered_set<std::string>& GetDependencies() const { return m_dependencies; }
    
    // Plugin reference for accessing other systems
    void SetPlugin(LinenFlax* plugin) { m_plugin = plugin; }

protected:
    LinenFlax* m_plugin = nullptr;
    std::unordered_set<std::string> m_dependencies;
};
// ^ RPGSystem.h