
#include "InteractionSystem.h"
#include "../core/GameEngine.h"
#include "../game/Animal.h"
#include "../game/Colony.h"
#include "../game/Door.h"
#include "../game/Tree.h"
#include "../systems/BuildingSystem.h"
#include "../systems/StorageSystem.h"
#include "../systems/UISystem.h"
#include <iostream>

extern Colony colony;
InteractionSystem::InteractionSystem()
    : m_playerEntity(nullptr), m_interactionRange(10.0f),
      m_currentTarget(nullptr), m_inputEnabled(true),
      m_lastInteractionTime(0.0f), m_uiVisible(false),
      m_wasInteractionHandled(false), m_chopTimer(0.0f), m_isChopping(false),
      m_activeChopTarget(nullptr), m_movementTarget({0.0f, 0.0f, 0.0f}),
      m_isMoving(false), m_movementSpeed(5.0f), m_activationDistance(2.0f) {}
void InteractionSystem::beginFrame() { m_wasInteractionHandled = false; }
void InteractionSystem::initialize() {
  // Nothing to init for now
}
void InteractionSystem::update(float deltaTime) {
  (void)deltaTime;
  for (auto *obj : m_interactableObjects) {
    if (obj && obj->isActive()) {
      obj->update(deltaTime);
    }
  }
}
void InteractionSystem::render() {
  if (m_activeChopTarget) {
    DrawCubeWires(m_activeChopTarget->getPosition(), 1.2f, 1.2f, 1.2f, RED);
  }
}
void InteractionSystem::shutdown() {
  // Cleanup
}
void InteractionSystem::registerInteractableObject(InteractableObject *obj) {
  m_interactableObjects.push_back(obj);
}
void InteractionSystem::unregisterInteractableObject(InteractableObject *obj) {
  for (auto it = m_interactableObjects.begin();
       it != m_interactableObjects.end(); ++it) {
    if (*it == obj) {
      m_interactableObjects.erase(it);
      break;
    }
  }
}
void InteractionSystem::processPlayerInput(Camera camera) {
  UISystem *uiSystem = GameEngine::getInstance().getSystem<UISystem>();
  bool mouseOverUI = uiSystem && uiSystem->IsMouseOverUI();
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    if (mouseOverUI) {
      return;
    }
    Vector2 mousePosition = GetMousePosition();
    Ray ray = GetMouseRay(mousePosition, camera);
    handleInput(ray);
  }
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (mouseOverUI) {
      return;
    }
    Vector2 mousePosition = GetMousePosition();
    Ray ray = GetMouseRay(mousePosition, camera);
    handleSelection(ray);
  }
}
bool InteractionSystem::handleSelection(Ray ray) {
  if (!m_colony)
    return false;
  float minHitDist = 9999.0f;
  Settler *hitSettler = nullptr;
  for (auto *settler : m_colony->getSettlers()) {
    if (!settler)
      continue;
    Vector3 sPos = settler->getPosition();
    RayCollision collision =
        GetRayCollisionSphere(ray, {sPos.x, sPos.y + 1.0f, sPos.z}, 1.0f);
    if (collision.hit) {
      if (collision.distance < minHitDist) {
        minHitDist = collision.distance;
        hitSettler = settler;
      }
    }
  }
  if (hitSettler) {
    if (!IsKeyDown(KEY_LEFT_SHIFT)) {
      m_colony->clearSelection();
    }
    hitSettler->SetSelected(true);
    return true;
  } else {
    if (!IsKeyDown(KEY_LEFT_SHIFT)) {
      m_colony->clearSelection();
    }
    return false;
  }
}
bool InteractionSystem::handleInput(Ray ray) {
  float minHitDist = 9999.0f;
  InteractableObject *hitObj = nullptr;
  for (auto *obj : m_interactableObjects) {
    if (!obj || !obj->isActive())
      continue;
    Tree *tree = dynamic_cast<Tree *>(obj);
    if (tree) {
      BoundingBox bbox = tree->getBoundingBox();
      RayCollision collision = GetRayCollisionBox(ray, bbox);
      if (collision.hit) {
        if (collision.distance < minHitDist) {
          minHitDist = collision.distance;
          hitObj = obj;
        }
      }
      continue;
    }
    Vector3 pos = obj->getPosition();
    Vector3 center = {pos.x, pos.y + 1.0f, pos.z};
    RayCollision collision = GetRayCollisionSphere(ray, center, 1.0f);
    if (collision.hit) {
      if (collision.distance < minHitDist) {
        minHitDist = collision.distance;
        hitObj = obj;
      }
    }
  }
  // Determine which colony pointer to use (prefer m_colony, fallback to global
  // extern)
  Colony *colonyPtr = m_colony;
  if (!colonyPtr) {
    extern Colony colony;
    colonyPtr = &colony;
  }
  const auto &animals = colonyPtr->getAnimals();
  for (const auto &animal : animals) {
    if (!animal || !animal->isActive())
      continue;
    Vector3 pos = animal->getPosition();
    Vector3 center = {pos.x, pos.y + 0.5f, pos.z};
    RayCollision collision = GetRayCollisionSphere(ray, center, 1.0f);
    if (collision.hit) {
      if (collision.distance < minHitDist) {
        minHitDist = collision.distance;
        hitObj = animal.get();
      }
    }
  }
  if (hitObj) {
    m_activeChopTarget = hitObj;
    m_wasInteractionHandled = true;
    auto selectedSettlers = colonyPtr->getSelectedSettlers();
    Tree *hitTree = dynamic_cast<Tree *>(hitObj);
    if (hitTree) {
      // Find closest settler among selected
      Settler *bestSettler = nullptr;
      float bestDist = 999999.0f;
      for (auto *settler : selectedSettlers) {
        float dist =
            Vector3Distance(settler->getPosition(), hitTree->getPosition());
        if (dist < bestDist) {
          bestDist = dist;
          bestSettler = settler;
        }
      }

      if (bestSettler) {
        bestSettler->ForceGatherTarget(hitTree);
        std::cout << "Interaction: Tree selected at "
                  << hitTree->getPosition().x
                  << ". Assigned to closest settler: " << bestSettler->getName()
                  << std::endl;
      } else {
        std::cout << "Interaction: Tree selected but no settler selected."
                  << std::endl;
      }
      return true;
    }
    Door *hitDoor = dynamic_cast<Door *>(hitObj);
    if (hitDoor) {
      hitDoor->interact(nullptr);
      return true;
    }
    Animal *hitAnimal = dynamic_cast<Animal *>(hitObj);
    if (hitAnimal) {
      for (auto *settler : selectedSettlers) {
        (void)settler;
        hitAnimal->interact(nullptr);
      }
      return true;
    }
    return true;
  }

  // Fallback: Ground Plane Click (Movement)
  // Plane Y=0 normal=(0,1,0)
  // Ray origin O, dir D. P = O + tD. P.y = 0 => O.y + t*D.y = 0 => t = -O.y / D.y
  if (fabs(ray.direction.y) > 0.001f) {
      float t = -ray.position.y / ray.direction.y;
      if (t >= 0.0f) {
          Vector3 hitPoint = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
          
          // Move selected settlers
          Colony *colonyPtr = m_colony;
          if (!colonyPtr) {
            extern Colony colony;
            colonyPtr = &colony;
          }
          auto selectedSettlers = colonyPtr->getSelectedSettlers();
          if (!selectedSettlers.empty()) {
              std::cout << "Interaction: Ground Click at (" << hitPoint.x << ", " << hitPoint.z << "). Moving " << selectedSettlers.size() << " settlers." << std::endl;
              
              // Basic formation or group movement could be added here
              // For now, just send all to the point (NavigationGrid handles pathfinding)
              for (auto* s : selectedSettlers) {
                  // Clear current tasks?
                  // s->ClearTasks(); // If we had a method exposed
                  // Just standard MoveTo
                   s->MoveTo(hitPoint);
              }
              m_wasInteractionHandled = true;
              return true;
          }
      }
  }

  BuildingSystem *buildingSystem = m_buildingSystem;
  if (!buildingSystem) {
    buildingSystem = GameEngine::getInstance().getSystem<BuildingSystem>();
  }
  RayCollision groundHit =
      GetRayCollisionQuad(ray, {-1000, 0, -1000}, {-1000, 0, 1000},
                          {1000, 0, 1000}, {1000, 0, -1000});
  if (groundHit.hit && buildingSystem) {
    BuildTask *nearbyTask =
        buildingSystem->getBuildTaskAt(groundHit.point, 2.0f);
    if (nearbyTask && nearbyTask->isActive()) {
      auto selectedSettlers = colonyPtr->getSelectedSettlers();
      for (auto *settler : selectedSettlers) {
        settler->AssignBuildTask(nearbyTask);
      }
      if (!selectedSettlers.empty()) {
        m_wasInteractionHandled = true;
        return true;
      }
    }
  }
  // Use storage registry from colony instead of scanning all buildings
  if (colonyPtr) {
    const auto &storageBuildings = colonyPtr->getStorageBuildings();
    for (auto *building : storageBuildings) {
      if (!building || building->getStorageId().empty())
        continue;
      BoundingBox bbox = building->getBoundingBox();
      RayCollision collision = GetRayCollisionBox(ray, bbox);
      if (collision.hit && collision.distance < minHitDist) {
        minHitDist = collision.distance;
        auto selectedSettlers = colonyPtr->getSelectedSettlers();
        if (!selectedSettlers.empty()) {
          StorageSystem *storageSys =
              GameEngine::getInstance().getSystem<StorageSystem>();
          if (storageSys) {
            bool anyAction = false;
            for (auto *settler : selectedSettlers) {
              float dist = Vector3Distance(settler->getPosition(),
                                           building->getPosition());
              if (dist > 5.0f) {
                settler->MoveTo(building->getPosition());
                std::cout << "[InteractionSystem] Settler moving to storage to "
                             "deposit."
                          << std::endl;
                anyAction = true;
                continue;
              }
              auto &inventory = settler->getInventory();
              const auto &items = inventory.getItems();
              int totalDeposited = 0;
              for (int i = (int)items.size() - 1; i >= 0; --i) {
                if (items[i] && items[i]->item &&
                    items[i]->item->getDisplayName() == "Wood Log") {
                  int quantity = items[i]->quantity;
                  int32_t added = storageSys->addResourceToStorage(
                      building->getStorageId(), settler->getName(),
                      Resources::ResourceType::Wood, quantity);
                  if (added > 0) {
                    inventory.extractItem(i, added);
                    totalDeposited += added;
                  }
                }
              }
              if (totalDeposited > 0) {
                std::cout << "[InteractionSystem] Deposited " << totalDeposited
                          << " logs to " << building->getStorageId()
                          << std::endl;
                anyAction = true;
              } else if (dist <= 5.0f) {
                std::cout
                    << "[InteractionSystem] No logs to deposit or storage full."
                    << std::endl;
              }
            }
            if (anyAction) {
              m_wasInteractionHandled = true;
              return true;
            }
          }
        }
      }
    }
  } else {
    // Fallback to old scanning logic if colony pointer is not available (should
    // not happen)
    BuildingSystem *buildingSystemDeposit =
        GameEngine::getInstance().getSystem<BuildingSystem>();
    if (buildingSystemDeposit) {
      for (auto *building : buildingSystemDeposit->getAllBuildings()) {
        if (!building || building->getStorageId().empty())
          continue;
        BoundingBox bbox = building->getBoundingBox();
        RayCollision collision = GetRayCollisionBox(ray, bbox);
        if (collision.hit && collision.distance < minHitDist) {
          minHitDist = collision.distance;
          extern Colony colony;
          auto selectedSettlers = colony.getSelectedSettlers();
          if (!selectedSettlers.empty()) {
            StorageSystem *storageSys =
                GameEngine::getInstance().getSystem<StorageSystem>();
            if (storageSys) {
              bool anyAction = false;
              for (auto *settler : selectedSettlers) {
                float dist = Vector3Distance(settler->getPosition(),
                                             building->getPosition());
                if (dist > 5.0f) {
                  settler->MoveTo(building->getPosition());
                  std::cout << "[InteractionSystem] Settler moving to storage "
                               "to deposit."
                            << std::endl;
                  anyAction = true;
                  continue;
                }
                auto &inventory = settler->getInventory();
                const auto &items = inventory.getItems();
                int totalDeposited = 0;
                for (int i = (int)items.size() - 1; i >= 0; --i) {
                  if (items[i] && items[i]->item &&
                      items[i]->item->getDisplayName() == "Wood Log") {
                    int quantity = items[i]->quantity;
                    int32_t added = storageSys->addResourceToStorage(
                        building->getStorageId(), settler->getName(),
                        Resources::ResourceType::Wood, quantity);
                    if (added > 0) {
                      inventory.extractItem(i, added);
                      totalDeposited += added;
                    }
                  }
                }
                if (totalDeposited > 0) {
                  std::cout << "[InteractionSystem] Deposited "
                            << totalDeposited << " logs to "
                            << building->getStorageId() << std::endl;
                  anyAction = true;
                } else if (dist <= 5.0f) {
                  std::cout << "[InteractionSystem] No logs to deposit or "
                               "storage full."
                            << std::endl;
                }
              }
              if (anyAction) {
                m_wasInteractionHandled = true;
                return true;
              }
            }
          }
        }
      }
    }
  }
  const auto &droppedItems = colonyPtr->getDroppedItems();
  if (!droppedItems.empty()) {
    for (const auto &wItem : droppedItems) {
      if (!wItem.item)
        continue;
      RayCollision collision = GetRayCollisionSphere(ray, wItem.position, 0.5f);
      if (collision.hit) {
        if (collision.distance < minHitDist) {
          auto selectedSettlers = colonyPtr->getSelectedSettlers();
          for (auto *settler : selectedSettlers) {
            settler->assignTask(TaskType::PICKUP, nullptr, wItem.position);
            std::cout << "Ordering settler to pickup item at ("
                      << wItem.position.x << ", " << wItem.position.y << ", "
                      << wItem.position.z << ")" << std::endl;
          }
          if (!selectedSettlers.empty()) {
            std::cout << "[InteractionSystem] Clicked on dropped item: "
                      << (wItem.item ? wItem.item->getDisplayName() : "Unknown")
                      << std::endl;
            m_wasInteractionHandled = true;
            return true;
          }
        }
      }
    }
  }
  return false;
}
void InteractionSystem::renderUI(Camera3D camera) { (void)camera; }
std::type_index InteractionSystem::getSystemType() const {
  return std::type_index(typeid(InteractionSystem));
}
RaycastHit InteractionSystem::raycastFromCamera(Camera camera) {
  return RaycastHit();
}
std::vector<RaycastHit> InteractionSystem::raycastAllInRange(Camera camera) {
  return {};
}
void InteractionSystem::showInteractionUI(InteractableObject *target) {}
void InteractionSystem::hideInteractionUI() {}
void InteractionSystem::updateInteractionUI(const InteractionInfo &info) {}
RaycastHit InteractionSystem::findClosestInteractable(Vector3 position,
                                                      Vector3 direction) {
  return RaycastHit();
}
RaycastHit InteractionSystem::checkRayObjectCollision(
    Vector3 rayOrigin, Vector3 rayDirection, InteractableObject *object) {
  return RaycastHit();
}
void InteractionSystem::renderInteractionPrompt(Camera camera) {}
void InteractionSystem::updateMovement(float deltaTime) {}
bool InteractionSystem::hasReachedTarget() const { return false; }
