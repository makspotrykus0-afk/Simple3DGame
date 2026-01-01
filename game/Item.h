#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <raylib.h>

// Forward declarations
class GameEntity;

/**
 * @brief Typy przedmiotów w grze
 */
enum class ItemType : uint32_t {
    RESOURCE = 0,      // Surowce (drewno, kamień, etc.)
    EQUIPMENT = 1,     // Ekwipunek (broń, zbroja)
    CONSUMABLE = 2,    // Przedmioty zużywalne (jedzenie, mikstury)
    TOOL = 3,          // Narzędzia (siekiera, kilof)
    BUILDING = 4,      // Materiały budowlane
    QUEST = 5,         // Przedmioty questowe
    MISC = 6           // Różne
};

/**
 * @brief Rzadkość przedmiotu
 */
enum class ItemRarity : uint32_t {
    COMMON = 0,        // Biały
    UNCOMMON = 1,      // Zielony
    RARE = 2,          // Niebieski
    EPIC = 3,          // Fioletowy
    LEGENDARY = 4,     // Pomarańczowy
    MYTHIC = 5         // Czerwony
};

/**
 * @brief Informacje wyświetlane o przedmiocie
 */
struct ItemDisplayInfo {
    std::string name;
    std::string description;
    ItemType type;
    ItemRarity rarity;
    int quantity;
    float weight;
    float volume;
    int value;
    Color rarityColor;
    std::string iconPath;
    std::unordered_map<std::string, std::string> properties;
};

/**
 * @brief Bazowa klasa dla wszystkich przedmiotów w grze
 */
class Item {
public:
    /**
     * @brief Konstruktor przedmiotu
     * @param itemType Typ przedmiotu
     * @param displayName Nazwa wyświetlana
     * @param description Opis przedmiotu
     */
    Item(ItemType itemType, const std::string& displayName, const std::string& description = "");

    /**
     * @brief Wirtualny destruktor
     */
    virtual ~Item() = default;

    /**
     * @brief Używa przedmiotu
     * @param user Encja używająca przedmiotu
     * @return true jeśli użycie się powiodło
     */
    virtual bool use(GameEntity* user) = 0;

    /**
     * @brief Klonuje przedmiot
     * @return Nowa instancja przedmiotu
     */
    virtual std::unique_ptr<Item> clone() const = 0;

    /**
     * @brief Pobiera informacje wyświetlane o przedmiocie
     * @return Struktura z informacjami
     */
    virtual ItemDisplayInfo getDisplayInfo() const;

    /**
     * @brief Pobiera typ przedmiotu
     * @return Typ przedmiotu
     */
    ItemType getItemType() const { return m_itemType; }

    /**
     * @brief Pobiera nazwę wyświetlaną
     * @return Nazwa przedmiotu
     */
    const std::string& getDisplayName() const { return m_displayName; }

    /**
     * @brief Pobiera opis przedmiotu
     * @return Opis przedmiotu
     */
    const std::string& getDescription() const { return m_description; }

    /**
     * @brief Pobiera wagę przedmiotu
     * @return Waga w kg
     */
    float getWeight() const { return m_weight; }

    /**
     * @brief Pobiera objętość przedmiotu
     * @return Objętość w litrach
     */
    float getVolume() const { return m_volume; }

    /**
     * @brief Pobiera wartość przedmiotu
     * @return Wartość w złocie
     */
    int getValue() const { return m_value; }

    /**
     * @brief Pobiera rzadkość przedmiotu
     * @return Rzadkość
     */
    ItemRarity getRarity() const { return m_rarity; }

    /**
     * @brief Sprawdza czy przedmiot jest stackowalny
     * @return true jeśli można stackować
     */
    bool isStackable() const { return m_stackable; }

    /**
     * @brief Pobiera maksymalny rozmiar stacka
     * @return Maksymalna liczba w stacku
     */
    int getMaxStackSize() const { return m_maxStackSize; }

    /**
     * @brief Ustawia wagę przedmiotu
     * @param weight Nowa waga
     */
    void setWeight(float weight) { m_weight = weight; }

    /**
     * @brief Ustawia objętość przedmiotu
     * @param volume Nowa objętość
     */
    void setVolume(float volume) { m_volume = volume; }

    /**
     * @brief Ustawia wartość przedmiotu
     * @param value Nowa wartość
     */
    void setValue(int value) { m_value = value; }

    /**
     * @brief Ustawia rzadkość przedmiotu
     * @param rarity Nowa rzadkość
     */
    void setRarity(ItemRarity rarity) { m_rarity = rarity; }

    /**
     * @brief Ustawia czy przedmiot jest stackowalny
     * @param stackable Flaga stackowalności
     */
    void setStackable(bool stackable) { m_stackable = stackable; }

    /**
     * @brief Ustawia maksymalny rozmiar stacka
     * @param maxStackSize Maksymalny rozmiar
     */
    void setMaxStackSize(int maxStackSize) { m_maxStackSize = maxStackSize; }

