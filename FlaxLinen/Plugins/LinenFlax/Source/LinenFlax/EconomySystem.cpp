// v EconomySystem.cpp
#include "EconomySystem.h"
#include "LinenFlax.h"
#include "TimeSystem.h"
#include "Engine/Core/Log.h"
#include <ctime>
#include <algorithm>

// Market methods
float Market::GetPriceMultiplier(ItemCategory category) const {
    // Base multiplier affected by market status
    float baseMultiplier = 1.0f;
    
    switch (m_status) {
        case MarketStatus::Prospering:
            baseMultiplier = 1.2f;
            break;
        case MarketStatus::Stable:
            baseMultiplier = 1.0f;
            break;
        case MarketStatus::Struggling:
            baseMultiplier = 0.9f;
            break;
        case MarketStatus::Depressed:
            baseMultiplier = 0.7f;
            break;
    }
    
    // Apply wealth factor
    baseMultiplier *= m_wealthFactor;
    
    // Specialization discount/premium
    if (category == m_specialization) {
        // Markets offer discounts on their specializations
        baseMultiplier *= 0.8f;
    }
    
    // Apply supply and demand factors
    baseMultiplier *= (m_demandFactor / m_supplyFactor);
    
    return baseMultiplier;
}

bool Market::HasItem(const std::string& itemId) const {
    auto it = m_itemStocks.find(itemId);
    return (it != m_itemStocks.end() && it->second > 0);
}

int Market::GetItemStock(const std::string& itemId) const {
    auto it = m_itemStocks.find(itemId);
    if (it == m_itemStocks.end()) {
        return 0;
    }
    return it->second;
}

void Market::SetItemStock(const std::string& itemId, int quantity) {
    if (quantity < 0) quantity = 0;
    m_itemStocks[itemId] = quantity;
}

void Market::ModifyItemStock(const std::string& itemId, int change) {
    int currentStock = GetItemStock(itemId);
    SetItemStock(itemId, currentStock + change);
}

float Market::GetItemPrice(const std::string& itemId, float basePrice) const {
    // Check for custom price
    if (HasCustomPrice(itemId)) {
        return GetCustomPrice(itemId);
    }
    
    // Calculate from item stock - decrease price when plenty of stock
    int stock = GetItemStock(itemId);
    float stockFactor = 1.0f;
    
    if (stock > 20) {
        stockFactor = 0.9f;
    } else if (stock < 5) {
        stockFactor = 1.1f;
    }
    
    return basePrice * stockFactor;
}

void Market::SetCustomPrice(const std::string& itemId, float price) {
    if (price < 0.0f) price = 0.0f;
    m_customPrices[itemId] = price;
}

bool Market::HasCustomPrice(const std::string& itemId) const {
    return m_customPrices.find(itemId) != m_customPrices.end();
}

float Market::GetCustomPrice(const std::string& itemId) const {
    auto it = m_customPrices.find(itemId);
    if (it == m_customPrices.end()) {
        return 0.0f;
    }
    return it->second;
}

void Market::Serialize(BinaryWriter& writer) const {
    writer.Write(m_id);
    writer.Write(m_name);
    writer.Write(static_cast<int32_t>(m_status));
    writer.Write(m_wealthFactor);
    writer.Write(static_cast<int32_t>(m_specialization));
    writer.Write(m_supplyFactor);
    writer.Write(m_demandFactor);
    
    // Write item stocks
    writer.Write(static_cast<uint32_t>(m_itemStocks.size()));
    for (const auto& pair : m_itemStocks) {
        writer.Write(pair.first);
        writer.Write(static_cast<int32_t>(pair.second));
    }
    
    // Write custom prices
    writer.Write(static_cast<uint32_t>(m_customPrices.size()));
    for (const auto& pair : m_customPrices) {
        writer.Write(pair.first);
        writer.Write(pair.second);
    }
}

