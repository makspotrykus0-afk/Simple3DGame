
#include "InventoryComponent.h"
#include "../core/EventSystem.h"
#include <algorithm>
#include "../events/InteractionEvents.h"        // InventoryChangedEvent
#include <iostream>               // std::cout, std::endl

InventoryComponent::InventoryComponent(const std::string& ownerId, float capacity, GameEntity* owner)
: m_ownerId(ownerId)
, m_capacity(capacity)
, m_maxSlots(50)
, m_owner(owner)
{
// Initialize vector with nullptrs
m_items.resize(m_maxSlots);
}
void InventoryComponent::initialize() {
m_items.clear();
m_items.resize(m_maxSlots);
}
void InventoryComponent::update(float deltaTime) {
// Aktualizacja komponentu ekwipunku
// Można tutaj dodać logikę automatycznego sortowania, czyszczenia itp.
}
void InventoryComponent::render() {
// Renderowanie komponentu ekwipunku
// Wizualizacja w trybie debug
}
void InventoryComponent::shutdown() {
m_items.clear();
}
std::type_index InventoryComponent::getComponentType() const {
return std::type_index(typeid(InventoryComponent));
}
void InventoryComponent::setMaxSlots(int slots) {
m_maxSlots = slots;
m_items.resize(m_maxSlots);
}
// New helper to set item directly at slot
void InventoryComponent::setItemAt(int slotIndex, std::unique_ptr<InventoryItem> item) {
if (slotIndex >= 0 && slotIndex < static_cast<int>(m_items.size())) {
m_items[slotIndex] = std::move(item);
if (m_items[slotIndex]) {
m_items[slotIndex]->slotIndex = slotIndex;
}
}
}
// New helper to release item from slot without deleting
std::unique_ptr<InventoryItem> InventoryComponent::releaseItemAt(int slotIndex) {
if (slotIndex >= 0 && slotIndex < static_cast<int>(m_items.size())) {
return std::move(m_items[slotIndex]);
}
return nullptr;
}
void InventoryComponent::moveItemInternal(int fromSlot, int toSlot) {
if (fromSlot < 0 || fromSlot >= static_cast<int>(m_items.size())) return;
if (toSlot < 0 || toSlot >= static_cast<int>(m_items.size())) return;
// Swap pointers directly
std::swap(m_items[fromSlot], m_items[toSlot]);
// Update slotIndex for both if they exist
if (m_items[fromSlot]) {
    m_items[fromSlot]->slotIndex = fromSlot;
}
if (m_items[toSlot]) {
    m_items[toSlot]->slotIndex = toSlot;
}
}

bool InventoryComponent::consumeItem(ItemType type) {
// Find first consumable
for (auto& invItem : m_items) {
if (invItem && invItem->item) {
bool isConsumable = (invItem->item->getItemType() == ItemType::CONSUMABLE);
bool matchType = (type == ItemType::CONSUMABLE) || (invItem->item->getItemType() == type);
        if (isConsumable && matchType) {
            // Consume one
            invItem->quantity--;
            if (invItem->quantity <= 0) {
                invItem.reset();
            }
            return true;
        }
    }
}
return false;
}

bool InventoryComponent::addItem(std::unique_ptr<Item> item, int quantity) {
    if (!item || quantity <= 0) {
        return false;
    }
    // Sprawdź czy ekwipunek nie jest pełny (wagowo)
    if (isFull()) {
        std::cout << "[Inventory] AddItem Failed: Full capacity/weight." << std::endl;
        onCapacityExceeded(getCurrentWeight(), m_capacity);
        return false;
    }
    // Jeśli przedmiot jest stackowalny, spróbuj stackować z istniejącymi
    if (item->isStackable()) {
        int remaining = tryStackItem(item.get(), quantity);
        // Jeśli wszystko zostało stackowane, zwróć sukces
        if (remaining == 0) {
            std::cout << "[Inventory] Stacked item completely: " << item->getDisplayName() << std::endl;
            return true;
        }
        quantity = remaining;
    }
    // Znajdź wolny slot
    int slotIndex = findFreeSlot();
    if (slotIndex < 0) {
        std::cout << "[Inventory] AddItem Failed: No free slots." << std::endl;
        return false; // Brak miejsca w slotach
    }
    std::cout << "[Inventory] Added item to new slot " << slotIndex << ": " << item->getDisplayName() << std::endl;
    auto invItem = std::make_unique<InventoryItem>(std::move(item), quantity, slotIndex);
    // Wstaw w slot
    m_items[slotIndex] = std::move(invItem);
    onItemAdded(m_items[slotIndex].get());
    std::cout << "[Inventory] Dodano przedmiot do ekwipunku właściciela: " << m_ownerId << std::endl;
    return true;
}
bool InventoryComponent::removeItem(ItemType type, int quantity) {
if (quantity <= 0) {
return false;
}
int remaining = quantity;
// Iterujemy po wszystkich slotach
for (size_t i = 0; i < m_items.size(); ++i) {
if (!m_items[i] || !m_items[i]->item) continue;
if (m_items[i]->item->getItemType() == type) {
if (m_items[i]->quantity <= remaining) {
remaining -= m_items[i]->quantity;
onItemRemoved(m_items[i].get());
m_items[i].reset(); // Zwolnij slot
} else {
m_items[i]->quantity -= remaining;
remaining = 0;
}
if (remaining == 0) break;
}
}
return remaining < quantity;
}
bool InventoryComponent::removeItemAt(int slotIndex) {
if (slotIndex < 0 || slotIndex >= static_cast<int>(m_items.size())) {
return false;
}
if (!m_items[slotIndex]) {
    return false;
}
onItemRemoved(m_items[slotIndex].get());
m_items[slotIndex].reset(); // Zwolnij slot (ustaw nullptr)
return true;
}

