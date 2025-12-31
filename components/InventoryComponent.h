#pragma once

#include "../core/IComponent.h"
#include "../game/Item.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <chrono>
class GameEntity;
struct InventoryItem {
std::unique_ptr<Item> item;
int quantity;
int slotIndex;
std::chrono::high_resolution_clock::time_point acquiredTime;
InventoryItem(std::unique_ptr<Item> it, int qty = 1, int slot = -1)
    : item(std::move(it))
    , quantity(qty)
    , slotIndex(slot)
    , acquiredTime(std::chrono::high_resolution_clock::now())
{}
float getTotalWeight() const {
    return item ? item->getWeight() * quantity : 0.0f;
}
float getTotalVolume() const {
    return item ? item->getVolume() * quantity : 0.0f;
}
bool canStackWith(const InventoryItem& other) const {
    if (!item || !other.item) return false;
    if (!item->isStackable() || !other.item->isStackable()) return false;
    return item->getItemType() == other.item->getItemType() &&
           item->getDisplayName() == other.item->getDisplayName();
}
};
class InventoryComponent : public IComponent {
public:
InventoryComponent(const std::string& ownerId, float capacity = 100.0f, GameEntity* owner = nullptr);
virtual ~InventoryComponent() = default;
void update(float deltaTime) override;
void render() override;
void initialize() override;
void shutdown() override;
std::type_index getComponentType() const override;
bool addItem(std::unique_ptr<Item> item, int quantity = 1);
bool removeItem(ItemType type, int quantity = 1);
bool removeItemAt(int slotIndex);
bool hasItem(ItemType type, int quantity = 1) const;
InventoryItem* getItemAt(int slotIndex);
InventoryItem* findItemByType(ItemType type);
int findFreeSlot() const;
int tryStackItem(Item* item, int quantity);
int getResourceAmount(const std::string& resourceType) const;
bool removeResource(const std::string& resourceType, int amount);
// Additional public methods
void setMaxSlots(int slots);
void setItemAt(int slotIndex, std::unique_ptr<InventoryItem> item);
std::unique_ptr<InventoryItem> releaseItemAt(int slotIndex);
void moveItemInternal(int fromSlot, int toSlot);
bool consumeItem(ItemType type);
std::vector<InventoryItem*> findItemsByPredicate(std::function<bool(const InventoryItem&)> predicate);
float getCurrentWeight() const;
float getCurrentVolume() const;
float getRemainingCapacity() const;
float getCapacityUsage() const;
void sortItems();
void compressItems();
void organizeByType();
void dropItem(int slotIndex, int quantity = 1);
std::unique_ptr<InventoryItem> extractItem(int slotIndex, int quantity = -1);
size_t getItemCount() const;
bool isFull() const;
bool isEmpty() const;
void clear();
private:
std::string m_ownerId;
std::vector<std::unique_ptr<InventoryItem>> m_items;
float m_capacity;
int m_maxSlots;
GameEntity* m_owner;
// Callbacki – muszą być zadeklarowane
void onItemAdded(InventoryItem* item);
void onItemRemoved(InventoryItem* item);
void onCapacityExceeded(float used, float max);
public:
// Niezbędna metoda używana w wielu miejscach
const std::vector<std::unique_ptr<InventoryItem>>& getItems() const { return m_items; }
};
