#include "StorageSystem.h"
#include "../core/EventSystem.h" // ADDED
#include <algorithm>
#include <cmath>
#include <iostream>

// ==========================================
// StorageSlot Implementation
// ==========================================

bool StorageSlot::canAddResource(
    Resources::ResourceType type, int32_t amountToCheck,
    const Resources::Resource *resourceInfo) const {
  if (!isOccupied) {
    return true;
  }

  if (resourceType != type) {
    return false;
  }

  int32_t maxStack = getMaxCapacity(resourceInfo);
  return (amount + amountToCheck) <= maxStack;
}

int32_t
StorageSlot::getMaxCapacity(const Resources::Resource *resourceInfo) const {
  if (!resourceInfo) {
    return 99; // Fallback default
  }
  return resourceInfo->maxStack;
}

// ==========================================
// StorageInstance Implementation
// ==========================================

float StorageSystem::StorageInstance::getCapacityUsage() const {
  if (slots.empty())
    return 0.0f;

  int occupied = 0;
  for (const auto &slot : slots) {
    if (slot.isOccupied)
      occupied++;
  }
  return static_cast<float>(occupied) / slots.size();
}

bool StorageSystem::StorageInstance::canAddResource(
    Resources::ResourceType type, int32_t amount,
    const Resources::Resource *resourceInfo) const {
  int32_t remainingAmount = amount;

  for (const auto &slot : slots) {
    if (slot.isOccupied && slot.resourceType == type) {
      int32_t maxStack = slot.getMaxCapacity(resourceInfo);
      int32_t space = maxStack - slot.amount;
      if (space > 0) {
        remainingAmount -= space;
        if (remainingAmount <= 0)
          return true;
      }
    }
  }

  int32_t neededSlots = 0;
  int32_t maxStack = resourceInfo ? resourceInfo->maxStack : 99;

  if (remainingAmount > 0) {
    neededSlots = (remainingAmount + maxStack - 1) / maxStack;
  }

  return getFreeSlots() >= static_cast<uint32_t>(neededSlots);
}

uint32_t StorageSystem::StorageInstance::getFreeSlots() const {
  uint32_t count = 0;
  for (const auto &slot : slots) {
    if (!slot.isOccupied)
      count++;
  }
  return count;
}

int32_t StorageSystem::StorageInstance::getMaxCapacityForResource(
    Resources::ResourceType type,
    const Resources::Resource *resourceInfo) const {
  int32_t totalCapacity = 0;
  int32_t maxStack = resourceInfo ? resourceInfo->maxStack : 99;

  for (const auto &slot : slots) {
    if (!slot.isOccupied) {
      totalCapacity += maxStack;
    } else if (slot.resourceType == type) {
      totalCapacity += (maxStack - slot.amount);
    }
  }

  return totalCapacity;
}

// ==========================================
// StorageSlotPool Implementation
// ==========================================

std::unique_ptr<StorageSlot> StorageSystem::StorageSlotPool::acquireSlot() {
  auto &pool = getPool();
  if (pool.empty()) {
    return std::make_unique<StorageSlot>();
  }

  auto slot = std::move(pool.back());
  pool.pop_back();

  slot->isOccupied = false;
  slot->amount = 0;
  slot->resourceType = Resources::ResourceType::Wood;

  return slot;
}

void StorageSystem::StorageSlotPool::releaseSlot(
    std::unique_ptr<StorageSlot> slot) {
  if (slot) {
    slot->isOccupied = false;
    slot->amount = 0;
    getPool().push_back(std::move(slot));
  }
}

void StorageSystem::StorageSlotPool::clearPool() { getPool().clear(); }

size_t StorageSystem::StorageSlotPool::getPoolSize() {
  return getPool().size();
}

// ==========================================
// StorageSystem Implementation
// ==========================================

StorageSystem::StorageSystem()
    : m_name("StorageSystem"), m_nextStorageId(1), m_cacheHits(0),
      m_cacheMisses(0) {}

StorageSystem::~StorageSystem() { shutdown(); }

void StorageSystem::initialize() {
  initializeBaseStorageTypes();
  std::cout << "StorageSystem initialized with " << m_storageTypes.size()
            << " base types." << std::endl;
}

void StorageSystem::update(float deltaTime) { m_cache.cleanup(); }

void StorageSystem::render() {
  // No visual rendering
}

void StorageSystem::shutdown() {
  m_storages.clear();
  m_playerStorages.clear();
  StorageSlotPool::clearPool();
  std::cout << "StorageSystem shutdown." << std::endl;
}

bool StorageSystem::registerStorageType(const StorageConfig &config) {
  if (m_storageTypes.find(config.type) != m_storageTypes.end()) {
    return false;
  }
  m_storageTypes.emplace(config.type, config);
  return true;
}

