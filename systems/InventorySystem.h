#pragma once

#include "../core/IGameSystem.h"
#include "../components/InventoryComponent.h"
#include <vector>
#include <memory>
#include <typeindex>

// Forward declarations
class GameEntity;

/**
 * @brief System zarządzania ekwipunkiem
 * Zarządza operacjami na ekwipunku dla wszystkich encji
 */
class InventorySystem : public IGameSystem {
public:
    /**
     * @brief Konstruktor
     */
    InventorySystem();

    /**
     * @brief Destruktor
     */
    virtual ~InventorySystem() = default;

    // IGameSystem interface
    void update(float deltaTime) override;
    void render() override;
    void initialize() override;
    void shutdown() override;
    
    std::string getName() const override;
    int getPriority() const override;

    /**
     * @brief Rejestruje komponent ekwipunku
     * @param inventory Komponent do zarejestrowania
     */
    void registerInventory(InventoryComponent* inventory);

    /**
     * @brief Wyrejestrowuje komponent ekwipunku
     * @param inventory Komponent do wyrejestrowania
     */
    void unregisterInventory(InventoryComponent* inventory);

    /**
     * @brief Transferuje przedmiot między ekwipunkami
     * @param from Ekwipunek źródłowy
     * @param to Ekwipunek docelowy
     * @param slotIndex Indeks slotu w źródłowym ekwipunku
     * @param quantity Ilość do przeniesienia
     * @return true jeśli transfer się powiódł
     */
    bool transferItem(InventoryComponent* from, InventoryComponent* to, 
                     int slotIndex, int quantity = -1);

    /**
     * @brief Przenosi przedmiot między slotami w tym samym ekwipunku
     * @param inventory Ekwipunek
     * @param fromSlot Slot źródłowy
     * @param toSlot Slot docelowy
     * @return true jeśli przeniesienie się powiodło
     */
    bool moveItem(InventoryComponent* inventory, int fromSlot, int toSlot);

    /**
     * @brief Dzieli stack przedmiotów
     * @param inventory Ekwipunek
     * @param slotIndex Indeks slotu
     * @param quantity Ilość do oddzielenia
     * @return true jeśli podział się powiódł
     */
    bool splitStack(InventoryComponent* inventory, int slotIndex, int quantity);

    /**
     * @brief Łączy stacki przedmiotów
     * @param inventory Ekwipunek
     * @param slot1 Pierwszy slot
     * @param slot2 Drugi slot
     * @return true jeśli połączenie się powiodło
     */
    bool mergeStacks(InventoryComponent* inventory, int slot1, int slot2);

    /**
     * @brief Automatycznie zbiera przedmiot (autoloot)
     * @param player Encja gracza
     * @param item Przedmiot do zebrania
     * @param quantity Ilość
     * @return true jeśli zebrano pomyślnie
     */
    bool autoLoot(GameEntity* player, std::unique_ptr<Item> item, int quantity = 1);

    /**
     * @brief Sprawdza czy gracz może zebrać przedmiot
     * @param player Encja gracza
     * @param item Przedmiot do sprawdzenia
     * @param quantity Ilość
     * @return true jeśli może zebrać
     */
    bool canAutoLoot(GameEntity* player, Item* item, int quantity = 1) const;

    /**
     * @brief Sortuje wszystkie zarejestrowane ekwipunki
     */
    void sortAllInventories();

    /**
     * @brief Kompresuje wszystkie zarejestrowane ekwipunki
     */
    void compressAllInventories();

    /**
     * @brief Ustawia czy autoloot jest włączony
     * @param enabled Flaga włączenia
     */
    void setAutoLootEnabled(bool enabled) { m_autoLootEnabled = enabled; }

    /**
     * @brief Sprawdza czy autoloot jest włączony
     * @return true jeśli włączony
     */
    bool isAutoLootEnabled() const { return m_autoLootEnabled; }

    /**
     * @brief Pobiera wszystkie zarejestrowane ekwipunki
     * @return Wektor ekwipunków
     */
    const std::vector<InventoryComponent*>& getInventories() const {
        return m_inventories;
    }

private:
    /** Zarejestrowane komponenty ekwipunku */
    std::vector<InventoryComponent*> m_inventories;

    /** Czy autoloot jest włączony */
    bool m_autoLootEnabled;

    /** Czas ostatniej aktualizacji */
    float m_lastUpdateTime;
};
