// v Serialization.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdint>
#include <sstream>

enum class SerializationFormat {
    Binary,
    Text
};

class BinaryWriter {
public:
    BinaryWriter(const std::string& filename) : m_stream(filename, std::ios::binary | std::ios::out) {}
    ~BinaryWriter() { m_stream.close(); }
    
    bool IsValid() const { return m_stream.good(); }
    
    // Write primitives
    void Write(bool value) { Write(&value, sizeof(bool)); }
    void Write(int32_t value) { Write(&value, sizeof(int32_t)); }
    void Write(uint32_t value) { Write(&value, sizeof(uint32_t)); }
    void Write(float value) { Write(&value, sizeof(float)); }
    void Write(double value) { Write(&value, sizeof(double)); }
    
    // Write string
    void Write(const std::string& value) {
        uint32_t length = static_cast<uint32_t>(value.length());
        Write(length);
        if (length > 0) {
            Write(value.data(), length);
        }
    }
    
    // Write raw data
    void Write(const void* data, size_t size) {
        m_stream.write(static_cast<const char*>(data), size);
    }
    
    // Write container helpers
    template<typename T>
    void WriteVector(const std::vector<T>& vec) {
        uint32_t size = static_cast<uint32_t>(vec.size());
        Write(size);
        for (const auto& item : vec) {
            Write(item);
        }
    }
    
    // Write map
    template<typename K, typename V>
    void WriteMap(const std::unordered_map<K, V>& map) {
        uint32_t size = static_cast<uint32_t>(map.size());
        Write(size);
        for (const auto& pair : map) {
            Write(pair.first);
            Write(pair.second);
        }
    }
    
private:
    std::ofstream m_stream;
};

class BinaryReader {
public:
    BinaryReader(const std::string& filename) : m_stream(filename, std::ios::binary | std::ios::in) {}
    ~BinaryReader() { m_stream.close(); }
    
    bool IsValid() const { return m_stream.good() && !m_stream.eof(); }
    
    // Read primitives
    void Read(bool& value) { Read(&value, sizeof(bool)); }
    void Read(int32_t& value) { Read(&value, sizeof(int32_t)); }
    void Read(uint32_t& value) { Read(&value, sizeof(uint32_t)); }
    void Read(float& value) { Read(&value, sizeof(float)); }
    void Read(double& value) { Read(&value, sizeof(double)); }
    
    // Read string
    void Read(std::string& value) {
        uint32_t length = 0;
        Read(length);
        value.resize(length);
        if (length > 0) {
            Read(&value[0], length);
        }
    }
    
    // Read raw data
    void Read(void* data, size_t size) {
        m_stream.read(static_cast<char*>(data), size);
    }
    
    // Read container helpers
    template<typename T>
    void ReadVector(std::vector<T>& vec) {
        uint32_t size = 0;
        Read(size);
        vec.resize(size);
        for (uint32_t i = 0; i < size; ++i) {
            Read(vec[i]);
        }
    }
    
    // Read map
    template<typename K, typename V>
    void ReadMap(std::unordered_map<K, V>& map) {
        uint32_t size = 0;
        Read(size);
        map.clear();
        K key;
        V value;
        for (uint32_t i = 0; i < size; ++i) {
            Read(key);
            Read(value);
            map[key] = value;
        }
    }
    
private:
    std::ifstream m_stream;
};

// Simple text-based serialization
class TextWriter {
public:
    TextWriter() {}
    
    // Generic value writing template
    template<typename T>
    void Write(const std::string& key, const T& value) {
        std::stringstream ss;
        ss << value;
        m_data[key] = ss.str();
    }
    
    // Specialization for strings (to avoid the to_string error)
    void Write(const std::string& key, const std::string& value) {
        m_data[key] = value;
    }
    
    // Specialization for string literals
    void Write(const std::string& key, const char* value) {
        m_data[key] = value;
    }
    
    // Write vector as comma-separated values
    template<typename T>
    void WriteVector(const std::string& key, const std::vector<T>& vec) {
        std::stringstream ss;
        for (size_t i = 0; i < vec.size(); ++i) {
            ss << vec[i];
            if (i < vec.size() - 1) {
                ss << ",";
            }
        }
        m_data[key] = ss.str();
    }
    
    // Write map as key-value pairs
    template<typename V>
    void WriteMap(const std::string& key, const std::unordered_map<std::string, V>& map) {
        std::stringstream ss;
        size_t index = 0;
        for (const auto& pair : map) {
            ss << pair.first << "=" << pair.second;
            if (index < map.size() - 1) {
                ss << ";";
            }
            index++;
        }
        m_data[key] = ss.str();
    }
    
    bool SaveToFile(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        // Write in a simple key=value format
        for (const auto& pair : m_data) {
            file << pair.first << "=" << pair.second << std::endl;
        }
        file.close();
        return true;
    }
    
private:
    std::unordered_map<std::string, std::string> m_data;
};

class TextReader {
public:
    TextReader() {}
    
    bool LoadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                m_data[key] = value;
            }
        }
        file.close();
        return true;
    }
    
    template<typename T>
    bool Read(const std::string& key, T& value) {
        auto it = m_data.find(key);
        if (it == m_data.end()) {
            return false;
        }
        
        std::stringstream ss(it->second);
        ss >> value;
        return !ss.fail();
    }
    
    // Specialization for strings
    bool Read(const std::string& key, std::string& value) {
        auto it = m_data.find(key);
        if (it == m_data.end()) {
            return false;
        }
        
        value = it->second;
        return true;
    }
    
    // Read vector from comma-separated values
    template<typename T>
    bool ReadVector(const std::string& key, std::vector<T>& vec) {
        auto it = m_data.find(key);
        if (it == m_data.end()) {
            return false;
        }
        
        std::string value = it->second;
        std::stringstream ss(value);
        std::string item;
        vec.clear();
        
        while (std::getline(ss, item, ',')) {
            T val;
            std::stringstream itemSS(item);
            itemSS >> val;
            if (!itemSS.fail()) {
                vec.push_back(val);
            }
        }
        
        return true;
    }
    
    // Read map from key-value pairs
    template<typename V>
    bool ReadMap(const std::string& key, std::unordered_map<std::string, V>& map) {
        auto it = m_data.find(key);
        if (it == m_data.end()) {
            return false;
        }
        
        std::string value = it->second;
        std::stringstream ss(value);
        std::string pair;
        map.clear();
        
        while (std::getline(ss, pair, ';')) {
            size_t pos = pair.find('=');
            if (pos != std::string::npos) {
                std::string mapKey = pair.substr(0, pos);
                std::string mapValue = pair.substr(pos + 1);
                
                V val;
                std::stringstream valSS(mapValue);
                valSS >> val;
                if (!valSS.fail()) {
                    map[mapKey] = val;
                }
            }
        }
        
        return true;
    }
    
private:
    std::unordered_map<std::string, std::string> m_data;
};
// ^ Serialization.h