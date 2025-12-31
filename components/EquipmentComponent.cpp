#include "EquipmentComponent.h"
#include "../game/Item.h"
#include <iostream>

EquipmentComponent::EquipmentComponent() {
}

void EquipmentComponent::initialize() {
    // Initialization logic if needed
}

void EquipmentComponent::update(float deltaTime) {
    // Update logic if needed (e.g. durability decay)
}

void EquipmentComponent::render() {
    // Render logic usually handled by systems, but available here
}

void EquipmentComponent::shutdown() {
    m_equippedItems.clear();
}

std::type_index EquipmentComponent::getComponentType() const {
    return std::type_index(typeid(EquipmentComponent));
}

bool EquipmentComponent::equipItem(EquipmentSlot slot, const Item& item) {
    if (isSlotOccupied(slot)) {
        return false;
    }
    
    // Clone the item
    m_equippedItems[slot] = item.clone();
    return true;
}

bool EquipmentComponent::equipItem(EquipmentSlot slot, std::unique_ptr<Item> item) {
    if (!item) return false;
    
    if (isSlotOccupied(slot)) {
        return false;
    }
    
    m_equippedItems[slot] = std::move(item);
    return true;
}

std::unique_ptr<Item> EquipmentComponent::unequipItem(EquipmentSlot slot) {
    auto it = m_equippedItems.find(slot);
    if (it != m_equippedItems.end()) {
        std::unique_ptr<Item> item = std::move(it->second);
        m_equippedItems.erase(it);
        return item;
    }
    return nullptr;
}

const Item* EquipmentComponent::getItemInSlot(EquipmentSlot slot) const {
    auto it = m_equippedItems.find(slot);
    if (it != m_equippedItems.end()) {
        return it->second.get();
    }
    return nullptr;
}

Item* EquipmentComponent::getItemInSlot(EquipmentSlot slot) {
    auto it = m_equippedItems.find(slot);
    if (it != m_equippedItems.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool EquipmentComponent::isSlotOccupied(EquipmentSlot slot) const {
    return m_equippedItems.find(slot) != m_equippedItems.end();
}

std::map<EquipmentSlot, const Item*> EquipmentComponent::getAllEquippedItems() const {
    std::map<EquipmentSlot, const Item*> result;
    for (const auto& pair : m_equippedItems) {
        result[pair.first] = pair.second.get();
    }
    return result;
}