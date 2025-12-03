#include "EquipmentSystem.h"
#include "../core/GameEntity.h"
#include "../components/EquipmentComponent.h"
#include "../components/StatsComponent.h"
#include <iostream>

EquipmentSystem::EquipmentSystem() : m_name("EquipmentSystem") {
}

void EquipmentSystem::initialize() {
    // Inicjalizacja systemu
}

void EquipmentSystem::update(float deltaTime) {
    // System działa na żądanie, więc update może być pusty lub obsługiwać efekty w czasie
}

void EquipmentSystem::render() {
    // Renderowanie debugowe jeśli potrzebne
}

void EquipmentSystem::shutdown() {
    // Czyszczenie systemu
}

std::string EquipmentSystem::getName() const {
    return m_name;
}

int EquipmentSystem::getPriority() const {
    return 5; // Średni priorytet
}

bool EquipmentSystem::equipItem(GameEntity* entity, std::unique_ptr<Item> item) {
    if (!entity || !item) return false;

    // Sprawdź czy encja ma komponent ekwipunku
    auto equipmentComp = entity->getComponent<EquipmentComponent>();
    if (!equipmentComp) return false;

    // Sprawdź czy przedmiot to ekwipunek
    if (item->getItemType() != ItemType::EQUIPMENT) {
        std::cout << "Item is not equipment: " << item->getDisplayName() << std::endl;
        return false;
    }

    // Rzutuj na EquipmentItem aby pobrać slot i statystyki
    EquipmentItem* equipItem = static_cast<EquipmentItem*>(item.get());
    EquipmentItem::EquipmentSlot slot = equipItem->getEquipmentSlot();

    // Jeśli slot jest zajęty, spróbuj zdjąć aktualny przedmiot
    if (equipmentComp->isSlotOccupied(slot)) {
        auto unequipped = unequipItem(entity, slot);
        if (!unequipped) {
            std::cout << "Failed to unequip item from slot " << (int)slot << std::endl;
            return false;
        }
        // Tutaj można dodać logikę co zrobić ze zdjętym przedmiotem (np. dodać do ekwipunku)
        // Na razie po prostu go tracimy lub zwracamy (w pełnej implementacji powinien trafić do Inventory)
    }

    // Pobierz statystyki przed założeniem (aby je zaaplikować)
    const auto& stats = equipItem->getStats();

    // Spróbuj założyć przedmiot
    if (equipmentComp->equipItem(slot, std::move(item))) {
        // Zaaplikuj statystyki
        auto statsComp = entity->getComponent<StatsComponent>();
        if (statsComp) {
            applyStats(statsComp.get(), stats, true);
        }
        
        std::cout << "Equipped item to slot " << (int)slot << std::endl;
        return true;
    }

    return false;
}

std::unique_ptr<Item> EquipmentSystem::unequipItem(GameEntity* entity, EquipmentSlot slot) {
    if (!entity) return nullptr;

    auto equipmentComp = entity->getComponent<EquipmentComponent>();
    if (!equipmentComp) return nullptr;

    if (!equipmentComp->isSlotOccupied(slot)) return nullptr;

    // Pobierz przedmiot przed zdjęciem aby odjąć statystyki
    Item* rawItem = equipmentComp->getItemInSlot(slot);
    if (rawItem && rawItem->getItemType() == ItemType::EQUIPMENT) {
        EquipmentItem* equipItem = static_cast<EquipmentItem*>(rawItem);
        auto statsComp = entity->getComponent<StatsComponent>();
        if (statsComp) {
            applyStats(statsComp.get(), equipItem->getStats(), false);
        }
    }

    return equipmentComp->unequipItem(slot);
}

void EquipmentSystem::applyStats(StatsComponent* statsComp, const EquipmentItem::EquipmentStats& itemStats, bool apply) {
    if (!statsComp) return;

    float multiplier = apply ? 1.0f : -1.0f;

    // Aplikowanie statystyk
    // Note: StatsComponent currently has health, energy, stamina. 
    // EquipmentStats has armor, attackPower, defense, speed, critChance.
    // We need to map these or extend StatsComponent. 
    // For now, we'll assume StatsComponent might handle these via attributes or we just skip 
    // implementation if StatsComponent doesn't support them directly yet.
    
    // W przyszłości:
    // statsComp->modifyAttribute("armor", itemStats.armor * multiplier);
    // statsComp->modifyAttribute("attackPower", itemStats.attackPower * multiplier);
    
    // Na razie logujemy
    // std::cout << (apply ? "Applying" : "Removing") << " stats: " 
    //           << "Armor: " << itemStats.armor << ", "
    //           << "Attack: " << itemStats.attackPower << std::endl;
}
