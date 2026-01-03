#ifndef NEED_COMPONENT_H
#define NEED_COMPONENT_H

#include "../core/IComponent.h"
#include "raylib.h"
#include <string>
#include <vector>
#include <typeindex>
#include <typeinfo>

// Forward declarations
class Settler;
class BuildingInstance;
class StatsComponent;

/**
 * @class NeedComponent
 * @brief Komponent odpowiedzialny za logikę potrzeb fizjologicznych (głód, sen, energia)
 * oraz obsługę rytmu dobowego (Circadian Rhythm).
 * Protokół Masrerpiece 9.4: "Duszny Kernel"
 */
class NeedComponent : public IComponent {
public:
    NeedComponent(Settler* owner);
    virtual ~NeedComponent() = default;

    void update(float deltaTime) override;
    void render() override {}
    void initialize() override {}
    void shutdown() override {}
    std::type_index getComponentType() const override;
    
    // Circadian Rhythm Logic
    void updateCircadian(float deltaTime, float currentTime, const std::vector<BuildingInstance*>& buildings);

    // Getters / Setters dla progów
    float getHungerThreshold() const { return m_hungerThreshold; }
    float getSleepThreshold() const { return m_sleepThreshold; }

private:
    Settler* m_owner;
    
    // Progi detekcji potrzeb
    float m_hungerThreshold = 40.0f;
    float m_sleepThreshold = 30.0f;
    float m_hungerExitThreshold = 80.0f;
    float m_sleepExitThreshold = 80.0f;

    // Stałe czasowe (Synchronizowane z DNA 9.4)
    const float TIME_WAKE_UP = 6.0f;
    const float TIME_WORK_START = 8.0f;
    const float TIME_WORK_END = 18.0f;
    const float TIME_SLEEP = 22.0f;

    // Timery i flagi wewnętrzne
    float m_socialTimer = 0.0f;
    float m_stretchTimer = 0.0f;
    bool m_hasGreetedMorning = false;

    // Helper: Znajdowanie punktów socjalnych
    BuildingInstance* findNearestSocialSpot(const std::vector<BuildingInstance*>& buildings);
};

#endif // NEED_COMPONENT_H