const StorageSystem::StorageConfig *
StorageSystem::getStorageConfig(StorageType type) const {
  auto it = m_storageTypes.find(type);
  if (it != m_storageTypes.end()) {
    return &it->second;
  }
  return nullptr;
}

std::string StorageSystem::createStorage(StorageType type,
                                         const std::string &ownerId) {
  auto it = m_storageTypes.find(type);
  if (it == m_storageTypes.end()) {
    return "";
  }

  StorageInstance instance =
      StorageFactory::createStorageInstance(type, ownerId);
  instance.id = "storage_" + std::to_string(m_nextStorageId++);

  const auto &config = it->second;
  instance.slots.reserve(config.maxSlots);
  for (uint32_t i = 0; i < config.maxSlots; ++i) {
    instance.slots.emplace_back();
  }

  std::string id = instance.id;
  m_storages.emplace(id, std::move(instance));
  m_playerStorages[ownerId].push_back(id);

  return id;
}

bool StorageSystem::deleteStorage(const std::string &storageId) {
  auto it = m_storages.find(storageId);
  if (it == m_storages.end()) {
    return false;
  }

  std::string ownerId = it->second.ownerId;
  m_storages.erase(it);

  auto &playerList = m_playerStorages[ownerId];
  auto pit = std::find(playerList.begin(), playerList.end(), storageId);
  if (pit != playerList.end()) {
    playerList.erase(pit);
  }

  return true;
}

int32_t StorageSystem::addResourceToStorage(
    const std::string &storageId, const std::string &playerId,
    Resources::ResourceType resourceType, int32_t amount) {
  auto storageIt = m_storages.find(storageId);
  if (storageIt == m_storages.end()) {
    return 0;
  }

  StorageInstance &storage = storageIt->second;
  const StorageConfig *config = getStorageConfig(storage.type);
  if (!config) {
    std::cout << "StorageSystem: Config not found for type "
              << (int)storage.type << std::endl;
    return 0;
  }

  if (config->requiresAuthentication && storage.ownerId != playerId &&
      !config->isShared) {
    return 0;
  }

  const Resources::Resource *resourceInfo = nullptr;

  // Debug logging for addResourceToStorage
  std::cout << "[StorageSystem] Adding " << amount << " of type "
            << (int)resourceType << " to " << storageId << " by " << playerId
            << std::endl;

  int32_t remaining = amount;
  int32_t addedTotal = 0;

  if (config->isStackable) {
    for (size_t i = 0; i < storage.slots.size(); ++i) {
      if (remaining <= 0)
        break;

      StorageSlot &slot = storage.slots[i];
      if (slot.isOccupied && slot.resourceType == resourceType) {
        int32_t added =
            addResourceToSlot(storage, static_cast<int32_t>(i), resourceType,
                              remaining, resourceInfo);
        remaining -= added;
        addedTotal += added;
      }
    }
  }

  if (remaining > 0) {
    for (size_t i = 0; i < storage.slots.size(); ++i) {
      if (remaining <= 0)
        break;

      StorageSlot &slot = storage.slots[i];
      if (!slot.isOccupied) {
        int32_t added =
            addResourceToSlot(storage, static_cast<int32_t>(i), resourceType,
                              remaining, resourceInfo);
        remaining -= added;
        addedTotal += added;
      }
    }
  }

  if (addedTotal > 0) {
    updateStorageState(storage);

    ItemAddedToStorageEvent event;
    event.storageId = storageId;
    event.resourceType = resourceType;
    event.amount = addedTotal;
    event.playerId = playerId;
    event.amount = addedTotal;
    event.playerId = playerId;
    EventBus::send(event);
  }

  if (addedTotal == 0) {
    // Failed to add resource (Full or no suitable slots)
    std::cout << "[StorageSystem] Failed to add any resource. Storage might be "
                 "full or incompatible."
              << std::endl;
  } else {
    std::cout << "[StorageSystem] Successfully added " << addedTotal
              << " items." << std::endl;
  }
  return addedTotal;
}

int32_t StorageSystem::removeResourceFromStorage(
    const std::string &storageId, const std::string &playerId,
    Resources::ResourceType resourceType, int32_t amount) {
  auto storageIt = m_storages.find(storageId);
  if (storageIt == m_storages.end()) {
    return 0;
  }

  StorageInstance &storage = storageIt->second;

  int32_t remaining = amount;
  int32_t removedTotal = 0;

  for (int i = static_cast<int>(storage.slots.size()) - 1; i >= 0; --i) {
    if (remaining <= 0)
      break;

    StorageSlot &slot = storage.slots[i];
    if (slot.isOccupied && slot.resourceType == resourceType) {
      int32_t removed = removeResourceFromSlot(storage, i, remaining);
      remaining -= removed;
      removedTotal += removed;
    }
  }

  if (removedTotal > 0) {
    updateStorageState(storage);

    ItemRemovedFromStorageEvent event;
    event.storageId = storageId;
    event.resourceType = resourceType;
    event.amount = removedTotal;
    event.playerId = playerId;
    event.amount = removedTotal;
    event.playerId = playerId;
    EventBus::send(event);
  }

  return removedTotal;
}

