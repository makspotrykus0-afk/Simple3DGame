#pragma once

#include "raylib.h"
#include "raymath.h"  // Dodano raymath.h
#include "../entities/GameEntity.h"
#include "../game/Item.h"
#include "../game/InteractableObject.h"
#include "../systems/SkillTypes.h"
#include <string>
#include <any>

// Enum for event types
enum class EventType {
    InteractionStarted,
    InteractionCompleted,
    Attack,
    EntityDeath,
    ItemPickedUp,
    ItemDropped,
    InventoryChanged,
    ResourceGathered,
    ItemCrafted,
    BuildingStarted,
    BuildingCompleted,
    BuildingDestroyed,
    ContainerOpened,
    ContainerClosed,
    InventoryCapacityExceeded,
    SkillLevelUp,
    SkillXPGained
};

// Base event struct
struct GameEvent {
    virtual ~GameEvent() = default;
    virtual EventType getType() const = 0;
};

// Zdarzenie rozpoczęcia interakcji
class InteractionStartedEvent : public GameEvent {
public:
    GameEntity* initiator;              // Kto inicjuje
    GameEntity* target;                 // Z kim/czym wchodzi w interakcję
    
    InteractionStartedEvent(GameEntity* init, GameEntity* tgt) 
        : initiator(init), target(tgt) {}
    
    EventType getType() const override { return EventType::InteractionStarted; }
};

// Zdarzenie zakończenia interakcji
class InteractionCompletedEvent : public GameEvent {
public:
    GameEntity* initiator;
    GameEntity* target;
    bool successful;                    // Czy interakcja się powiodła
    
    InteractionCompletedEvent(GameEntity* init, GameEntity* tgt, bool success)
        : initiator(init), target(tgt), successful(success) {}
        
    EventType getType() const override { return EventType::InteractionCompleted; }
};

// Zdarzenie ataku
class AttackEvent : public GameEvent {
public:
    GameEntity* attacker;               // Atakujący
    GameEntity* target;                 // Cel
    float damage;                       // Obrażenia
    
    AttackEvent(GameEntity* att, GameEntity* tgt, float dmg)
        : attacker(att), target(tgt), damage(dmg) {}
        
    EventType getType() const override { return EventType::Attack; }
};

// Zdarzenie śmierci
class EntityDeathEvent : public GameEvent {
public:
    GameEntity* victim;                 // Kto zginął
    GameEntity* killer;                 // Kto zabił (może być nullptr)
    
    EntityDeathEvent(GameEntity* v, GameEntity* k = nullptr)
        : victim(v), killer(k) {}
        
    EventType getType() const override { return EventType::EntityDeath; }
};

// Zdarzenie podniesienia przedmiotu
class ItemPickedUpEvent : public GameEvent {
public:
    GameEntity* picker;                 // Kto podniósł
    Item* item;                         // Co podniósł
    int quantity;                       // Ile sztuk
    std::string source;                 // Skąd (ziemia, skrzynia, itp.)
    Vector3 position;                   // Pozycja podniesienia
    
    ItemPickedUpEvent(GameEntity* p, Item* i, int qty, const std::string& src, Vector3 pos)
        : picker(p), item(i), quantity(qty), source(src), position(pos) {}
        
    EventType getType() const override { return EventType::ItemPickedUp; }
};

// Zdarzenie upuszczenia przedmiotu
class ItemDroppedEvent : public GameEvent {
public:
    GameEntity* dropper;                // Kto upuścił
    Item* item;                         // Co upuścił
    int quantity;                       // Ile sztuk
    Vector3 position;                   // Pozycja upuszczenia
    
    ItemDroppedEvent(GameEntity* d, Item* i, int qty, Vector3 pos)
        : dropper(d), item(i), quantity(qty), position(pos) {}
        
    EventType getType() const override { return EventType::ItemDropped; }
};

// Zdarzenie zmiany ekwipunku
class InventoryChangedEvent : public GameEvent {
public:
    GameEntity* owner;                  // Właściciel ekwipunku
    
    InventoryChangedEvent(GameEntity* o) : owner(o) {}
    
    EventType getType() const override { return EventType::InventoryChanged; }
};

// Zdarzenie zdobycia surowca
class ResourceGatheredEvent : public GameEvent {
public:
    GameEntity* gatherer;               // Kto zebrał
    std::string resourceType;           // Typ surowca
    int amount;                         // Ilość
    Vector3 location;                   // Gdzie zebrano
    float quality;                      // Jakość zasobu
    
    ResourceGatheredEvent(GameEntity* g, const std::string& type, int amt, Vector3 loc, float qual = 1.0f)
        : gatherer(g), resourceType(type), amount(amt), location(loc), quality(qual) {}
        
