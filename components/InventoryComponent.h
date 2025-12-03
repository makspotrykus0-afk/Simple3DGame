#pragma once

#include "../core/IComponent.h"
#include "../game/Item.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <chrono>

/**
 * @brief Struktura reprezentująca przedmiot w ekwipunku
 */
struct InventoryItem {
    std::unique_ptr<Item> item;      // Wskaźnik do przedmiotu
    int quantity;                     // Ilość w stacku
    int slotIndex;                    // Indeks slotu (nadmiarowy, ale przydatny)
    std::chrono::high_resolution_clock::time_point acquiredTime; // Czas zdobycia
    
    InventoryItem(std::unique_ptr<Item> it, int qty = 1, int slot = -1)
        : item(std::move(it))
        , quantity(qty)
        , slotIndex(slot)
        , acquiredTime(std::chrono::high_resolution_clock::now())
    {}
    
    /**
     * @brief Pobiera całkowitą wagę stacku
     * @return Waga w kg
     */
    float getTotalWeight() const {
        return item ? item->getWeight() * quantity : 0.0f;
    }
    
    /**
     * @brief Pobiera całkowitą objętość stacku
     * @return Objętość w litrach
     */
    float getTotalVolume() const {
        return item ? item->getVolume() * quantity : 0.0f;
    }
    
    /**
     * @brief Sprawdza czy można stackować z innym przedmiotem
     * @param other Inny przedmiot
     * @return true jeśli można stackować
     */
    bool canStackWith(const InventoryItem& other) const {
        if (!item || !other.item) return false;
        if (!item->isStackable() || !other.item->isStackable()) return false;
        
        // Sprawdź czy to ten sam typ przedmiotu
        return item->getItemType() == other.item->getItemType() &&
               item->getDisplayName() == other.item->getDisplayName();
    }
};

/**
 * @brief Komponent ekwipunku dla encji
 * Zarządza przedmiotami posiadanymi przez encję
 */
class InventoryComponent : public IComponent {
public:
    /**
     * @brief Konstruktor
     * @param ownerId ID właściciela
     * @param capacity Pojemność ekwipunku
     */
    InventoryComponent(const std::string& ownerId, float capacity = 100.0f);

    /**
     * @brief Destruktor
     */
    virtual ~InventoryComponent() = default;

    // IComponent interface
    void update(float deltaTime) override;
    void render() override;
    void initialize() override;
    void shutdown() override;
    std::type_index getComponentType() const override;

    /**
     * @brief Dodaje przedmiot do ekwipunku
     * @param item Przedmiot do dodania
     * @param quantity Ilość
     * @return true jeśli dodano pomyślnie
     */
    bool addItem(std::unique_ptr<Item> item, int quantity = 1);

    /**
     * @brief Usuwa przedmiot z ekwipunku
     * @param type Typ przedmiotu
     * @param quantity Ilość do usunięcia
     * @return true jeśli usunięto pomyślnie
     */
    bool removeItem(ItemType type, int quantity = 1);

    /**
     * @brief Usuwa przedmiot ze slotu
     * @param slotIndex Indeks slotu
     * @return true jeśli usunięto pomyślnie
     */
    bool removeItemAt(int slotIndex);

    /**
     * @brief Sprawdza czy ekwipunek zawiera przedmiot
     * @param type Typ przedmiotu
     * @param quantity Wymagana ilość
     * @return true jeśli zawiera
     */
    bool hasItem(ItemType type, int quantity = 1) const;

    /**
     * @brief Pobiera przedmiot ze slotu
     * @param slotIndex Indeks slotu
     * @return Wskaźnik do przedmiotu lub nullptr
     */
    InventoryItem* getItemAt(int slotIndex);

    /**
     * @brief Znajduje przedmiot po typie
     * @param type Typ przedmiotu
     * @return Wskaźnik do pierwszego znalezionego przedmiotu lub nullptr
     */
    InventoryItem* findItemByType(ItemType type);

    /**
     * @brief Znajduje przedmioty spełniające warunek
     * @param predicate Funkcja warunkowa
     * @return Wektor wskaźników do przedmiotów
     */
    std::vector<InventoryItem*> findItemsByPredicate(
        std::function<bool(const InventoryItem&)> predicate);

    /**
     * @brief Pobiera aktualną wagę ekwipunku
     * @return Waga w kg
     */
    float getCurrentWeight() const;