int32_t
StorageSystem::getResourceAmount(const std::string &storageId,
                                 Resources::ResourceType resourceType) const {
  auto it = m_storages.find(storageId);
  if (it == m_storages.end())
    return 0;

  auto &instance = it->second;
  auto cacheIt = instance.totalAmounts.find(resourceType);
  if (cacheIt != instance.totalAmounts.end()) {
    m_cacheHits++;
    return cacheIt->second;
  }

  m_cacheMisses++;
  int32_t amount = 0;
  for (const auto &slot : instance.slots) {
    if (slot.isOccupied && slot.resourceType == resourceType) {
      amount += slot.amount;
    }
  }
  return amount;
}

std::unordered_map<Resources::ResourceType, int32_t>
StorageSystem::getAllResources(const std::string &storageId) const {
  std::unordered_map<Resources::ResourceType, int32_t> resources;
  auto it = m_storages.find(storageId);
  if (it == m_storages.end())
    return resources;

  for (const auto &slot : it->second.slots) {
    if (slot.isOccupied) {
      resources[slot.resourceType] += slot.amount;
    }
  }
  return resources;
}

bool StorageSystem::canAddResource(const std::string &storageId,
                                   Resources::ResourceType resourceType,
                                   int32_t amount) const {
  auto it = m_storages.find(storageId);
  if (it == m_storages.end())
    return false;

  return it->second.canAddResource(resourceType, amount, nullptr);
}

// Implement addItemToStorage
bool StorageSystem::addItemToStorage(const std::string &storageId,
                                     const Item &item, int amount) {
  // Map items to resource types based on name or type
  // This is a temporary solution until we fully integrate Item system with
  // Resource system
  Resources::ResourceType type = Resources::ResourceType::None;
  std::string name = item.getDisplayName();

  if (item.getItemType() == ItemType::RESOURCE) {
    if (name == "Wood")
      type = Resources::ResourceType::Wood;
    else if (name == "Stone")
      type = Resources::ResourceType::Stone;
    else if (name == "Metal")
      type = Resources::ResourceType::Metal;
    else if (name == "Gold")
      type = Resources::ResourceType::Gold;
    else if (name == "Food" || name == "Raw Meat")
      type = Resources::ResourceType::Food;
  } else if (item.getItemType() == ItemType::CONSUMABLE) {
    if (name == "Raw Meat" || name == "Cooked Meat" || name == "Berry")
      type = Resources::ResourceType::Food;
    else if (name == "Water")
      type = Resources::ResourceType::Water;
  }

  // Also check direct string mapping if enum check failed but name matches
  // known resources
  if (type == Resources::ResourceType::None) {
    if (name == "Wood")
      type = Resources::ResourceType::Wood;
    else if (name == "Stone")
      type = Resources::ResourceType::Stone;
    else if (name == "Food")
      type = Resources::ResourceType::Food;
    else if (name == "Metal")
      type = Resources::ResourceType::Metal;
  }

  if (type != Resources::ResourceType::None) {
    int32_t added = addResourceToStorage(storageId, "system", type, amount);
    return added > 0;
  } else {
    std::cout << "StorageSystem: Cannot store item " << item.getDisplayName()
              << " (Type not mapped)" << std::endl;
    return false;
  }
}

StorageSystem::StorageInstance *
StorageSystem::getStorage(const std::string &storageId) const {
  auto it = m_storages.find(storageId);
  if (it != m_storages.end()) {
    return const_cast<StorageInstance *>(&it->second);
  }
  return nullptr;
}

std::vector<std::string>
StorageSystem::getPlayerStorages(const std::string &playerId) const {
  auto it = m_playerStorages.find(playerId);
  if (it != m_playerStorages.end()) {
    return it->second;
  }
  return {};
}