    /**
     * @brief Pobiera kolor dla rzadkości
     * @param rarity Rzadkość
     * @return Kolor raylib
     */
    static Color getRarityColor(ItemRarity rarity);

protected:
    /** Typ przedmiotu */
    ItemType m_itemType;
    
    /** Nazwa wyświetlana */
    std::string m_displayName;
    
    /** Opis przedmiotu */
    std::string m_description;
    
    /** Waga przedmiotu w kg */
    float m_weight;
    
    /** Objętość przedmiotu w litrach */
    float m_volume;
    
    /** Wartość przedmiotu w złocie */
    int m_value;
    
    /** Rzadkość przedmiotu */
    ItemRarity m_rarity;
    
    /** Czy przedmiot jest stackowalny */
    bool m_stackable;
    
    /** Maksymalny rozmiar stacka */
    int m_maxStackSize;
    
    /** Ścieżka do ikony przedmiotu */
    std::string m_iconPath;
    
    /** Dodatkowe właściwości przedmiotu */
    std::unordered_map<std::string, std::string> m_properties;
};

/**
 * @brief Przedmiot zasobowy (drewno, kamień, etc.)
 */
class ResourceItem : public Item {
public:
    /**
     * @brief Konstruktor przedmiotu zasobowego
     * @param resourceType Typ zasobu
     * @param displayName Nazwa wyświetlana
     * @param description Opis
     */
    ResourceItem(const std::string& resourceType, const std::string& displayName, 
                 const std::string& description = "");

    /**
     * @brief Destruktor
     */
    virtual ~ResourceItem() = default;

    /**
     * @brief Używa przedmiotu zasobowego
     * @param user Encja używająca
     * @return true jeśli użycie się powiodło
     */
    bool use(GameEntity* user) override;

    /**
     * @brief Klonuje przedmiot
     * @return Nowa instancja
     */
    std::unique_ptr<Item> clone() const override;

    /**
     * @brief Pobiera typ zasobu
     * @return Typ zasobu jako string
     */
    const std::string& getResourceType() const { return m_resourceType; }

    /**
     * @brief Pobiera jakość zasobu
     * @return Jakość (0.0 - 1.0)
     */
    float getQuality() const { return m_quality; }

    /**
     * @brief Ustawia jakość zasobu
     * @param quality Nowa jakość
     */
    void setQuality(float quality) { m_quality = quality; }

private:
    /** Typ zasobu */
    std::string m_resourceType;
    
    /** Jakość zasobu (0.0 - 1.0) */
    float m_quality;
};

/**
 * @brief Przedmiot ekwipunku (broń, zbroja)
 */
class EquipmentItem : public Item {
public:
    /**
     * @brief Slot ekwipunku
     */
    enum class EquipmentSlot : uint32_t {
        HEAD = 0,
        CHEST = 1,
        LEGS = 2,
        FEET = 3,
        HANDS = 4,
        MAIN_HAND = 5,
        OFF_HAND = 6,
        ACCESSORY_1 = 7,
        ACCESSORY_2 = 8
    };

    /**
     * @brief Statystyki ekwipunku
     */
    struct EquipmentStats {
        float armor;
        float attackPower;
        float defense;
        float speed;
        float critChance;
        std::unordered_map<std::string, float> bonuses;

        EquipmentStats() : armor(0.0f), attackPower(0.0f), defense(0.0f),
                          speed(0.0f), critChance(0.0f) {}
    };

    /**
     * @brief Konstruktor przedmiotu ekwipunku
     * @param displayName Nazwa wyświetlana
     * @param slot Slot ekwipunku
     * @param description Opis
     */
    EquipmentItem(const std::string& displayName, EquipmentSlot slot,
                  const std::string& description = "");

    /**
     * @brief Destruktor
     */
    virtual ~EquipmentItem() = default;

    /**
     * @brief Używa przedmiotu (zakłada ekwipunek)
     * @param user Encja używająca
     * @return true jeśli użycie się powiodło
     */
    bool use(GameEntity* user) override;

    /**
     * @brief Klonuje przedmiot
     * @return Nowa instancja
     */
    std::unique_ptr<Item> clone() const override;

    /**
     * @brief Zakłada ekwipunek
     * @param user Encja zakładająca
     * @return true jeśli zakładanie się powiodło
     */
    bool equip(GameEntity* user);

    /**
     * @brief Zdejmuje ekwipunek
     * @param user Encja zdejmująca
     * @return true jeśli zdejmowanie się powiodło
     */
    bool unequip(GameEntity* user);

    /**
     * @brief Sprawdza czy ekwipunek jest założony
     * @return true jeśli założony
     */
    bool isEquipped() const { return m_equipped; }

    /**
     * @brief Pobiera slot ekwipunku
     * @return Slot
     */
    EquipmentSlot getEquipmentSlot() const { return m_equipmentSlot; }

    /**
     * @brief Pobiera statystyki ekwipunku
     * @return Statystyki
     */
    const EquipmentStats& getStats() const { return m_stats; }

    /**
     * @brief Ustawia statystyki ekwipunku
     * @param stats Nowe statystyki
     */
    void setStats(const EquipmentStats& stats) { m_stats = stats; }

private:
    /** Slot ekwipunku */
    EquipmentSlot m_equipmentSlot;
    