void Market::Deserialize(BinaryReader& reader) {
    reader.Read(m_id);
    reader.Read(m_name);
    
    int32_t statusValue;
    reader.Read(statusValue);
    m_status = static_cast<MarketStatus>(statusValue);
    
    reader.Read(m_wealthFactor);
    
    int32_t specializationValue;
    reader.Read(specializationValue);
    m_specialization = static_cast<ItemCategory>(specializationValue);
    
    reader.Read(m_supplyFactor);
    reader.Read(m_demandFactor);
    
    // Read item stocks
    uint32_t stockCount;
    reader.Read(stockCount);
    
    m_itemStocks.clear();
    for (uint32_t i = 0; i < stockCount; ++i) {
        std::string itemId;
        int32_t quantity;
        
        reader.Read(itemId);
        reader.Read(quantity);
        
        m_itemStocks[itemId] = quantity;
    }
    
    // Read custom prices
    uint32_t priceCount;
    reader.Read(priceCount);
    
    m_customPrices.clear();
    for (uint32_t i = 0; i < priceCount; ++i) {
        std::string itemId;
        float price;
        
        reader.Read(itemId);
        reader.Read(price);
        
        m_customPrices[itemId] = price;
    }
}

void Market::SerializeToText(TextWriter& writer) const {
    writer.Write("marketId", m_id);
    writer.Write("marketName", m_name);
    writer.Write("marketStatus", static_cast<int>(m_status));
    writer.Write("marketWealthFactor", m_wealthFactor);
    writer.Write("marketSpecialization", static_cast<int>(m_specialization));
    writer.Write("marketSupplyFactor", m_supplyFactor);
    writer.Write("marketDemandFactor", m_demandFactor);
    
    // Write item stocks count
    writer.Write("marketStockCount", static_cast<int>(m_itemStocks.size()));
    
    // Write each stock entry
    int stockIndex = 0;
    for (const auto& pair : m_itemStocks) {
        std::string prefix = "marketStock" + std::to_string(stockIndex) + "_";
        writer.Write(prefix + "itemId", pair.first);
        writer.Write(prefix + "quantity", pair.second);
        stockIndex++;
    }
    
    // Write custom prices count
    writer.Write("marketPriceCount", static_cast<int>(m_customPrices.size()));
    
    // Write each price entry
    int priceIndex = 0;
    for (const auto& pair : m_customPrices) {
        std::string prefix = "marketPrice" + std::to_string(priceIndex) + "_";
        writer.Write(prefix + "itemId", pair.first);
        writer.Write(prefix + "price", pair.second);
        priceIndex++;
    }
}

void Market::DeserializeFromText(TextReader& reader) {
    reader.Read("marketId", m_id);
    reader.Read("marketName", m_name);
    
    int statusValue;
    if (reader.Read("marketStatus", statusValue))
        m_status = static_cast<MarketStatus>(statusValue);
    
    reader.Read("marketWealthFactor", m_wealthFactor);
    
    int specializationValue;
    if (reader.Read("marketSpecialization", specializationValue))
        m_specialization = static_cast<ItemCategory>(specializationValue);
    
    reader.Read("marketSupplyFactor", m_supplyFactor);
    reader.Read("marketDemandFactor", m_demandFactor);
    
    // Read item stocks
    int stockCount = 0;
    reader.Read("marketStockCount", stockCount);
    
    m_itemStocks.clear();
    for (int i = 0; i < stockCount; i++) {
        std::string prefix = "marketStock" + std::to_string(i) + "_";
        
        std::string itemId;
        int quantity = 0;
        
        reader.Read(prefix + "itemId", itemId);
        reader.Read(prefix + "quantity", quantity);
        
        m_itemStocks[itemId] = quantity;
    }
    
    // Read custom prices
    int priceCount = 0;
    reader.Read("marketPriceCount", priceCount);
    
    m_customPrices.clear();
    for (int i = 0; i < priceCount; i++) {
        std::string prefix = "marketPrice" + std::to_string(i) + "_";
        
        std::string itemId;
        float price = 0.0f;
        
        reader.Read(prefix + "itemId", itemId);
        reader.Read(prefix + "price", price);
        
        m_customPrices[itemId] = price;
    }
}