bool StorageSystem::optimizeStorageLayout(const std::string &storageId) {
  auto it = m_storages.find(storageId);
  if (it == m_storages.end())
    return false;

  StorageInstance &storage = it->second;
  const StorageConfig *config = getStorageConfig(storage.type);
  if (!config || !config->isStackable)
    return false;

  bool changed = false;

  for (size_t i = 0; i < storage.slots.size(); ++i) {
    if (!storage.slots[i].isOccupied)
      continue;

    for (size_t j = i + 1; j < storage.slots.size(); ++j) {
      if (storage.slots[j].isOccupied &&
          storage.slots[j].resourceType == storage.slots[i].resourceType) {

        int32_t space =
            storage.slots[i].getMaxCapacity(nullptr) - storage.slots[i].amount;
        if (space > 0) {
          int32_t toMove = std::min(space, storage.slots[j].amount);
          storage.slots[i].amount += toMove;
          storage.slots[j].amount -= toMove;
          if (storage.slots[j].amount == 0) {
            storage.slots[j].isOccupied = false;
          }
          changed = true;
        }
      }
    }
  }

  if (changed) {
    updateStorageState(storage);
  }

  return changed;
}

void StorageSystem::lazyLoadStorage(const std::string &storageId) {
  // Placeholder
}

StorageSystem::StorageSystemStats StorageSystem::getStats() const {
  StorageSystemStats stats;
  stats.totalStorageTypes = m_storageTypes.size();
  stats.totalStorages = m_storages.size();
  stats.cacheHits = m_cacheHits;
  stats.cacheMisses = m_cacheMisses;

  stats.totalSlots = 0;
  stats.usedSlots = 0;
  float totalCapUsage = 0.0f;

  for (const auto &pair : m_storages) {
    stats.totalSlots += pair.second.slots.size();
    stats.usedSlots += (pair.second.slots.size() - pair.second.getFreeSlots());
    totalCapUsage += pair.second.getCapacityUsage();
  }

  stats.averageCapacityUsage =
      stats.totalStorages > 0 ? totalCapUsage / stats.totalStorages : 0.0f;

  return stats;
}

void StorageSystem::initializeBaseStorageTypes() {
  auto types = StorageFactory::createBaseStorageTypes();
  for (const auto &pair : types) {
    m_storageTypes[pair.first] = pair.second;
  }
}

void StorageSystem::updateStorageState(StorageInstance &storage) {
  storage.totalAmounts.clear();

  bool full = true;
  bool empty = true;

  for (const auto &slot : storage.slots) {
    if (slot.isOccupied) {
      storage.totalAmounts[slot.resourceType] += slot.amount;
      empty = false;
    } else {
      full = false;
    }
  }

  StorageState oldState = storage.state;
  if (empty)
    storage.state = StorageState::EMPTY;
  else if (full)
    storage.state = StorageState::FULL;
  else
    storage.state = StorageState::PARTIAL;

  if (oldState != storage.state) {
    StorageStateChangedEvent event;
    event.storageId = storage.id;
    event.storageType = storage.type;
    event.oldState = oldState;
    event.newState = storage.state;
    event.capacityUsage = storage.getCapacityUsage();
    event.playerId = storage.ownerId;
    // notifyStorageEvent(event);
  }
}

int32_t StorageSystem::findSlotForResource(
    StorageInstance &storage, Resources::ResourceType resourceType,
    const Resources::Resource *resourceInfo) const {
  for (size_t i = 0; i < storage.slots.size(); ++i) {
    if (!storage.slots[i].isOccupied)
      return static_cast<int32_t>(i);
  }
  return -1;
}

int32_t
StorageSystem::addResourceToSlot(StorageInstance &storage, int32_t slotIndex,
                                 Resources::ResourceType resourceType,
                                 int32_t amount,
                                 const Resources::Resource *resourceInfo) {
  if (slotIndex < 0 || slotIndex >= static_cast<int32_t>(storage.slots.size()))
    return 0;

  StorageSlot &slot = storage.slots[slotIndex];

  if (slot.isOccupied && slot.resourceType != resourceType)
    return 0;

  if (!slot.isOccupied) {
    slot.isOccupied = true;
    slot.resourceType = resourceType;
    slot.amount = 0;
  }

  int32_t maxCap = slot.getMaxCapacity(resourceInfo);
  int32_t canAdd = maxCap - slot.amount;
  int32_t toAdd = std::min(canAdd, amount);

  std::cout << "[StorageSystem] Slot " << slotIndex
            << ": Current=" << slot.amount << " Max=" << maxCap
            << " Adding=" << toAdd << std::endl;

  slot.amount += toAdd;
  return toAdd;
}

int32_t StorageSystem::removeResourceFromSlot(StorageInstance &storage,
                                              int32_t slotIndex,
                                              int32_t amount) {
  if (slotIndex < 0 || slotIndex >= static_cast<int32_t>(storage.slots.size()))
    return 0;

  StorageSlot &slot = storage.slots[slotIndex];
  if (!slot.isOccupied)
    return 0;

  int32_t toRemove = std::min(slot.amount, amount);
  slot.amount -= toRemove;

  if (slot.amount <= 0) {
    slot.isOccupied = false;
    slot.amount = 0;
  }

  return toRemove;
}