    /** Statystyki ekwipunku */
    EquipmentStats m_stats;
    
    /** Czy ekwipunek jest założony */
    bool m_equipped;
};

/**
 * @brief Przedmiot zużywalny (jedzenie, mikstury)
 */
class ConsumableItem : public Item {
public:
    /**
     * @brief Konstruktor przedmiotu zużywalnego
     * @param displayName Nazwa wyświetlana
     * @param description Opis
     */
    ConsumableItem(const std::string& displayName, const std::string& description = "");

    /**
     * @brief Destruktor
     */
    virtual ~ConsumableItem() = default;

    /**
     * @brief Używa przedmiotu (konsumuje)
     * @param user Encja używająca
     * @return true jeśli użycie się powiodło
     */
    bool use(GameEntity* user) override;

    /**
     * @brief Renderuje przedmiot w świecie gry
     * @param position Pozycja renderowania
     */
    void render(Vector3 position);

    /**
     * @brief Klonuje przedmiot
     * @return Nowa instancja
     */
    std::unique_ptr<Item> clone() const override;

    /**
     * @brief Pobiera efekt zdrowia
     * @return Ilość zdrowia do przywrócenia
     */
    float getHealthEffect() const { return m_healthEffect; }

    /**
     * @brief Pobiera efekt many
     * @return Ilość many do przywrócenia
     */
    float getManaEffect() const { return m_manaEffect; }

    /**
     * @brief Pobiera efekt wytrzymałości
     * @return Ilość wytrzymałości do przywrócenia
     */
    float getStaminaEffect() const { return m_staminaEffect; }

    /**
     * @brief Ustawia efekt zdrowia
     * @param effect Nowy efekt
     */
    void setHealthEffect(float effect) { m_healthEffect = effect; }

    /**
     * @brief Ustawia efekt many
     * @param effect Nowy efekt
     */
    void setManaEffect(float effect) { m_manaEffect = effect; }

    /**
     * @brief Ustawia efekt wytrzymałości
     * @param effect Nowy efekt
     */
    void setStaminaEffect(float effect) { m_staminaEffect = effect; }

private:
    /** Efekt zdrowia */
    float m_healthEffect;
    
    /** Efekt many */
    float m_manaEffect;
    
    /** Efekt wytrzymałości */
    float m_staminaEffect;
};

/**
 * @brief Przedmiot broni
 */
class WeaponItem : public EquipmentItem {
public:
    /**
     * @brief Konstruktor broni
     * @param displayName Nazwa wyświetlana
     * @param slot Slot ekwipunku (zazwyczaj MAIN_HAND)
     * @param damage Obrażenia
     * @param range Zasięg
     * @param cooldown Czas odnowienia
     * @param description Opis
     */
    WeaponItem(const std::string& displayName, EquipmentSlot slot, float damage, float range, float cooldown, const std::string& description = "")
        : EquipmentItem(displayName, slot, description), m_damage(damage), m_range(range), m_cooldown(cooldown), m_currentCooldown(0.0f) {}

    virtual ~WeaponItem() = default;

    /**
     * @brief Klonuje broń
     */
    std::unique_ptr<Item> clone() const override {
        auto clone = std::make_unique<WeaponItem>(m_displayName, getEquipmentSlot(), m_damage, m_range, m_cooldown, m_description);
        clone->setStats(getStats());
        // Copy other base properties if needed
        return clone;
    }

    float getDamage() const { return m_damage; }
    float getRange() const { return m_range; }
    float getCooldown() const { return m_cooldown; }
    
    bool canFire() const { return m_currentCooldown <= 0.0f; }
    void resetCooldown() { m_currentCooldown = m_cooldown; }
    void updateCooldown(float deltaTime) {
        if (m_currentCooldown > 0.0f) m_currentCooldown -= deltaTime;
    }

private:
    float m_damage;
    float m_range;
    float m_cooldown;
    float m_currentCooldown;
};

/**
 * @brief Przedmiot różnego typu (niezasobowy, nieekwipunkowy)
 */
class MiscItem : public Item {
public:
    /**
     * @brief Konstruktor przedmiotu różnego
     * @param name Nazwa wyświetlana
     * @param desc Opis
     */
    MiscItem(const std::string& name, const std::string& desc = "")
        : Item(ItemType::MISC, name, desc) {}

    virtual ~MiscItem() = default;

    /**
     * @brief Użycie przedmiotu - domyślnie nic nie robi
     */
    bool use(GameEntity* /*user*/) override { return false; }

    /**
     * @brief Klonowanie przedmiotu
     */
    std::unique_ptr<Item> clone() const override {
        auto item = std::make_unique<MiscItem>(m_displayName, m_description);
        item->setWeight(m_weight);
        item->setValue(m_value);
        item->setVolume(m_volume);
        item->setRarity(m_rarity);
        item->setStackable(m_stackable);
        item->setMaxStackSize(m_maxStackSize);
        return item;
    }
};