// EconomyItem methods
float EconomyItem::CalculatePrice(const Market& market) const {
    // Get base market multiplier for this category
    float marketMultiplier = market.GetPriceMultiplier(m_category);
    
    // Apply item's rarity factor
    float rarityFactor = std::pow(m_rarity, 1.5f);
    
    // Apply supply and demand scaling
    float supplyDemandRatio = market.GetDemandFactor() / market.GetSupplyFactor();
    float scaledRatio = std::pow(supplyDemandRatio, m_demandScale / m_supplyScale);
    
    // Calculate final price
    float finalPrice = m_basePrice * marketMultiplier * rarityFactor * scaledRatio;
    
    // Apply market's item-specific pricing
    finalPrice = market.GetItemPrice(m_id, finalPrice);
    
    // Ensure minimum price
    if (finalPrice < 0.01f) finalPrice = 0.01f;
    
    return finalPrice;
}

void EconomyItem::Serialize(BinaryWriter& writer) const {
    writer.Write(m_id);
    writer.Write(m_name);
    writer.Write(m_basePrice);
    writer.Write(static_cast<int32_t>(m_category));
    writer.Write(m_volatility);
    writer.Write(m_rarity);
    writer.Write(m_supplyScale);
    writer.Write(m_demandScale);
}

void EconomyItem::Deserialize(BinaryReader& reader) {
    reader.Read(m_id);
    reader.Read(m_name);
    reader.Read(m_basePrice);
    
    int32_t categoryValue;
    reader.Read(categoryValue);
    m_category = static_cast<ItemCategory>(categoryValue);
    
    reader.Read(m_volatility);
    reader.Read(m_rarity);
    reader.Read(m_supplyScale);
    reader.Read(m_demandScale);
}

void EconomyItem::SerializeToText(TextWriter& writer) const {
    writer.Write("itemId", m_id);
    writer.Write("itemName", m_name);
    writer.Write("itemBasePrice", m_basePrice);
    writer.Write("itemCategory", static_cast<int>(m_category));
    writer.Write("itemVolatility", m_volatility);
    writer.Write("itemRarity", m_rarity);
    writer.Write("itemSupplyScale", m_supplyScale);
    writer.Write("itemDemandScale", m_demandScale);
}

void EconomyItem::DeserializeFromText(TextReader& reader) {
    reader.Read("itemId", m_id);
    reader.Read("itemName", m_name);
    reader.Read("itemBasePrice", m_basePrice);
    
    int categoryValue;
    if (reader.Read("itemCategory", categoryValue))
        m_category = static_cast<ItemCategory>(categoryValue);
    
    reader.Read("itemVolatility", m_volatility);
    reader.Read("itemRarity", m_rarity);
    reader.Read("itemSupplyScale", m_supplyScale);
    reader.Read("itemDemandScale", m_demandScale);
}

// EconomySystem methods
EconomySystem::EconomySystem() {
    // Define system dependencies
    m_dependencies.insert("TimeSystem");
    
    // Initialize random generator with a time-based seed
    m_randomGenerator.seed(static_cast<unsigned int>(std::time(nullptr)));
}

EconomySystem::~EconomySystem() {
    Destroy();
}

void EconomySystem::Initialize() {
    // Subscribe to day changed events
    if (m_plugin) {
        m_plugin->GetEventSystem().Subscribe<DayChangedEvent>(
            [this](const DayChangedEvent& event) {
                // Simulate economy changes on day change
                SimulateEconomyDay();
            });
    }
    
    LOG(Info, "Economy System Initialized.");
}

void EconomySystem::Shutdown() {
    m_markets.clear();
    m_items.clear();
    LOG(Info, "Economy System Shutdown.");
}

void EconomySystem::Update(float deltaTime) {
    // Check if we should update the economy based on game time
    auto* timeSystem = m_plugin->GetSystem<TimeSystem>();
    if (timeSystem) {
        // Accumulate time since last update
        m_timeSinceLastUpdate += deltaTime * timeSystem->GetTimeScale();
        
        // Update economy after interval passes
        if (m_timeSinceLastUpdate >= m_economyUpdateInterval) {
            m_timeSinceLastUpdate = 0.0f;
            UpdateMarketConditions();
            FluctuateItemPrices();
        }
    }
}

