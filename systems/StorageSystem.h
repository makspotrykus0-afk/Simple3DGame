#pragma once

#include "../core/GameEngine.h"
#include "ResourceSystem.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <atomic>
#include <any>

/**
 * @brief Typy magazynów
 */
enum class StorageType : uint32_t {
    INVENTORY = 0,       // Ekwipunek gracza
    WAREHOUSE = 1,       // Magazyn budynku
    CHEST = 2,          // Skrzynia
    BANK = 3,           // Bank/grupa
    VAULT = 4,          // Sejf
    CUSTOM_START = 1000
};

/**
 * @brief Stan magazynu
 */
enum class StorageState : uint8_t {
    EMPTY = 0,          // Pusty
    PARTIAL = 1,        // Częściowo zapełniony
    FULL = 2,           // Całkowicie zapełniony
    OVERLOADED = 3      // Przeciążony
};

/**
 * @brief Struktura opisująca pojedynczy slot w magazynie
 */
struct StorageSlot {
    bool isOccupied;
    Resources::ResourceType resourceType;
    int32_t amount;
    
    StorageSlot() : isOccupied(false), resourceType(Resources::ResourceType::Wood), amount(0) {}
    
    bool canAddResource(Resources::ResourceType type, int32_t amount, const Resources::Resource* resourceInfo) const;
    int32_t getMaxCapacity(const Resources::Resource* resourceInfo) const;
};

/**
 * @brief Zdarzenie zmiany stanu magazynu
 */
struct StorageStateChangedEvent {
    std::string storageId;
    StorageType storageType;
    StorageState oldState;
    StorageState newState;
    float capacityUsage;  // Użycie pojemności (0.0 - 1.0)
    std::string playerId;
};

/**
 * @brief Zdarzenie dodania przedmiotu do magazynu
 */
struct ItemAddedToStorageEvent {
    std::string storageId;
    Resources::ResourceType resourceType;
    int32_t amount;
    int32_t slotIndex;
    std::string playerId;
};

/**
 * @brief Zdarzenie usunięcia przedmiotu z magazynu
 */
struct ItemRemovedFromStorageEvent {
    std::string storageId;
    Resources::ResourceType resourceType;
    int32_t amount;
    int32_t slotIndex;
    std::string playerId;
};

/**
 * @brief System magazynowy implementujący object pooling i lazy loading
 */
class StorageSystem : public IGameSystem {
public:
    /**
     * @brief Konfiguracja typu magazynu
     */
    struct StorageConfig {
        StorageType type;
        std::string name;
        std::string description;
        uint32_t maxSlots;              // Maksymalna liczba slotów
        float maxWeight;               // Maksymalna waga
        float maxVolume;               // Maksymalna objętość
        bool isStackable;              // Czy przedmioty mogą być układane w stosy
        uint32_t stackLimitPerSlot;    // Limit na slot
        bool requiresAuthentication;   // Czy wymaga uwierzytelnienia
        float accessTime;              // Czas dostępu (sekundy)
        bool isShared;                 // Czy jest współdzielony między graczami
        
        StorageConfig(StorageType t, const std::string& n, const std::string& desc = "")
            : type(t), name(n), description(desc), maxSlots(20), maxWeight(100.0f),
              maxVolume(100.0f), isStackable(true), stackLimitPerSlot(99),
              requiresAuthentication(false), accessTime(0.1f), isShared(false) {}

        StorageConfig()
            : type(StorageType::INVENTORY), name("Default"), description(""), maxSlots(20), maxWeight(100.0f),
              maxVolume(100.0f), isStackable(true), stackLimitPerSlot(99),
              requiresAuthentication(false), accessTime(0.1f), isShared(false) {}
    };

    /**
     * @brief Instancja magazynu
     */
    struct StorageInstance {
        std::string id;
        StorageType type;
        std::string ownerId;           // ID właściciela
        StorageState state;
        std::vector<StorageSlot> slots;
        float currentWeight;
        float currentVolume;
        std::chrono::high_resolution_clock::time_point lastAccess;
        std::unordered_map<Resources::ResourceType, int32_t> totalAmounts; // Cache dla szybkiego dostępu
        