bool InventoryComponent::hasItem(ItemType type, int quantity) const {
int totalQuantity = 0;
for (const auto& item : m_items) {
if (item && item->item && item->item->getItemType() == type) {
totalQuantity += item->quantity;
}
}
return totalQuantity >= quantity;
}
int InventoryComponent::getResourceAmount(const std::string& resourceType) const {
int total = 0;
for (const auto& item : m_items) {
if (item && item->item) {
// Sprawdź czy to ResourceItem i czy pasuje typ
if (item->item->getItemType() == ItemType::RESOURCE) {
// Rzutowanie na ResourceItem* - potrzebujemy dostępu do getResourceType()
ResourceItem* resItem = dynamic_cast<ResourceItem*>(item->item.get());
                if (resItem && resItem->getResourceType() == resourceType) {
                    total += item->quantity;
                }
            }
        }
    }
    return total;
}
bool InventoryComponent::removeResource(const std::string& resourceType, int amount) {
if (amount <= 0) return false;
int remaining = amount;
// Iterujemy po wszystkich slotach
for (size_t i = 0; i < m_items.size(); ++i) {
if (!m_items[i] || !m_items[i]->item) continue;
// Sprawdź czy to ResourceItem i czy pasuje typ
if (m_items[i]->item->getItemType() == ItemType::RESOURCE) {
ResourceItem* resItem = dynamic_cast<ResourceItem*>(m_items[i]->item.get());
if (resItem && resItem->getResourceType() == resourceType) {
if (m_items[i]->quantity <= remaining) {
remaining -= m_items[i]->quantity;
onItemRemoved(m_items[i].get());
m_items[i].reset(); // Zwolnij slot
} else {
m_items[i]->quantity -= remaining;
remaining = 0;
}
if (remaining == 0) break;
}
}
}
return remaining < amount; // true jeśli usunięto przynajmniej część
}
InventoryItem* InventoryComponent::getItemAt(int slotIndex) {
if (slotIndex < 0 || slotIndex >= static_cast<int>(m_items.size())) {
return nullptr;
}
return m_items[slotIndex].get();
}
InventoryItem* InventoryComponent::findItemByType(ItemType type) {
for (const auto& item : m_items) {
if (item && item->item && item->item->getItemType() == type) {
return item.get();
}
}
return nullptr;
}
std::vector<InventoryItem*> InventoryComponent::findItemsByPredicate(
std::function<bool(const InventoryItem&)> predicate) {
std::vector<InventoryItem*> result;
for (const auto& item : m_items) {
    if (item && predicate(*item)) {
        result.push_back(item.get());
    }
}
return result;
}

float InventoryComponent::getCurrentWeight() const {
float totalWeight = 0.0f;
for (const auto& item : m_items) {
if (item) {
totalWeight += item->getTotalWeight();
}
}
return totalWeight;
}
float InventoryComponent::getCurrentVolume() const {
float totalVolume = 0.0f;
for (const auto& item : m_items) {
if (item) {
totalVolume += item->getTotalVolume();
}
}
return totalVolume;
}
float InventoryComponent::getRemainingCapacity() const {
return m_capacity - getCurrentWeight();
}
float InventoryComponent::getCapacityUsage() const {
if (m_capacity <= 0.0f) {
return 0.0f;
}
return getCurrentWeight() / m_capacity;
}
void InventoryComponent::sortItems() {
// Zbierz wszystkie niepuste przedmioty
std::vector<std::unique_ptr<InventoryItem>> tempItems;
for(auto& item : m_items) {
if(item) tempItems.push_back(std::move(item));
}
// Posortuj
std::sort(tempItems.begin(), tempItems.end(),
    [](const std::unique_ptr<InventoryItem>& a, const std::unique_ptr<InventoryItem>& b) {
        if (!a || !a->item) return false;
        if (!b || !b->item) return true;
        if (a->item->getItemType() != b->item->getItemType()) {
            return static_cast<int>(a->item->getItemType()) < static_cast<int>(b->item->getItemType());
        }
        return a->item->getDisplayName() < b->item->getDisplayName();
    }
);
// Wyczyść i wypełnij od nowa
m_items.clear();
m_items.resize(m_maxSlots);
for(size_t i = 0; i < tempItems.size() && i < m_items.size(); ++i) {
    m_items[i] = std::move(tempItems[i]);
    m_items[i]->slotIndex = static_cast<int>(i);
}
}