bool EconomySystem::AddMarket(const std::string& id, const std::string& name) {
    if (m_markets.find(id) != m_markets.end()) {
        LOG(Warning, "Market with ID {0} already exists", String(id.c_str()));
        return false;
    }
    
    auto market = std::make_unique<Market>(id, name);
    m_markets[id] = std::move(market);
    
    LOG(Info, "Added market: {0} ({1})", String(name.c_str()), String(id.c_str()));
    return true;
}

Market* EconomySystem::GetMarket(const std::string& id) {
    auto it = m_markets.find(id);
    if (it == m_markets.end()) {
        return nullptr;
    }
    return it->second.get();
}

void EconomySystem::SetMarketStatus(const std::string& marketId, MarketStatus status) {
    Market* market = GetMarket(marketId);
    if (market) {
        market->SetStatus(status);
        LOG(Info, "Market {0} status set to {1}", 
            String(marketId.c_str()), 
            String(MarketStatusToString(status).c_str()));
    } else {
        LOG(Warning, "Cannot set status for nonexistent market: {0}", String(marketId.c_str()));
    }
}

bool EconomySystem::RegisterItem(const std::string& id, const std::string& name, float basePrice, ItemCategory category) {
    if (m_items.find(id) != m_items.end()) {
        LOG(Warning, "Item with ID {0} already registered", String(id.c_str()));
        return false;
    }
    
    auto item = std::make_unique<EconomyItem>(id, name, basePrice, category);
    m_items[id] = std::move(item);
    
    LOG(Info, "Registered item: {0} ({1}) - Base price: {2}", 
        String(name.c_str()), String(id.c_str()), basePrice);
    return true;
}

EconomyItem* EconomySystem::GetItem(const std::string& id) {
    auto it = m_items.find(id);
    if (it == m_items.end()) {
        return nullptr;
    }
    return it->second.get();
}

void EconomySystem::UpdateItemPrice(const std::string& itemId, float newBasePrice) {
    EconomyItem* item = GetItem(itemId);
    if (item) {
        float oldPrice = item->GetBasePrice();
        item->SetBasePrice(newBasePrice);
        
        LOG(Info, "Updated item {0} base price: {1} -> {2}", 
            String(itemId.c_str()), oldPrice, newBasePrice);
    } else {
        LOG(Warning, "Cannot update price for nonexistent item: {0}", String(itemId.c_str()));
    }
}

float EconomySystem::GetItemPrice(const std::string& itemId, const std::string& marketId) {
    EconomyItem* item = GetItem(itemId);
    Market* market = GetMarket(marketId);
    
    if (!item || !market) {
        LOG(Warning, "Cannot get price - item or market not found");
        return 0.0f;
    }
    
    // Calculate price based on item and market factors
    float price = item->CalculatePrice(*market);
    
    // Apply global economy factor
    price *= m_globalEconomyFactor;
    
    return price;
}

bool EconomySystem::SellItem(const std::string& itemId, const std::string& marketId, int quantity) {
    if (quantity <= 0) {
        LOG(Warning, "Cannot sell non-positive quantity");
        return false;
    }
    
    Market* market = GetMarket(marketId);
    EconomyItem* item = GetItem(itemId);
    
    if (!market || !item) {
        LOG(Warning, "Cannot sell item - market or item not found");
        return false;
    }
    
    // Calculate price
    float unitPrice = GetItemPrice(itemId, marketId);
    float totalValue = unitPrice * quantity;
    
    // Update market inventory
    market->ModifyItemStock(itemId, quantity);
    
    // Fire event
    TradeCompletedEvent event;
    event.itemId = itemId;
    event.marketId = marketId;
    event.quantity = quantity;
    event.totalValue = totalValue;
    event.playerIsBuyer = false;
    
    m_plugin->GetEventSystem().Publish(event);
    
    LOG(Info, "Player sold {0}x {1} to {2} for {3} each (total: {4})",
        quantity, String(itemId.c_str()), String(marketId.c_str()),
        unitPrice, totalValue);
    
    // Increase supply, potentially lowering future price
    // This helps simulate supply and demand
    AdjustSupplyAndDemand(marketId);
    
    return true;
}