    EventType getType() const override { return EventType::ResourceGathered; }
};

// Zdarzenie wytworzenia przedmiotu
class ItemCraftedEvent : public GameEvent {
public:
    GameEntity* crafter;                // Kto wytworzył
    std::string itemId;                 // ID przedmiotu
    int quantity;                       // Ilość
    
    ItemCraftedEvent(GameEntity* c, const std::string& id, int qty)
        : crafter(c), itemId(id), quantity(qty) {}
        
    EventType getType() const override { return EventType::ItemCrafted; }
};

// Zdarzenie rozpoczęcia budowy
class BuildingStartedEvent : public GameEvent {
public:
    GameEntity* builder;                // Kto buduje
    std::string blueprintId;            // Co buduje
    Vector3 position;                   // Pozycja budowy
    float estimatedTime;                // Szacowany czas
    
    BuildingStartedEvent(GameEntity* b, const std::string& id, Vector3 pos, float time)
        : builder(b), blueprintId(id), position(pos), estimatedTime(time) {}
        
    EventType getType() const override { return EventType::BuildingStarted; }
};

// Zdarzenie zakończenia budowy
class BuildingCompletedEvent : public GameEvent {
public:
    GameEntity* building;               // Zbudowany obiekt
    GameEntity* builder;                // Kto zbudował (może być nullptr)
    
    BuildingCompletedEvent(GameEntity* b, GameEntity* w = nullptr)
        : building(b), builder(w) {}
        
    EventType getType() const override { return EventType::BuildingCompleted; }
};

/**
 * @brief Zdarzenie zniszczenia budynku
 */
class BuildingDestroyedEvent : public GameEvent {
public:
    GameEntity* building;               // Zniszczony budynek
    GameEntity* destroyer;              // Niszczyciel (może być nullptr)
    std::string cause;                  // Przyczyna zniszczenia
    Vector3 position;                   // Pozycja budynku
    
    BuildingDestroyedEvent(GameEntity* bld, GameEntity* dest, 
                          const std::string& c, Vector3 pos)
        : building(bld), destroyer(dest), cause(c), position(pos) {}
        
    EventType getType() const override { return EventType::BuildingDestroyed; }
};

/**
 * @brief Zdarzenie przekroczenia pojemności ekwipunku
 */
class InventoryCapacityExceededEvent : public GameEvent {
public:
    GameEntity* owner;                  // Właściciel ekwipunku
    float currentWeight;                // Aktualna waga
    float maxCapacity;                  // Maksymalna pojemność
    Item* attemptedItem;                // Przedmiot który próbowano dodać
    
    InventoryCapacityExceededEvent(GameEntity* o, float curr, float max, Item* item)
        : owner(o), currentWeight(curr), maxCapacity(max), attemptedItem(item) {}
        
    EventType getType() const override { return EventType::InventoryCapacityExceeded; }
};

/**
 * @brief Zdarzenie otwarcia kontenera
 */
class ContainerOpenedEvent : public GameEvent {
public:
    GameEntity* opener;                 // Encja otwierająca
    InteractableObject* container;      // Kontener
    Vector3 position;                   // Pozycja kontenera
    
    ContainerOpenedEvent(GameEntity* o, InteractableObject* c, Vector3 pos)
        : opener(o), container(c), position(pos) {}
        
    EventType getType() const override { return EventType::ContainerOpened; }
};

/**
 * @brief Zdarzenie zamknięcia kontenera
 */
class ContainerClosedEvent : public GameEvent {
public:
    GameEntity* closer;                 // Encja zamykająca
    InteractableObject* container;      // Kontener
    Vector3 position;                   // Pozycja kontenera
    
    ContainerClosedEvent(GameEntity* c, InteractableObject* cont, Vector3 pos)
        : closer(c), container(cont), position(pos) {}
        
    EventType getType() const override { return EventType::ContainerClosed; }
};

// Zdarzenie awansu na wyższy poziom umiejętności
class SkillLevelUpEvent : public GameEvent {
public:
    GameEntity* entity;
    SkillType skillType;
    int newLevel;

    SkillLevelUpEvent(GameEntity* e, SkillType type, int level)
        : entity(e), skillType(type), newLevel(level) {}

    EventType getType() const override { return EventType::SkillLevelUp; }
};

// Zdarzenie zdobycia doświadczenia
class SkillXPGainedEvent : public GameEvent {
public:
    GameEntity* entity;
    SkillType skillType;
    float xpGained;

    SkillXPGainedEvent(GameEntity* e, SkillType type, float xp)
        : entity(e), skillType(type), xpGained(xp) {}

    EventType getType() const override { return EventType::SkillXPGained; }
};