        StorageInstance() : state(StorageState::EMPTY), currentWeight(0.0f), currentVolume(0.0f) {}
        
        /**
         * @brief Pobiera całkowitą pojemność
         * @return Pojemność w procentach
         */
        float getCapacityUsage() const;
        
        /**
         * @brief Sprawdza czy może pomieścić zasób
         * @param type Typ zasobu
         * @param amount Ilość
         * @param resourceInfo Informacje o zasobie
         * @return true jeśli może pomieścić
         */
        bool canAddResource(Resources::ResourceType type, int32_t amount, const Resources::Resource* resourceInfo) const;
        
        /**
         * @brief Pobiera liczbę wolnych slotów
         * @return Liczba wolnych slotów
         */
        uint32_t getFreeSlots() const;
        
        /**
         * @brief Pobiera łączną pojemność dla zasobu
         * @param type Typ zasobu
         * @param resourceInfo Informacje o zasobie
         * @return Maksymalna ilość tego zasobu
         */
        int32_t getMaxCapacityForResource(Resources::ResourceType type, const Resources::Resource* resourceInfo) const;
    };

    /**
     * @brief Factory dla magazynów
     */
    class StorageFactory {
    public:
        /**
         * @brief Tworzy podstawowe konfiguracje magazynów
         * @return Mapa konfiguracji magazynów
         */
        static std::unordered_map<StorageType, StorageConfig> createBaseStorageTypes() {
            std::unordered_map<StorageType, StorageConfig> storageTypes;
            
            // Ekwipunek gracza
            {
                StorageConfig inventory(StorageType::INVENTORY, "Ekwipunek", "Podstawowy ekwipunek gracza");
                inventory.maxSlots = 30;
                inventory.maxWeight = 50.0f;
                inventory.maxVolume = 50.0f;
                inventory.stackLimitPerSlot = 99;
                inventory.accessTime = 0.0f;
                storageTypes[StorageType::INVENTORY] = std::move(inventory);
            }
            
            // Magazyn
            {
                StorageConfig warehouse(StorageType::WAREHOUSE, "Magazyn", "Duży magazyn w budynkach");
                warehouse.maxSlots = 100;
                warehouse.maxWeight = 1000.0f;
                warehouse.maxVolume = 1000.0f;
                warehouse.stackLimitPerSlot = 999;
                warehouse.accessTime = 0.2f;
                storageTypes[StorageType::WAREHOUSE] = std::move(warehouse);
            }
            
            // Skrzynia
            {
                StorageConfig chest(StorageType::CHEST, "Skrzynia", "Mała przenośna skrzynia");
                chest.maxSlots = 20;
                chest.maxWeight = 200.0f;
                chest.maxVolume = 200.0f;
                chest.stackLimitPerSlot = 50;
                chest.accessTime = 0.5f;
                storageTypes[StorageType::CHEST] = std::move(chest);
            }
            
            // Bank
            {
                StorageConfig bank(StorageType::BANK, "Bank", "Bezpieczny bank dla cennych przedmiotów");
                bank.maxSlots = 200;
                bank.maxWeight = 200.0f;
                bank.maxVolume = 200.0f;
                bank.stackLimitPerSlot = 999;
                bank.accessTime = 1.0f;
                bank.requiresAuthentication = true;
                bank.isShared = false;
                storageTypes[StorageType::BANK] = std::move(bank);
            }
            
            return storageTypes;
        }
        
        /**
         * @brief Tworzy instancję magazynu
         * @param type Typ magazynu
         * @param ownerId ID właściciela
         * @return Nowa instancja magazynu
         */
        static StorageInstance createStorageInstance(StorageType type, const std::string& ownerId) {
            StorageInstance instance;
            instance.id = ""; // Will be set by system
            instance.type = type;
            instance.ownerId = ownerId;
            instance.state = StorageState::EMPTY;
            return instance;
        }
    };