bool EconomySystem::BuyItem(const std::string& itemId, const std::string& marketId, int quantity) {
    if (quantity <= 0) {
        LOG(Warning, "Cannot buy non-positive quantity");
        return false;
    }
    
    Market* market = GetMarket(marketId);
    EconomyItem* item = GetItem(itemId);
    
    if (!market || !item) {
        LOG(Warning, "Cannot buy item - market or item not found");
        return false;
    }
    
    // Check if market has enough stock
    int currentStock = market->GetItemStock(itemId);
    if (currentStock < quantity) {
        LOG(Warning, "Market does not have enough stock. Requested: {0}, Available: {1}",
            quantity, currentStock);
        return false;
    }
    
    // Calculate price
    float unitPrice = GetItemPrice(itemId, marketId);
    float totalValue = unitPrice * quantity;
    
    // Update market inventory
    market->ModifyItemStock(itemId, -quantity);
    
    // Fire event
    TradeCompletedEvent event;
    event.itemId = itemId;
    event.marketId = marketId;
    event.quantity = quantity;
    event.totalValue = totalValue;
    event.playerIsBuyer = true;
    
    m_plugin->GetEventSystem().Publish(event);
    
    LOG(Info, "Player bought {0}x {1} from {2} for {3} each (total: {4})",
        quantity, String(itemId.c_str()), String(marketId.c_str()),
        unitPrice, totalValue);
    
    // Reduce supply, potentially increasing future price
    AdjustSupplyAndDemand(marketId);
    
    return true;
}

void EconomySystem::SimulateEconomyDay() {
    LOG(Info, "Simulating economy day");
    
    // Update market conditions
    UpdateMarketConditions();
    
    // Fluctuate prices
    FluctuateItemPrices();
    
    // Apply inflation
    m_globalEconomyFactor *= (1.0f + m_inflationRate);
    
    // Log economy status
    LOG(Info, "Economy day simulation complete. Global factor: {0:0.3f}", m_globalEconomyFactor);
}

void EconomySystem::SetGlobalEconomyFactor(float factor) {
    if (factor <= 0.0f) {
        LOG(Warning, "Cannot set non-positive economy factor. Using 0.1");
        factor = 0.1f;
    }
    
    m_globalEconomyFactor = factor;
    LOG(Info, "Global economy factor set to {0:0.3f}", factor);
}

void EconomySystem::TriggerEconomicEvent(const std::string& eventType) {
    LOG(Info, "Economic event triggered: {0}", String(eventType.c_str()));
    
    if (eventType == "boom") {
        // Economic boom - prosperity for all markets
        for (auto& pair : m_markets) {
            pair.second->SetStatus(MarketStatus::Prospering);
            pair.second->SetWealthFactor(pair.second->GetWealthFactor() * 1.2f);
        }
        
        // Reduce prices of luxury goods to simulate availability
        for (auto& pair : m_items) {
            if (pair.second->GetCategory() == ItemCategory::Luxury) {
                pair.second->SetBasePrice(pair.second->GetBasePrice() * 0.9f);
            }
        }
    }
    else if (eventType == "recession") {
        // Economic recession - all markets struggle
        for (auto& pair : m_markets) {
            pair.second->SetStatus(MarketStatus::Struggling);
            pair.second->SetWealthFactor(pair.second->GetWealthFactor() * 0.8f);
        }
        
        // Increase prices of essentials to simulate scarcity
        for (auto& pair : m_items) {
            if (pair.second->GetCategory() == ItemCategory::Food) {
                pair.second->SetBasePrice(pair.second->GetBasePrice() * 1.3f);
            }
        }
    }
    else if (eventType == "trade_disruption") {
        // Trade routes disrupted - reduces supplies
        for (auto& pair : m_markets) {
            pair.second->SetSupplyFactor(pair.second->GetSupplyFactor() * 0.7f);
        }
    }
    else if (eventType == "harvest") {
        // Bountiful harvest - increases food supply
        for (auto& pair : m_items) {
            if (pair.second->GetCategory() == ItemCategory::Food) {
                pair.second->SetBasePrice(pair.second->GetBasePrice() * 0.7f);
            }
        }
        
        for (auto& pair : m_markets) {
            // Increase stock of food items
            for (auto& itemPair : m_items) {
                if (itemPair.second->GetCategory() == ItemCategory::Food) {
                    int currentStock = pair.second->GetItemStock(itemPair.first);
                    pair.second->SetItemStock(itemPair.first, currentStock + 10);
                }
            }
        }
    }
}