void InventoryComponent::compressItems() {
// Najpierw stackuj
for (size_t i = 0; i < m_items.size(); ++i) {
if (!m_items[i] || !m_items[i]->item || !m_items[i]->item->isStackable()) continue;
    for (size_t j = i + 1; j < m_items.size(); ++j) {
        if (!m_items[j] || !m_items[j]->item) continue;
        if (m_items[i]->canStackWith(*m_items[j])) {
            int maxStack = m_items[i]->item->getMaxStackSize();
            int canAdd = maxStack - m_items[i]->quantity;
            int toAdd = std::min(canAdd, m_items[j]->quantity);
            m_items[i]->quantity += toAdd;
            m_items[j]->quantity -= toAdd;
            if (m_items[j]->quantity <= 0) {
                m_items[j].reset();
            }
        }
    }
}
// Opcjonalnie przesuń na początek (deframentacja) - na razie wyłączone, żeby zachować sloty
// sortItems() robi defragmentację przy okazji

}
void InventoryComponent::organizeByType() {
sortItems(); // To posortuje i skompresuje luki
}
void InventoryComponent::dropItem(int slotIndex, int quantity) {
if (slotIndex < 0 || slotIndex >= static_cast<int>(m_items.size())) return;
auto& item = m_items[slotIndex];
if (!item) return;
if (quantity < 0 || quantity >= item->quantity) {
    removeItemAt(slotIndex);
} else {
    item->quantity -= quantity;
}
}

std::unique_ptr<InventoryItem> InventoryComponent::extractItem(int slotIndex, int quantity) {
if (slotIndex < 0 || slotIndex >= static_cast<int>(m_items.size())) return nullptr;
auto& invItem = m_items[slotIndex];
if (!invItem || !invItem->item) return nullptr;
if (quantity < 0 || quantity >= invItem->quantity) {
    // Extract entire slot
    std::unique_ptr<InventoryItem> extracted = std::move(invItem);
    // invItem is now nullptr (moved from)
    // Trigger removal logic
    onItemRemoved(extracted.get());
    m_items[slotIndex] = nullptr; // Explicitly nullify to be safe
    return extracted;
} else {
    // Extract partial
    if (quantity <= 0) return nullptr;
    // Clone the item definition for the new stack
    std::unique_ptr<Item> newItem = invItem->item->clone();
    if (!newItem) return nullptr;
    auto extracted = std::make_unique<InventoryItem>(std::move(newItem), quantity);
    // Reduce current stack
    invItem->quantity -= quantity;
    return extracted;
}
}

size_t InventoryComponent::getItemCount() const {
size_t count = 0;
for(const auto& item : m_items) {
if(item) count++;
}
return count;
}
bool InventoryComponent::isFull() const {
// Pełny wagowo lub brak slotów
if (getCurrentWeight() >= m_capacity) return true;
return findFreeSlot() == -1;
}
bool InventoryComponent::isEmpty() const {
for(const auto& item : m_items) {
if(item) return false;
}
return true;
}
void InventoryComponent::clear() {
for(auto& item : m_items) {
item.reset();
}
}
void InventoryComponent::onItemAdded(InventoryItem* item) {
// TODO: Wyślij event o dodaniu przedmiotu
if (m_owner) {
InventoryChangedEvent event(m_owner);
EVENT_BUS.send(event);
}
}
void InventoryComponent::onItemRemoved(InventoryItem* item) {
// TODO: Wyślij event o usunięciu przedmiotu
if (m_owner) {
InventoryChangedEvent event(m_owner);
EVENT_BUS.send(event);
}
}
void InventoryComponent::onCapacityExceeded(float used, float max) {
// TODO: Wyślij event o przekroczeniu pojemności
std::cout << "[Inventory] Capacity exceeded: " << used << "/" << max << std::endl;
}
int InventoryComponent::findFreeSlot() const {
for (size_t i = 0; i < m_items.size(); ++i) {
if (!m_items[i]) {
return static_cast<int>(i);
}
}
return -1;
}
int InventoryComponent::tryStackItem(Item* item, int quantity) {
if (!item || !item->isStackable()) {
return quantity;
}
int remaining = quantity;
for (auto& invItem : m_items) {
    if (!invItem || !invItem->item) continue;
    // Sprawdź czy można stackować (ta sama nazwa i typ)
    if (invItem->item->getDisplayName() == item->getDisplayName() &&
        invItem->item->getItemType() == item->getItemType()) {
        int maxStack = invItem->item->getMaxStackSize();
        int canAdd = maxStack - invItem->quantity;
        int toAdd = std::min(canAdd, remaining);
        invItem->quantity += toAdd;
        remaining -= toAdd;
        if (remaining == 0) {
            break;
        }
    }
}
return remaining;
}
