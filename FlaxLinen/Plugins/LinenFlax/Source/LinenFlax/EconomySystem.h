// v EconomySystem.h
#pragma once

#include "EventSystem.h"
#include "RPGSystem.h"
#include "Serialization.h"
#include <random>
#include <string>
#include <unordered_map>
#include <vector>


// Economy-related events
class PriceChangedEvent : public EventType<PriceChangedEvent> {
public:
    std::string itemId;
    std::string marketId;
    float oldPrice;
    float newPrice;
    float priceRatio;
    bool isSignificant;
};

class TradeCompletedEvent : public EventType<TradeCompletedEvent> {
public:
    std::string itemId;
    std::string marketId;
    int quantity;
    float totalValue;
    bool playerIsBuyer;
};

// Item type classification
enum class ItemCategory {
    Food,
    Clothing,
    Weapons,
    Armor,
    Tools,
    Materials,
    Luxury,
    Magic,
    Misc
};

// Market status indicates overall economic health
enum class MarketStatus {
    Prospering,
    Stable,
    Struggling,
    Depressed
};

// Market represents a trading area (town, village, city)
class Market {
public:
    Market(const std::string& id, const std::string& name)
        : m_id(id)
        , m_name(name)
        , m_status(MarketStatus::Stable)
        , m_wealthFactor(1.0f)
        , m_specialization(ItemCategory::Misc)
        , m_supplyFactor(1.0f)
        , m_demandFactor(1.0f)
    {
    }

    // Getters
    std::string GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    MarketStatus GetStatus() const { return m_status; }
    float GetWealthFactor() const { return m_wealthFactor; }
    ItemCategory GetSpecialization() const { return m_specialization; }
    float GetSupplyFactor() const { return m_supplyFactor; }
    float GetDemandFactor() const { return m_demandFactor; }

    // Setters
    void SetStatus(MarketStatus status) { m_status = status; }
    void SetWealthFactor(float factor) { m_wealthFactor = factor; }
    void SetSpecialization(ItemCategory specialty) { m_specialization = specialty; }
    void SetSupplyFactor(float factor) { m_supplyFactor = factor; }
    void SetDemandFactor(float factor) { m_demandFactor = factor; }

    // Item price modifiers
    float GetPriceMultiplier(ItemCategory category) const;

    // Inventory management
    bool HasItem(const std::string& itemId) const;
    int GetItemStock(const std::string& itemId) const;
    void SetItemStock(const std::string& itemId, int quantity);
    void ModifyItemStock(const std::string& itemId, int change);

    // Market price functions
    float GetItemPrice(const std::string& itemId, float basePrice) const;
    void SetCustomPrice(const std::string& itemId, float price);
    bool HasCustomPrice(const std::string& itemId) const;
    float GetCustomPrice(const std::string& itemId) const;

    // Serialization
    void Serialize(BinaryWriter& writer) const;
    void Deserialize(BinaryReader& reader);
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);

private:
    std::string m_id;
    std::string m_name;
    MarketStatus m_status;
    float m_wealthFactor;
    ItemCategory m_specialization;
    float m_supplyFactor;
    float m_demandFactor;

    // Item stocks and custom prices
    std::unordered_map<std::string, int> m_itemStocks;
    std::unordered_map<std::string, float> m_customPrices;
};

// Item definition for economy system
class EconomyItem {
public:
    EconomyItem(const std::string& id, const std::string& name, float basePrice, ItemCategory category)
        : m_id(id)
        , m_name(name)
        , m_basePrice(basePrice)
        , m_category(category)
        , m_volatility(0.1f)
        , m_rarity(1.0f)
        , m_supplyScale(1.0f)
        , m_demandScale(1.0f)
    {
    }

    // Getters
    std::string GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    float GetBasePrice() const { return m_basePrice; }
    ItemCategory GetCategory() const { return m_category; }
    float GetVolatility() const { return m_volatility; }
    float GetRarity() const { return m_rarity; }
    float GetSupplyScale() const { return m_supplyScale; }
    float GetDemandScale() const { return m_demandScale; }

    // Setters
    void SetBasePrice(float price) { m_basePrice = price; }
    void SetVolatility(float volatility) { m_volatility = volatility; }
    void SetRarity(float rarity) { m_rarity = rarity; }
    void SetSupplyScale(float scale) { m_supplyScale = scale; }
    void SetDemandScale(float scale) { m_demandScale = scale; }

    // Price calculations
    float CalculatePrice(const Market& market) const;

    // Serialization
    void Serialize(BinaryWriter& writer) const;
    void Deserialize(BinaryReader& reader);
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);

private:
    std::string m_id;
    std::string m_name;
    float m_basePrice;
    ItemCategory m_category;
    float m_volatility;
    float m_rarity;
    float m_supplyScale;
    float m_demandScale;
};

class EconomySystem : public RPGSystem {
public:
    // Delete copy constructor and assignment operator
    EconomySystem(const EconomySystem&) = delete;
    EconomySystem& operator=(const EconomySystem&) = delete;

    // Meyer's Singleton - thread-safe in C++11 and beyond
    static EconomySystem* GetInstance()
    {
        static EconomySystem* instance = new EconomySystem();
        return instance;
    }

    // Cleanup method
    static void Destroy()
    {
        static EconomySystem* instance = GetInstance();
        delete instance;
        instance = nullptr;
    }

    ~EconomySystem();

    // RPGSystem interface
    void Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    std::string GetName() const override { return "EconomySystem"; }

    // Serialization
    void Serialize(BinaryWriter& writer) const override;
    void Deserialize(BinaryReader& reader) override;
    void SerializeToText(TextWriter& writer) const;
    void DeserializeFromText(TextReader& reader);

    // Market management
    bool AddMarket(const std::string& id, const std::string& name);
    Market* GetMarket(const std::string& id);
    void SetMarketStatus(const std::string& marketId, MarketStatus status);

    // Item management
    bool RegisterItem(const std::string& id, const std::string& name, float basePrice, ItemCategory category);
    EconomyItem* GetItem(const std::string& id);
    void UpdateItemPrice(const std::string& itemId, float newBasePrice);

    // Trading functions
    float GetItemPrice(const std::string& itemId, const std::string& marketId);
    bool SellItem(const std::string& itemId, const std::string& marketId, int quantity);
    bool BuyItem(const std::string& itemId, const std::string& marketId, int quantity);

    // Economy control
    void SimulateEconomyDay();
    void SetGlobalEconomyFactor(float factor);
    void TriggerEconomicEvent(const std::string& eventType);

    // Utility functions
    std::string CategoryToString(ItemCategory category) const;
    ItemCategory StringToCategory(const std::string& str) const;
    std::string MarketStatusToString(MarketStatus status) const;
    MarketStatus StringToMarketStatus(const std::string& str) const;

private:
    // Private constructor
    EconomySystem();

    // Markets and items
    std::unordered_map<std::string, std::unique_ptr<Market>> m_markets;
    std::unordered_map<std::string, std::unique_ptr<EconomyItem>> m_items;

    // Global economy factors
    float m_globalEconomyFactor = 1.0f;
    float m_inflationRate = 0.01f;
    float m_marketFluctuation = 0.05f;

    // Timing
    float m_timeSinceLastUpdate = 0.0f;
    float m_economyUpdateInterval = 24.0f; // In hours

    // Random generator
    std::mt19937 m_randomGenerator;

    // Helper methods
    void UpdateMarketConditions();
    void FluctuateItemPrices();
    float GenerateRandomFactor(float baseVolatility);
    void AdjustSupplyAndDemand(const std::string& marketId);
};
// ^ EconomySystem.h