void EconomySystem::UpdateMarketConditions() {
    LOG(Info, "Updating market conditions");
    
    for (auto& pair : m_markets) {
        Market* market = pair.second.get();
        
        // Random chance to change market status
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float changeChance = dist(m_randomGenerator);
        
        if (changeChance < 0.1f) {  // 10% chance to change status
            // Determine new status
            int currentStatus = static_cast<int>(market->GetStatus());
            int statusDelta = (dist(m_randomGenerator) > 0.5f) ? 1 : -1;
            int newStatus = std::min(std::max(currentStatus + statusDelta, 0), 3);
            
            market->SetStatus(static_cast<MarketStatus>(newStatus));
            
            LOG(Info, "Market {0} status changed to {1}", 
                String(market->GetName().c_str()), 
                String(MarketStatusToString(market->GetStatus()).c_str()));
        }
        
        // Update wealth factor with some volatility
        float wealthFactor = market->GetWealthFactor();
        float wealthDelta = (dist(m_randomGenerator) - 0.5f) * 0.1f;  // +/- 5%
        market->SetWealthFactor(std::max(wealthFactor + wealthDelta, 0.5f));
        
        // Natural rebalancing of supply/demand
        float supplyFactor = market->GetSupplyFactor();
        float demandFactor = market->GetDemandFactor();
        
        // Supply and demand tend to equalize over time (market forces)
        float equilibriumRate = 0.05f;
        if (supplyFactor > demandFactor) {
            supplyFactor -= equilibriumRate;
            demandFactor += equilibriumRate * 0.5f;
        } else {
            supplyFactor += equilibriumRate * 0.5f;
            demandFactor -= equilibriumRate;
        }
        
        // Ensure minimum values
        supplyFactor = std::max(supplyFactor, 0.5f);
        demandFactor = std::max(demandFactor, 0.5f);
        
        market->SetSupplyFactor(supplyFactor);
        market->SetDemandFactor(demandFactor);
        
        // Replenish some stock naturally
        for (auto& itemPair : m_items) {
            std::string itemId = itemPair.first;
            int currentStock = market->GetItemStock(itemId);
            
            // Only replenish if below threshold
            if (currentStock < 5) {
                // More likely to replenish if it's the market's specialization
                int replenishAmount = 1;
                if (itemPair.second->GetCategory() == market->GetSpecialization()) {
                    replenishAmount = 3;
                }
                
                market->SetItemStock(itemId, currentStock + replenishAmount);
            }
        }
    }
}

void EconomySystem::FluctuateItemPrices() {
    LOG(Info, "Fluctuating item prices");
    
    for (auto& pair : m_items) {
        EconomyItem* item = pair.second.get();
        
        // Calculate price fluctuation based on item volatility
        float basePrice = item->GetBasePrice();
        float volatility = item->GetVolatility();
        
        float fluctuation = GenerateRandomFactor(volatility);
        float newPrice = basePrice * fluctuation;
        
        // Ensure minimum price
        newPrice = std::max(newPrice, 0.01f);
        
        // Update if price change is significant
        if (std::abs(newPrice - basePrice) / basePrice > 0.02f) {  // 2% threshold
            item->SetBasePrice(newPrice);
            
            // Log significant price changes
            float priceRatio = newPrice / basePrice;
            bool isSignificant = std::abs(priceRatio - 1.0f) > 0.1f;  // 10% change
            
            if (isSignificant) {
                LOG(Info, "Item {0} price changed significantly: {1:0.2f} -> {2:0.2f} ({3:0.1f}%)",
                    String(item->GetName().c_str()),
                    basePrice, newPrice,
                    (priceRatio - 1.0f) * 100.0f);
                    
                // For each market, fire a price changed event
                for (auto& marketPair : m_markets) {
                    PriceChangedEvent event;
                    event.itemId = item->GetId();
                    event.marketId = marketPair.first;
                    event.oldPrice = basePrice;
                    event.newPrice = newPrice;
                    event.priceRatio = priceRatio;
                    event.isSignificant = isSignificant;
                    
                    m_plugin->GetEventSystem().Publish(event);
                }
            }
        }
    }
}

