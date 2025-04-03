// v SaveLoadSystem.h
#pragma once

#include "RPGSystem.h"
#include "Serialization.h"
#include <string>
#include <unordered_set>
#include <functional>
#include <unordered_map>
#include <memory>

class SaveLoadSystem : public RPGSystem {
public:
    
    // Delete copy constructor and assignment operator
    SaveLoadSystem(const SaveLoadSystem&) = delete;
    SaveLoadSystem& operator=(const SaveLoadSystem&) = delete;

    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;

    std::string GetName() const override { return "SaveLoadSystem"; }
    
    // Save/Load functionality
    bool SaveGame(const std::string& filename, SerializationFormat format = SerializationFormat::Binary);
    bool LoadGame(const std::string& filename, SerializationFormat format = SerializationFormat::Binary);
    
    // System registration for save/load
    void RegisterSerializableSystem(const std::string& systemName);
    
    // Default serialization methods
    virtual void Serialize(BinaryWriter& writer) const override;
    virtual void Deserialize(BinaryReader& reader) override;
    
    // Text serialization methods
    virtual void SerializeToText(TextWriter& writer) const;
    virtual void DeserializeFromText(TextReader& reader);
    
    // Meyer's Singleton - thread-safe in C++11 and beyond
    static SaveLoadSystem* GetInstance() {
        // Thread-safe in C++11 and beyond
        static SaveLoadSystem* instance = new SaveLoadSystem();
        return instance;
    }

    // Cleanup method
    static void Destroy() {
        static SaveLoadSystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }
    
    ~SaveLoadSystem();

private:
    SaveLoadSystem() {};
    
    // Track which systems need serialization
    std::unordered_set<std::string> m_serializableSystems;
    
    // Helper to get the correct system pointer based on name
    RPGSystem* GetSystemByName(const std::string& systemName);
    
    // Helper functions for file extension management
    std::string GetExtensionForFormat(SerializationFormat format) const;
    SerializationFormat GetFormatFromFilename(const std::string& filename) const;
    std::string EnsureCorrectExtension(const std::string& filename, SerializationFormat format) const;
};
// ^ SaveLoadSystem.h