    /**
     * @brief Pobiera aktualną objętość ekwipunku
     * @return Objętość w litrach
     */
    float getCurrentVolume() const;

    /**
     * @brief Pobiera pozostałą pojemność
     * @return Pozostała pojemność
     */
    float getRemainingCapacity() const;

    /**
     * @brief Pobiera procent wykorzystania pojemności
     * @return Procent (0.0 - 1.0)
     */
    float getCapacityUsage() const;

    /**
     * @brief Sortuje przedmioty w ekwipunku
     */
    void sortItems();

    /**
     * @brief Kompresuje stacki przedmiotów
     */
    void compressItems();

    /**
     * @brief Organizuje przedmioty według typu
     */
    void organizeByType();

    /**
     * @brief Upuszcza przedmiot ze slotu
     * @param slotIndex Indeks slotu
     * @param quantity Ilość do upuszczenia (-1 = wszystko)
     */
    void dropItem(int slotIndex, int quantity = -1);

    /**
     * @brief Pobiera wszystkie przedmioty
     * @return Wektor przedmiotów
     */
    const std::vector<std::unique_ptr<InventoryItem>>& getItems() const { return m_items; }

    /**
     * @brief Pobiera pojemność ekwipunku
     * @return Pojemność
     */
    float getCapacity() const { return m_capacity; }

    /**
     * @brief Ustawia pojemność ekwipunku
     * @param capacity Nowa pojemność
     */
    void setCapacity(float capacity) { m_capacity = capacity; }

    /**
     * @brief Pobiera liczbę przedmiotów
     * @return Liczba przedmiotów
     */
    size_t getItemCount() const;

    /**
     * @brief Sprawdza czy ekwipunek jest pełny
     * @return true jeśli pełny
     */
    bool isFull() const;

    /**
     * @brief Sprawdza czy ekwipunek jest pusty
     * @return true jeśli pusty
     */
    bool isEmpty() const;

    /**
     * @brief Czyści ekwipunek
     */
    void clear();
    
    /**
     * @brief Ustawia maksymalną liczbę slotów
     * @param slots Liczba slotów
     */
    void setMaxSlots(int slots);

    /**
     * @brief Pobiera maksymalną liczbę slotów
     * @return Liczba slotów
     */
    int getMaxSlots() const { return m_maxSlots; }

    /**
     * @brief Callback wywoływany po dodaniu przedmiotu
     * @param item Dodany przedmiot
     */
    void onItemAdded(InventoryItem* item);

    /**
     * @brief Callback wywoływany po usunięciu przedmiotu
     * @param item Usunięty przedmiot
     */
    void onItemRemoved(InventoryItem* item);

    /**
     * @brief Callback wywoływany gdy pojemność zostanie przekroczona
     * @param used Wykorzystana pojemność
     * @param max Maksymalna pojemność
     */
    void onCapacityExceeded(float used, float max);

    // Helper to directly insert/set item at slot (for swap logic mostly)
    void setItemAt(int slotIndex, std::unique_ptr<InventoryItem> item);
    std::unique_ptr<InventoryItem> releaseItemAt(int slotIndex);

    /**
     * @brief Przenosi przedmiot wewnątrz ekwipunku
     * @param fromSlot Indeks slotu źródłowego
     * @param toSlot Indeks slotu docelowego
     */
    void moveItemInternal(int fromSlot, int toSlot);

    /**
     * @brief Próbuje zjeść przedmiot (consumable)
     * @param itemType Typ przedmiotu do zjedzenia (opcjonalne)
     * @return true jeśli zjedzono cokolwiek
     */
    bool consumeItem(ItemType type = ItemType::CONSUMABLE);

private:
    /**
     * @brief Znajduje wolny slot
     * @return Indeks wolnego slotu lub -1
     */
    int findFreeSlot() const;

    /**
     * @brief Próbuje stackować przedmiot z istniejącymi
     * @param item Przedmiot do stackowania
     * @param quantity Ilość
     * @return Ilość pozostała do dodania
     */
    int tryStackItem(Item* item, int quantity);

private:
    /** ID właściciela */
    std::string m_ownerId;

    /** Przedmioty w ekwipunku (fixed size vector with nullptrs for empty slots) */
    std::vector<std::unique_ptr<InventoryItem>> m_items;

    /** Pojemność ekwipunku */
    float m_capacity;

    /** Maksymalna liczba slotów */
    int m_maxSlots;
};
