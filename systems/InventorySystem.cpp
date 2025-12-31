#include "InventorySystem.h"
#include "../entities/GameEntity.h"
#include "../core/EventSystem.h"
#include <algorithm>

InventorySystem::InventorySystem()
    : m_autoLootEnabled(true)
    , m_lastUpdateTime(0.0f)
{
}

void InventorySystem::update(float deltaTime) {
    m_lastUpdateTime += deltaTime;

    // Aktualizuj wszystkie zarejestrowane ekwipunki
    for (auto* inventory : m_inventories) {
        if (inventory) {
            inventory->update(deltaTime);
        }
    }
}

void InventorySystem::render() {
    // Renderowanie systemu ekwipunku
    // Można tutaj dodać wizualizację w trybie debug
}

void InventorySystem::initialize() {
    m_inventories.clear();
    m_autoLootEnabled = true;
    m_lastUpdateTime = 0.0f;
}

void InventorySystem::shutdown() {
    m_inventories.clear();
}

std::string InventorySystem::getName() const {
    return "InventorySystem";
}

int InventorySystem::getPriority() const {
    return 1;
}

void InventorySystem::registerInventory(InventoryComponent* inventory) {
    if (!inventory) {
        return;
    }

    auto it = std::find(m_inventories.begin(), m_inventories.end(), inventory);
    if (it == m_inventories.end()) {
        m_inventories.push_back(inventory);
    }
}

void InventorySystem::unregisterInventory(InventoryComponent* inventory) {
    if (!inventory) {
        return;
    }

    auto it = std::find(m_inventories.begin(), m_inventories.end(), inventory);
    if (it != m_inventories.end()) {
        m_inventories.erase(it);
    }
}

bool InventorySystem::transferItem(InventoryComponent* from, InventoryComponent* to,
                                   int slotIndex, int quantity) {
    if (!from || !to) {
        return false;
    }

    // Pobierz przedmiot ze źródłowego ekwipunku
    InventoryItem* item = from->getItemAt(slotIndex);
    if (!item || !item->item) {
        return false;
    }

    // Określ ilość do przeniesienia
    int transferQuantity = (quantity < 0) ? item->quantity : std::min(quantity, item->quantity);

    // Sklonuj przedmiot
    auto clonedItem = item->item->clone();
    
    // Dodaj do docelowego ekwipunku
    if (to->addItem(std::move(clonedItem), transferQuantity)) {
        // Usuń z źródłowego ekwipunku
        if (transferQuantity >= item->quantity) {
            from->removeItemAt(slotIndex);
        } else {
            item->quantity -= transferQuantity;
        }
        return true;
    }

    return false;
}

bool InventorySystem::moveItem(InventoryComponent* inventory, int fromSlot, int toSlot) {
    if (!inventory) {
        return false;
    }

    InventoryItem* fromItem = inventory->getItemAt(fromSlot);
    InventoryItem* toItem = inventory->getItemAt(toSlot);
    
    if (!fromItem || !fromItem->item) {
        return false; // Nothing to move
    }

    // If target slot has an item, check for stacking or swapping
    if (toItem && toItem->item) {
        // Check if stackable
        if (fromItem->canStackWith(*toItem)) {
            int maxStack = toItem->item->getMaxStackSize();
            int spaceRemaining = maxStack - toItem->quantity;
            
            if (spaceRemaining > 0) {
                // Merge from -> to
                int amountToMove = std::min(spaceRemaining, fromItem->quantity);
                toItem->quantity += amountToMove;
                fromItem->quantity -= amountToMove;
                
                // If source is empty, remove it
                if (fromItem->quantity <= 0) {
                    inventory->removeItemAt(fromSlot);
                }
                return true;
            } else {
                // Target is full, just swap them (or could do nothing)
                inventory->moveItemInternal(fromSlot, toSlot);
                return true;
            }
        } else {
            // Different items -> Swap
            inventory->moveItemInternal(fromSlot, toSlot);
            return true;
        }
    } else {
        // Target is empty -> Move/Swap (effectively move since target is empty)
        inventory->moveItemInternal(fromSlot, toSlot);
        return true;
    }
}

bool InventorySystem::splitStack(InventoryComponent* inventory, int slotIndex, int quantity) {
    if (!inventory) {
        return false;
    }

    InventoryItem* item = inventory->getItemAt(slotIndex);
    if (!item || !item->item || !item->item->isStackable()) {
        return false;
    }

    if (quantity <= 0 || quantity >= item->quantity) {
        return false;
    }

    // Sklonuj przedmiot
    auto clonedItem = item->item->clone();
    
    // Dodaj nowy stack
    if (inventory->addItem(std::move(clonedItem), quantity)) {
        item->quantity -= quantity;
        return true;
    }

    return false;
}

bool InventorySystem::mergeStacks(InventoryComponent* inventory, int slot1, int slot2) {
    if (!inventory) {
        return false;
    }

    InventoryItem* item1 = inventory->getItemAt(slot1);
    InventoryItem* item2 = inventory->getItemAt(slot2);

    if (!item1 || !item2) {
        return false;
    }

    if (!item1->canStackWith(*item2)) {
        return false;
    }

    int maxStack = item1->item->getMaxStackSize();
    int canAdd = maxStack - item1->quantity;
    int toAdd = std::min(canAdd, item2->quantity);

    item1->quantity += toAdd;
    item2->quantity -= toAdd;

    if (item2->quantity <= 0) {
        inventory->removeItemAt(slot2);
    }

    return true;
}

bool InventorySystem::autoLoot(GameEntity* player, std::unique_ptr<Item> item, int quantity) {
    if (!player || !item || !m_autoLootEnabled) {
        return false;
    }

    // Pobierz komponent ekwipunku gracza
    auto inventory = player->getComponent<InventoryComponent>();
    if (!inventory) {
        return false;
    }

    // Sprawdź czy można zebrać
    if (!canAutoLoot(player, item.get(), quantity)) {
        return false;
    }

    // Dodaj przedmiot do ekwipunku
    return inventory->addItem(std::move(item), quantity);
}

bool InventorySystem::canAutoLoot(GameEntity* player, Item* item, int quantity) const {
    if (!player || !item || !m_autoLootEnabled) {
        return false;
    }

    auto inventory = player->getComponent<InventoryComponent>();
    if (!inventory) {
        return false;
    }

    // Sprawdź czy jest miejsce w ekwipunku
    if (inventory->isFull()) {
        return false;
    }

    // Sprawdź czy nie przekroczy limitu wagi
    float itemWeight = item->getWeight() * quantity;
    float remainingCapacity = inventory->getRemainingCapacity();

    return itemWeight <= remainingCapacity;
}

void InventorySystem::sortAllInventories() {
    for (auto* inventory : m_inventories) {
        if (inventory) {
            inventory->sortItems();
        }
    }
}

void InventorySystem::compressAllInventories() {
    for (auto* inventory : m_inventories) {
        if (inventory) {
            inventory->compressItems();
        }
    }
}
