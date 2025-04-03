// v LinenSystem.h
#pragma once
#include <string>
#include "Serialization.h"

// Simple base system class
class LinenSystem {
public:
    virtual ~LinenSystem() = default;
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(float deltaTime) {}
    virtual std::string GetName() const = 0;
    
    // Serialization methods
    virtual void Serialize(BinaryWriter& writer) const { /* Default empty implementation */ }
    virtual void Deserialize(BinaryReader& reader) { /* Default empty implementation */ }
    virtual void SerializeToText(TextWriter& writer) const { /* Default empty implementation */ }
    virtual void DeserializeFromText(TextReader& reader) { /* Default empty implementation */ }

};
// ^ LinenSystem.h