float EconomySystem::GenerateRandomFactor(float baseVolatility) {
    // Generate random factor with normal distribution
    std::normal_distribution<float> distribution(1.0f, baseVolatility);
    float factor = distribution(m_randomGenerator);
    
    // Clamp to reasonable range
    return std::max(std::min(factor, 1.0f + baseVolatility * 3.0f), 1.0f - baseVolatility * 3.0f);
}

void EconomySystem::AdjustSupplyAndDemand(const std::string& marketId) {
    Market* market = GetMarket(marketId);
    if (!market) return;
    
    // Adjust supply and demand based on trading activity
    float supplyFactor = market->GetSupplyFactor();
    float demandFactor = market->GetDemandFactor();
    
    // Small adjustments to simulate market response
    float adjustmentRate = 0.02f;  // 2% adjustment
    
    supplyFactor += adjustmentRate;
    demandFactor -= adjustmentRate * 0.5f;
    
    // Ensure minimum values
    supplyFactor = std::max(supplyFactor, 0.5f);
    demandFactor = std::max(demandFactor, 0.5f);
    
    market->SetSupplyFactor(supplyFactor);
    market->SetDemandFactor(demandFactor);
}

std::string EconomySystem::CategoryToString(ItemCategory category) const {
    switch (category) {
        case ItemCategory::Food: return "Food";
        case ItemCategory::Clothing: return "Clothing";
        case ItemCategory::Weapons: return "Weapons";
        case ItemCategory::Armor: return "Armor";
        case ItemCategory::Tools: return "Tools";
        case ItemCategory::Materials: return "Materials";
        case ItemCategory::Luxury: return "Luxury";
        case ItemCategory::Magic: return "Magic";
        case ItemCategory::Misc: return "Misc";
        default: return "Unknown";
    }
}

ItemCategory EconomySystem::StringToCategory(const std::string& str) const {
    if (str == "Food") return ItemCategory::Food;
    if (str == "Clothing") return ItemCategory::Clothing;
    if (str == "Weapons") return ItemCategory::Weapons;
    if (str == "Armor") return ItemCategory::Armor;
    if (str == "Tools") return ItemCategory::Tools;
    if (str == "Materials") return ItemCategory::Materials;
    if (str == "Luxury") return ItemCategory::Luxury;
    if (str == "Magic") return ItemCategory::Magic;
    if (str == "Misc") return ItemCategory::Misc;
    
    return ItemCategory::Misc;  // Default
}

std::string EconomySystem::MarketStatusToString(MarketStatus status) const {
    switch (status) {
        case MarketStatus::Prospering: return "Prospering";
        case MarketStatus::Stable: return "Stable";
        case MarketStatus::Struggling: return "Struggling";
        case MarketStatus::Depressed: return "Depressed";
        default: return "Unknown";
    }
}

MarketStatus EconomySystem::StringToMarketStatus(const std::string& str) const {
    if (str == "Prospering") return MarketStatus::Prospering;
    if (str == "Stable") return MarketStatus::Stable;
    if (str == "Struggling") return MarketStatus::Struggling;
    if (str == "Depressed") return MarketStatus::Depressed;
    
    return MarketStatus::Stable;  // Default
}

void EconomySystem::Serialize(BinaryWriter& writer) const {
    // Write global economy state
    writer.Write(m_globalEconomyFactor);
    writer.Write(m_inflationRate);
    writer.Write(m_marketFluctuation);
    writer.Write(m_timeSinceLastUpdate);
    writer.Write(m_economyUpdateInterval);
    
    // Write markets
    writer.Write(static_cast<uint32_t>(m_markets.size()));
    for (const auto& pair : m_markets) {
        writer.Write(pair.first);
        pair.second->Serialize(writer);
    }
    
    // Write items
    writer.Write(static_cast<uint32_t>(m_items.size()));
    for (const auto& pair : m_items) {
        writer.Write(pair.first);
        pair.second->Serialize(writer);
    }
    
    LOG(Info, "EconomySystem serialized");
}

