// v SaveLoadSystem.cpp
#include "SaveLoadSystem.h"
#include "LinenFlax.h"
#include "LinenSystemIncludes.h"
#include "Engine/Core/Log.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

SaveLoadSystem::~SaveLoadSystem() {
    Destroy();
}

void SaveLoadSystem::Initialize() {
    // Register all known systems for serialization
    RegisterSerializableSystem("CharacterProgressionSystem");
    RegisterSerializableSystem("QuestSystem");
    RegisterSerializableSystem("TestSystem");
    RegisterSerializableSystem("TimeSystem");
    
    LOG(Info, "Save/Load System Initialized.");
}

void SaveLoadSystem::Shutdown() {
    m_serializableSystems.clear();
    LOG(Info, "Save/Load System Shutdown.");
}

void SaveLoadSystem::Update(float deltaTime) {
    // Nothing to update regularly
}

std::string SaveLoadSystem::GetExtensionForFormat(SerializationFormat format) const {
    switch (format) {
        case SerializationFormat::Binary: return ".bin";
        case SerializationFormat::Text: return ".txt";
        default: return ".sav";
    }
}

SerializationFormat SaveLoadSystem::GetFormatFromFilename(const std::string& filename) const {
    fs::path filePath(filename);
    std::string extension = filePath.extension().string();
    
    if (extension == ".txt") {
        return SerializationFormat::Text;
    } else if (extension == ".bin") {
        return SerializationFormat::Binary;
    } else {
        // Default to binary for unknown extensions
        return SerializationFormat::Binary;
    }
}

std::string SaveLoadSystem::EnsureCorrectExtension(const std::string& filename, SerializationFormat format) const {
    fs::path filePath(filename);
    std::string baseFilename = filePath.stem().string();
    std::string correctExtension = GetExtensionForFormat(format);
    
    return baseFilename + correctExtension;
}

RPGSystem* SaveLoadSystem::GetSystemByName(const std::string& systemName) {
    if (systemName == "CharacterProgressionSystem") {
        return m_plugin->GetSystem<CharacterProgressionSystem>();
    } else if (systemName == "QuestSystem") {
        return m_plugin->GetSystem<QuestSystem>();
    } else if (systemName == "TestSystem") {
        return m_plugin->GetSystem<TestSystem>();
    } else if (systemName == "TimeSystem") {
        return m_plugin->GetSystem<TimeSystem>();
    }
    return nullptr;
}

bool SaveLoadSystem::SaveGame(const std::string& filename, SerializationFormat format) {    
    std::string saveFilename = EnsureCorrectExtension(filename, format);

    const char* formatName = format == SerializationFormat::Binary ? "Binary" : "Text";
    LOG(Info, "Saving game to: {0} (Format: {1})", String(saveFilename.c_str()), String(formatName));
    
    try {
        if (format == SerializationFormat::Binary) {
            BinaryWriter writer(saveFilename);
            if (!writer.IsValid()) {
                LOG(Error, "Failed to create save file: {0}", String(saveFilename.c_str()));
                return false;
            }
            
            // Write header information
            writer.Write(static_cast<uint32_t>(m_serializableSystems.size()));
            
            // For each registered system, call its Serialize method
            for (const auto& systemName : m_serializableSystems) {
                writer.Write(systemName); // Write system name
                
                auto system = GetSystemByName(systemName);
                if (system) {
                    system->Serialize(writer);
                    LOG(Info, "Saved system: {0}", String(systemName.c_str()));
                } else {
                    LOG(Warning, "System not found for serialization: {0}", String(systemName.c_str()));
                    // Write empty placeholder
                    uint32_t size = 0;
                    writer.Write(size);
                }
            }
        } else { // Text format
            TextWriter textWriter;
            
            // Write version info
            textWriter.Write("version", std::string("1.0.0"));  // Convert char* to std::string
            textWriter.Write("systemCount", static_cast<int>(m_serializableSystems.size()));
            
            // Write system names
            int index = 0;
            for (const auto& systemName : m_serializableSystems) {
                textWriter.Write("system" + std::to_string(index), systemName);
                index++;
            }
            
            // For each registered system, call its SerializeToText method
            for (const auto& systemName : m_serializableSystems) {
                auto system = GetSystemByName(systemName);
                if (system) {
                    if (systemName == "CharacterProgressionSystem") {
                        static_cast<CharacterProgressionSystem*>(system)->SerializeToText(textWriter);
                    } else if (systemName == "QuestSystem") {
                        static_cast<QuestSystem*>(system)->SerializeToText(textWriter);
                    } else if (systemName == "TestSystem") {
                        static_cast<TestSystem*>(system)->SerializeToText(textWriter);
                    } else if (systemName == "TimeSystem") {
                        static_cast<TimeSystem*>(system)->SerializeToText(textWriter);
                    }
                    LOG(Info, "Saved system to text: {0}", String(systemName.c_str()));
                } else {
                    LOG(Warning, "System not found for text serialization: {0}", String(systemName.c_str()));
                }
            }
            
            // Save to file
            if (!textWriter.SaveToFile(saveFilename)) {
                LOG(Error, "Failed to write text save file: {0}", String(saveFilename.c_str()));
                return false;
            }
        }
        
        LOG(Info, "Game saved successfully: {0}", String(saveFilename.c_str()));
        return true;
    } catch (const std::exception& e) {
        LOG(Error, "Exception during save: {0}", String(e.what()));
        return false;
    }
}