    /**
     * @brief Object Pool dla slotów magazynowych
     */
    class StorageSlotPool {
    public:
        static std::unique_ptr<StorageSlot> acquireSlot();
        static void releaseSlot(std::unique_ptr<StorageSlot> slot);
        static void clearPool();
        static size_t getPoolSize();
        
    private:
        static std::vector<std::unique_ptr<StorageSlot>>& getPool() {
            static std::vector<std::unique_ptr<StorageSlot>> pool;
            return pool;
        }
    };

public:
    StorageSystem();
    virtual ~StorageSystem() override;

    // IGameSystem interface
    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;
    std::string getName() const override { return m_name; }
    int getPriority() const override { return 8; }

    bool registerStorageType(const StorageConfig& config);
    const StorageConfig* getStorageConfig(StorageType type) const;
    std::string createStorage(StorageType type, const std::string& ownerId);
    bool deleteStorage(const std::string& storageId);

    int32_t addResourceToStorage(const std::string& storageId, const std::string& playerId,
                                Resources::ResourceType resourceType, int32_t amount);
    int32_t removeResourceFromStorage(const std::string& storageId, const std::string& playerId,
                                     Resources::ResourceType resourceType, int32_t amount);

    int32_t getResourceAmount(const std::string& storageId, Resources::ResourceType resourceType) const;
    std::unordered_map<Resources::ResourceType, int32_t> getAllResources(const std::string& storageId) const;
    bool canAddResource(const std::string& storageId, Resources::ResourceType resourceType, int32_t amount) const;
    
    // Missing method implementation
    void addItemToStorage(const std::string& storageId, const Item& item, int amount);

    StorageInstance* getStorage(const std::string& storageId) const;
    std::vector<std::string> getPlayerStorages(const std::string& playerId) const;
    bool optimizeStorageLayout(const std::string& storageId);
    void lazyLoadStorage(const std::string& storageId);

    struct StorageSystemStats {
        size_t totalStorageTypes;
        size_t totalStorages;
        size_t totalSlots;
        size_t usedSlots;
        float averageCapacityUsage;
        size_t cacheHits;
        size_t cacheMisses;
    };

    StorageSystemStats getStats() const;

private:
    void initializeBaseStorageTypes();
    void updateStorageState(StorageInstance& storage);
    int32_t findSlotForResource(StorageInstance& storage, Resources::ResourceType resourceType, const Resources::Resource* resourceInfo) const;
    int32_t addResourceToSlot(StorageInstance& storage, int32_t slotIndex,
                             Resources::ResourceType resourceType, int32_t amount, const Resources::Resource* resourceInfo);
    int32_t removeResourceFromSlot(StorageInstance& storage, int32_t slotIndex, int32_t amount);
    void notifyStorageEvent(const std::any& event);

    struct StorageCache {
        std::unordered_map<std::string, std::vector<Resources::ResourceType>> storageResourceTypes;
        std::chrono::high_resolution_clock::time_point lastCleanup;
        
        StorageCache() : lastCleanup(std::chrono::high_resolution_clock::now()) {}
        
        void addToCache(const std::string& storageId, const std::vector<Resources::ResourceType>& types) {
            storageResourceTypes[storageId] = types;
        }
        
        bool getFromCache(const std::string& storageId, std::vector<Resources::ResourceType>& types) const {
            auto it = storageResourceTypes.find(storageId);
            if (it != storageResourceTypes.end()) {
                types = it->second;
                return true;
            }
            return false;
        }
        
        void cleanup() {
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCleanup).count() > 300) {
                storageResourceTypes.clear();
                lastCleanup = now;
            }
        }
    };

private:
    std::string m_name;
    std::unordered_map<StorageType, StorageConfig> m_storageTypes;
    std::unordered_map<std::string, StorageInstance> m_storages;
    std::unordered_map<std::string, std::vector<std::string>> m_playerStorages;
    StorageCache m_cache;
    
    mutable std::atomic<size_t> m_cacheHits;
    mutable std::atomic<size_t> m_cacheMisses;
    std::atomic<uint64_t> m_nextStorageId;
};