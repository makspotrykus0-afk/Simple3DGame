#include "NeedComponent.h"
#include "../game/Settler.h"
#include "../game/BuildingInstance.h"
#include "../game/Colony.h"
#include "StatsComponent.h"
#include "raymath.h"
#include <iostream>
#include <cmath>

extern Colony* g_colony;

NeedComponent::NeedComponent(Settler* owner) : m_owner(owner) {
}

std::type_index NeedComponent::getComponentType() const {
    return std::type_index(typeid(NeedComponent));
}

void NeedComponent::update(float deltaTime) {
    if (!m_owner || m_owner->isPlayerControlled()) return;

    // Logika regeneracji (Well Bonus) - przeniesiona ze Settler.cpp
    if (g_colony && m_owner->getStats().getCurrentEnergy() < 100.0f) {
        if (g_colony->getEfficiencyModifier(m_owner->getPosition(), SettlerState::IDLE) > 1.05f) {
            m_owner->getStats().modifyEnergy(deltaTime * 2.0f);
        }
    }
}

void NeedComponent::updateCircadian(float deltaTime, float currentTime, const std::vector<BuildingInstance*>& buildings) {
    if (!m_owner || m_owner->isPlayerControlled()) return;

    // NIGHT LOGIC (22:00 - 06:00)
    bool isNight = (currentTime >= TIME_SLEEP) || (currentTime < TIME_WAKE_UP);
    
    if (isNight) {
        if (m_owner->getState() != SettlerState::SLEEPING && m_owner->getState() != SettlerState::MOVING_TO_BED) {
            // Force sleep if not doing critical survival or eating
            if (m_owner->getState() != SettlerState::EATING && 
                m_owner->getState() != SettlerState::MOVING_TO_FOOD && 
                m_owner->getState() != SettlerState::SEARCHING_FOR_FOOD) {
                
                 // Logika łóżka przeniesiona z Settler.cpp
                 // Wymaga dostępu do m_assignedBed (póki co przez metody Settlera)
                 // TODO: W 9.6 BedManager przejmie to zadanie
            }
        }
        return; 
    }

    // MORNING LOGIC (06:00 - 08:00)
    if (currentTime >= TIME_WAKE_UP && currentTime < TIME_WORK_START) {
        if (m_owner->getState() == SettlerState::SLEEPING) {
             m_owner->setState(SettlerState::IDLE); // Wake up
             m_hasGreetedMorning = false;
        }
        
        // Social stretch / Idle
        if (m_owner->getState() == SettlerState::IDLE && !m_hasGreetedMorning) {
             m_owner->setState(SettlerState::WAITING);
             // m_stretchTimer = 2.0f; 
             m_hasGreetedMorning = true;
        }
        return;
    }

    // EVENING LOGIC (18:00 - 22:00)
    if (currentTime >= TIME_WORK_END && currentTime < TIME_SLEEP) {
         // Stop working
         bool isWorking = (m_owner->getState() == SettlerState::CHOPPING || m_owner->getState() == SettlerState::MINING || 
                           m_owner->getState() == SettlerState::BUILDING || m_owner->getState() == SettlerState::CRAFTING ||
                           m_owner->getState() == SettlerState::GATHERING);
         
         if (isWorking) {
             m_owner->setState(SettlerState::IDLE); 
             m_owner->InterruptCurrentAction();
         }

         if (m_owner->getState() == SettlerState::IDLE || m_owner->getState() == SettlerState::WANDER) {
             BuildingInstance* socialSpot = findNearestSocialSpot(buildings);

             if (socialSpot) {
                 float dist = Vector3Distance(m_owner->getPosition(), socialSpot->getPosition());
                 if (dist > 5.0f) {
                     m_owner->setState(SettlerState::MOVING_TO_SOCIAL);
                     m_owner->MoveTo(socialSpot->getPosition());
                 } else {
                     m_owner->setState(SettlerState::SOCIAL);
                     // Look at fire
                     Vector3 dir = Vector3Subtract(socialSpot->getPosition(), m_owner->getPosition());
                     float angle = atan2f(dir.x, dir.z) * RAD2DEG;
                     m_owner->setRotation(angle);
                 }
             }
         }
    }
}

BuildingInstance* NeedComponent::findNearestSocialSpot(const std::vector<BuildingInstance*>& buildings) {
    BuildingInstance* nearest = nullptr;
    float minDist = 9999.0f;
    Vector3 myPos = m_owner->getPosition();

    for (auto* b : buildings) {
        if ((b->getBlueprintId() == "campfire" || b->getBlueprintId() == "taverna") && b->isBuilt()) {
            float d = Vector3Distance(myPos, b->getPosition());
            if (d < minDist) {
                minDist = d;
                nearest = b;
            }
        }
    }
    return nearest;
}
