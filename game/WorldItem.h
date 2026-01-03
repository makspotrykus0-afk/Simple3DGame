#ifndef WORLD_ITEM_H
#define WORLD_ITEM_H

#include <memory>
#include "raylib.h"
#include "Item.h"

// Structure representing an item dropped in the world
class WorldItem {
public:
    Vector3 position;
    std::unique_ptr<Item> item;
    float dropTime;
    bool pendingRemoval = false;
    int amount = 1;

    // Reservation support
    bool m_isReserved = false;
    std::string m_reservedBy = "";

    WorldItem(Vector3 pos, std::unique_ptr<Item> it, float time, bool pending, int amt)
        : position(pos), item(std::move(it)), dropTime(time), pendingRemoval(pending), amount(amt) {}

    bool isReserved() const { return m_isReserved; }
    void reserve(const std::string& name) { m_isReserved = true; m_reservedBy = name; }
    void releaseReservation() { m_isReserved = false; m_reservedBy = ""; }
};

#endif // WORLD_ITEM_H
