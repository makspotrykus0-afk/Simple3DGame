#pragma once

#include "../core/IComponent.h"
#include "../game/Item.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Re-use EquipmentSlot from Item.h instead of redefining it
using EquipmentSlot = EquipmentItem::EquipmentSlot;

class EquipmentComponent : public IComponent {
public:
    EquipmentComponent();
    virtual ~EquipmentComponent() override = default;

    // IComponent interface
    void update(float deltaTime) override;
    void render() override;
    void initialize() override;
    void shutdown() override;
    std::type_index getComponentType() const override;

    // Equip an item to a specific slot
    // Returns true if successful
    bool equipItem(EquipmentSlot slot, const Item& item);
    
    // Overload to equip from unique_ptr
    bool equipItem(EquipmentSlot slot, std::unique_ptr<Item> item);

    // Unequip an item from a specific slot
    // Returns the unequipped item or nullptr if slot was empty
    std::unique_ptr<Item> unequipItem(EquipmentSlot slot);

    // Get the item currently equipped in a slot
    const Item* getItemInSlot(EquipmentSlot slot) const;
    
    // Get mutable item
    Item* getItemInSlot(EquipmentSlot slot);

    // Check if a slot is occupied
    bool isSlotOccupied(EquipmentSlot slot) const;

    // Get all equipped items (read-only view)
    std::map<EquipmentSlot, const Item*> getAllEquippedItems() const;

private:
    // Map to store equipped items, where key is the slot and value is the item
    std::map<EquipmentSlot, std::unique_ptr<Item>> m_equippedItems;
};