void EconomySystem::Deserialize(BinaryReader& reader) {
    // Read global economy state
    reader.Read(m_globalEconomyFactor);
    reader.Read(m_inflationRate);
    reader.Read(m_marketFluctuation);
    reader.Read(m_timeSinceLastUpdate);
    reader.Read(m_economyUpdateInterval);
    
    // Read markets
    uint32_t marketCount;
    reader.Read(marketCount);
    
    m_markets.clear();
    for (uint32_t i = 0; i < marketCount; ++i) {
        std::string marketId;
        reader.Read(marketId);
        
        auto market = std::make_unique<Market>("", "");
        market->Deserialize(reader);
        m_markets[marketId] = std::move(market);
    }
    
    // Read items
    uint32_t itemCount;
    reader.Read(itemCount);
    
    m_items.clear();
    for (uint32_t i = 0; i < itemCount; ++i) {
        std::string itemId;
        reader.Read(itemId);
        
        auto item = std::make_unique<EconomyItem>("", "", 0.0f, ItemCategory::Misc);
        item->Deserialize(reader);
        m_items[itemId] = std::move(item);
    }
    
    LOG(Info, "EconomySystem deserialized");
}

void EconomySystem::SerializeToText(TextWriter& writer) const {
    // Write global economy state
    writer.Write("economyGlobalFactor", m_globalEconomyFactor);
    writer.Write("economyInflationRate", m_inflationRate);
    writer.Write("economyMarketFluctuation", m_marketFluctuation);
    writer.Write("economyTimeSinceUpdate", m_timeSinceLastUpdate);
    writer.Write("economyUpdateInterval", m_economyUpdateInterval);
    
    // Write market count
    writer.Write("marketCount", static_cast<int>(m_markets.size()));
    
    // Write each market
    int marketIndex = 0;
    for (const auto& pair : m_markets) {
        std::string prefix = "market" + std::to_string(marketIndex) + "_";
        writer.Write(prefix + "id", pair.first);
        
        // Let the market serialize itself with its own prefix
        pair.second->SerializeToText(writer);
        
        marketIndex++;
    }
    
    // Write item count
    writer.Write("itemCount", static_cast<int>(m_items.size()));
    
    // Write each item
    int itemIndex = 0;
    for (const auto& pair : m_items) {
        std::string prefix = "item" + std::to_string(itemIndex) + "_";
        writer.Write(prefix + "id", pair.first);
        
        // Let the item serialize itself with its own prefix
        pair.second->SerializeToText(writer);
        
        itemIndex++;
    }
    
    LOG(Info, "EconomySystem serialized to text");
}

void EconomySystem::DeserializeFromText(TextReader& reader) {
    // Read global economy state
    reader.Read("economyGlobalFactor", m_globalEconomyFactor);
    reader.Read("economyInflationRate", m_inflationRate);
    reader.Read("economyMarketFluctuation", m_marketFluctuation);
    reader.Read("economyTimeSinceUpdate", m_timeSinceLastUpdate);
    reader.Read("economyUpdateInterval", m_economyUpdateInterval);
    
    // Read markets
    int marketCount = 0;
    reader.Read("marketCount", marketCount);
    
    m_markets.clear();
    for (int i = 0; i < marketCount; i++) {
        std::string prefix = "market" + std::to_string(i) + "_";
        
        std::string marketId;
        reader.Read(prefix + "id", marketId);
        
        auto market = std::make_unique<Market>("", "");
        market->DeserializeFromText(reader);
        m_markets[marketId] = std::move(market);
    }
    
    // Read items
    int itemCount = 0;
    reader.Read("itemCount", itemCount);
    
    m_items.clear();
    for (int i = 0; i < itemCount; i++) {
        std::string prefix = "item" + std::to_string(i) + "_";
        
        std::string itemId;
        reader.Read(prefix + "id", itemId);
        
        auto item = std::make_unique<EconomyItem>("", "", 0.0f, ItemCategory::Misc);
        item->DeserializeFromText(reader);
        m_items[itemId] = std::move(item);
    }
    
    LOG(Info, "EconomySystem deserialized from text");
}
// ^ EconomySystem.cpp