bool SaveLoadSystem::LoadGame(const std::string& filename, SerializationFormat format) {    
    std::string loadFilename = EnsureCorrectExtension(filename, format);
    
    if (!fs::exists(loadFilename)) {
        LOG(Error, "Save file not found: {0}", String(loadFilename.c_str()));
        return false;
    }
    
    const char* formatName = format == SerializationFormat::Binary ? "Binary" : "Text";
    LOG(Info, "Loading game from: {0} (Format: {1})", String(loadFilename.c_str()), String(formatName));

    try {
        if (format == SerializationFormat::Binary) {
            BinaryReader reader(loadFilename);
            if (!reader.IsValid()) {
                LOG(Error, "Failed to open save file: {0}", String(loadFilename.c_str()));
                return false;
            }
            
            // Read header information
            uint32_t systemCount = 0;
            reader.Read(systemCount);
            
            // For each system in the file
            for (uint32_t i = 0; i < systemCount; ++i) {
                std::string systemName;
                reader.Read(systemName);
                
                auto system = GetSystemByName(systemName);
                if (system) {
                    system->Deserialize(reader);
                    LOG(Info, "Loaded system: {0}", String(systemName.c_str()));
                } else {
                    LOG(Warning, "System not found for deserialization: {0}", String(systemName.c_str()));
                    // Skip data
                    uint32_t size = 0;
                    reader.Read(size);
                    for (uint32_t j = 0; j < size; j++) {
                        char dummy;
                        reader.Read(&dummy, 1);
                    }
                }
            }
        } else { // Text format
            TextReader textReader;
            if (!textReader.LoadFromFile(loadFilename)) {
                LOG(Error, "Failed to parse text save file: {0}", String(loadFilename.c_str()));
                return false;
            }
            
            // Version check (optional)
            std::string version;
            if (textReader.Read("version", version)) {
                LOG(Info, "Save file version: {0}", String(version.c_str()));
            }
            
            // Get system count
            int systemCount = 0;
            if (!textReader.Read("systemCount", systemCount)) {
                LOG(Error, "Invalid system count in save file");
                return false;
            }
            
            // Load systems
            for (int i = 0; i < systemCount; i++) {
                std::string systemName;
                if (!textReader.Read("system" + std::to_string(i), systemName)) {
                    LOG(Warning, "Missing system name at index {0}", i);
                    continue;
                }
                
                auto system = GetSystemByName(systemName);
                if (system) {
                    if (systemName == "CharacterProgressionSystem") {
                        static_cast<CharacterProgressionSystem*>(system)->DeserializeFromText(textReader);
                    } else if (systemName == "QuestSystem") {
                        static_cast<QuestSystem*>(system)->DeserializeFromText(textReader);
                    } else if (systemName == "TestSystem") {
                        static_cast<TestSystem*>(system)->DeserializeFromText(textReader);
                    } else if (systemName == "TimeSystem") {
                        static_cast<TimeSystem*>(system)->DeserializeFromText(textReader);
                    }
                    
                    LOG(Info, "Loaded system from text: {0}", String(systemName.c_str()));
                } else {
                    LOG(Warning, "System not found for text deserialization: {0}", String(systemName.c_str()));
                }
            }
        }
        
        LOG(Info, "Game loaded successfully: {0}", String(loadFilename.c_str()));
        return true;
    } catch (const std::exception& e) {
        LOG(Error, "Exception during load: {0}", String(e.what()));
        return false;
    }
}

void SaveLoadSystem::RegisterSerializableSystem(const std::string& systemName) {
    m_serializableSystems.insert(systemName);
    LOG(Info, "Registered system for serialization: {0}", String(systemName.c_str()));
}

// Default binary serialization
void SaveLoadSystem::Serialize(BinaryWriter& writer) const {
    // Save system's own data (if any)
    writer.Write(static_cast<uint32_t>(m_serializableSystems.size()));
    
    for (const auto& systemName : m_serializableSystems) {
        writer.Write(systemName);
    }
}

// Default binary deserialization
void SaveLoadSystem::Deserialize(BinaryReader& reader) {
    // Load system's own data (if any)
    uint32_t systemCount = 0;
    reader.Read(systemCount);
    
    m_serializableSystems.clear();
    for (uint32_t i = 0; i < systemCount; ++i) {
        std::string systemName;
        reader.Read(systemName);
        m_serializableSystems.insert(systemName);
    }
}

// Text serialization
void SaveLoadSystem::SerializeToText(TextWriter& writer) const {
    // Create list of system names
    std::vector<std::string> systems;
    for (const auto& systemName : m_serializableSystems) {
        systems.push_back(systemName);
    }
    
    // Write the number of systems
    writer.Write("systemCount", static_cast<int>(systems.size()));
    
    // Write each system name
    for (size_t i = 0; i < systems.size(); ++i) {
        writer.Write("system" + std::to_string(i), systems[i]);
    }
}

// Text deserialization
void SaveLoadSystem::DeserializeFromText(TextReader& reader) {
    m_serializableSystems.clear();
    
    std::vector<std::string> systems;
    if (reader.ReadVector("registeredSystems", systems)) {
        for (const auto& systemName : systems) {
            m_serializableSystems.insert(systemName);
        }
    }
}
// ^ SaveLoadSystem.h