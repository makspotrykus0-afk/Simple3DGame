#include "Item.h"
#include "../entities/GameEntity.h"
#include "../components/ResourceComponent.h"
#include <algorithm>

// ============================================================================
// Item - Bazowa klasa
// ============================================================================

Item::Item(ItemType itemType, const std::string& displayName, const std::string& description)
    : m_itemType(itemType)
    , m_displayName(displayName)
    , m_description(description)
    , m_weight(1.0f)
    , m_volume(1.0f)
    , m_value(1)
    , m_rarity(ItemRarity::COMMON)
    , m_stackable(true)
    , m_maxStackSize(64)
    , m_iconPath("")
{
}

ItemDisplayInfo Item::getDisplayInfo() const {
    ItemDisplayInfo info;
    info.name = m_displayName;
    info.description = m_description;
    info.type = m_itemType;
    info.rarity = m_rarity;
    info.quantity = 1;
    info.weight = m_weight;
    info.volume = m_volume;
    info.value = m_value;
    info.rarityColor = getRarityColor(m_rarity);
    info.iconPath = m_iconPath;
    info.properties = m_properties;
    return info;
}

Color Item::getRarityColor(ItemRarity rarity) {
    switch (rarity) {
        case ItemRarity::COMMON:
            return WHITE;
        case ItemRarity::UNCOMMON:
            return GREEN;
        case ItemRarity::RARE:
            return BLUE;
        case ItemRarity::EPIC:
            return PURPLE;
        case ItemRarity::LEGENDARY:
            return ORANGE;
        case ItemRarity::MYTHIC:
            return RED;
        default:
            return WHITE;
    }
}

// ============================================================================
// ResourceItem - Przedmiot zasobowy
// ============================================================================

ResourceItem::ResourceItem(const std::string& resourceType, const std::string& displayName,
                           const std::string& description)
    : Item(ItemType::RESOURCE, displayName, description)
    , m_resourceType(resourceType)
    , m_quality(1.0f)
{
    // Zasoby są zawsze stackowalne
    m_stackable = true;
    m_maxStackSize = 999;
    
    // Ustaw domyślne właściwości zasobów
    m_weight = 0.5f;
    m_volume = 0.5f;
    m_value = 1;
    m_rarity = ItemRarity::COMMON;
}

bool ResourceItem::use(GameEntity* user) {
    if (!user) {
        return false;
    }

    // Zasoby nie są bezpośrednio używane, ale mogą być dodane do komponentu zasobów
    auto resourceComp = user->getComponent<ResourceComponent>();
    if (resourceComp) {
        // Dodaj zasób do komponentu
        resourceComp->addResource(m_resourceType, 1);
        return true;
    }

    return false;
}

std::unique_ptr<Item> ResourceItem::clone() const {
    auto cloned = std::make_unique<ResourceItem>(m_resourceType, m_displayName, m_description);
    cloned->setWeight(m_weight);
    cloned->setVolume(m_volume);
    cloned->setValue(m_value);
    cloned->setRarity(m_rarity);
    cloned->setQuality(m_quality);
    return cloned;
}

// ============================================================================
// EquipmentItem - Przedmiot ekwipunku
// ============================================================================

EquipmentItem::EquipmentItem(const std::string& displayName, EquipmentSlot slot,
                             const std::string& description)
    : Item(ItemType::EQUIPMENT, displayName, description)
    , m_equipmentSlot(slot)
    , m_equipped(false)
{
    // Ekwipunek nie jest stackowalny
    m_stackable = false;
    m_maxStackSize = 1;
    
    // Ustaw domyślne właściwości ekwipunku
    m_weight = 2.0f;
    m_volume = 2.0f;
    m_value = 10;
    m_rarity = ItemRarity::COMMON;
}

bool EquipmentItem::use(GameEntity* user) {
    if (!user) {
        return false;
    }

    // Użycie ekwipunku = zakładanie/zdejmowanie
    if (m_equipped) {
        return unequip(user);
    } else {
        return equip(user);
    }
}

std::unique_ptr<Item> EquipmentItem::clone() const {
    auto cloned = std::make_unique<EquipmentItem>(m_displayName, m_equipmentSlot, m_description);
    cloned->setWeight(m_weight);
    cloned->setVolume(m_volume);
    cloned->setValue(m_value);
    cloned->setRarity(m_rarity);
    cloned->setStats(m_stats);
    return cloned;
}

bool EquipmentItem::equip(GameEntity* user) {
    if (!user || m_equipped) {
        return false;
    }

    // TODO: Integracja z EquipmentSystem
    // Na razie tylko oznacz jako założony
    m_equipped = true;
    
    return true;
}

bool EquipmentItem::unequip(GameEntity* user) {
    if (!user || !m_equipped) {
        return false;
    }

    // TODO: Integracja z EquipmentSystem
    // Na razie tylko oznacz jako zdjęty
    m_equipped = false;
    
    return true;
}

// ============================================================================
// ConsumableItem - Przedmiot zużywalny
// ============================================================================

ConsumableItem::ConsumableItem(const std::string& displayName, const std::string& description)
    : Item(ItemType::CONSUMABLE, displayName, description)
    , m_healthEffect(0.0f)
    , m_manaEffect(0.0f)
    , m_staminaEffect(0.0f)
{
    // Przedmioty zużywalne są stackowalne
    m_stackable = true;
    m_maxStackSize = 20;
    
    // Ustaw domyślne właściwości
    m_weight = 0.2f;
    m_volume = 0.2f;
    m_value = 5;
    m_rarity = ItemRarity::COMMON;
}

bool ConsumableItem::use(GameEntity* user) {
    if (!user) {
        return false;
    }

    // TODO: Integracja z systemem statystyk postaci
    // Na razie tylko zwróć sukces
    // W przyszłości: zastosuj efekty do postaci
    
    return true;
}

std::unique_ptr<Item> ConsumableItem::clone() const {
    auto cloned = std::make_unique<ConsumableItem>(m_displayName, m_description);
    cloned->setWeight(m_weight);
    cloned->setVolume(m_volume);
    cloned->setValue(m_value);
    cloned->setRarity(m_rarity);
    cloned->setHealthEffect(m_healthEffect);
    cloned->setManaEffect(m_manaEffect);
    cloned->setStaminaEffect(m_staminaEffect);
    return cloned;
}
