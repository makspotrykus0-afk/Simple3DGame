#include "InteractionSystem.h"
#include "../game/Colony.h"
#include "../game/Tree.h"
#include "../game/Animal.h" // Include Animal
#include "../systems/BuildingSystem.h" // Include BuildingSystem
#include "../core/GameEngine.h" // Include GameEngine for system access
#include "../game/Door.h"
#include <iostream>

// Note: GameEngine handles IGameSystem registration

InteractionSystem::InteractionSystem()
    : m_playerEntity(nullptr)
    , m_interactionRange(10.0f)
    , m_currentTarget(nullptr)
    , m_inputEnabled(true)
    , m_lastInteractionTime(0.0f)
    , m_uiVisible(false)
    , m_wasInteractionHandled(false)
    , m_chopTimer(0.0f)
    , m_isChopping(false)
    , m_activeChopTarget(nullptr)
    , m_movementTarget({ 0.0f, 0.0f, 0.0f })
    , m_isMoving(false)
    , m_movementSpeed(5.0f)
    , m_activationDistance(2.0f) // Increased slightly for trees
{
}

void InteractionSystem::beginFrame() {
    m_wasInteractionHandled = false;
}

void InteractionSystem::initialize() {
    // Nothing to init for now
}

void InteractionSystem::update(float deltaTime) {
    (void)deltaTime;
    // Update registered interactables (like Doors)
    for (auto* obj : m_interactableObjects) {
        if (obj && obj->isActive()) {
            obj->update(deltaTime);
        }
    }
}

void InteractionSystem::render() {
    // Debug visualization
    if (m_activeChopTarget) {
        DrawCubeWires(m_activeChopTarget->getPosition(), 1.2f, 1.2f, 1.2f, RED);
    }
}

void InteractionSystem::shutdown() {
    // Cleanup
}

void InteractionSystem::registerInteractableObject(InteractableObject* obj) {
    m_interactableObjects.push_back(obj);
}

void InteractionSystem::unregisterInteractableObject(InteractableObject* obj) {
    // In a real system, we'd remove from vector
    for (auto it = m_interactableObjects.begin(); it != m_interactableObjects.end(); ++it) {
        if (*it == obj) {
            m_interactableObjects.erase(it);
            break;
        }
    }
}

void InteractionSystem::processPlayerInput(Camera3D camera) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        Vector2 mousePosition = GetMousePosition();
        Ray ray = GetMouseRay(mousePosition, camera);
        handleInput(ray);
    }

    // Handle selection with Left Click
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // If we are clicking on UI, we should probably ignore (UI system handles that)
        // But for now, assume 3D selection
        Vector2 mousePosition = GetMousePosition();
        Ray ray = GetMouseRay(mousePosition, camera);
        handleSelection(ray);
    }
}

bool InteractionSystem::handleSelection(Ray ray) {
    if (!m_colony) return false;
    
    float minHitDist = 9999.0f;
    Settler* hitSettler = nullptr;
    
    for (auto* settler : m_colony->getSettlers()) {
        if (!settler) continue;

        Vector3 sPos = settler->getPosition();
        // Simple sphere collision for selection
        RayCollision collision = GetRayCollisionSphere(ray, {sPos.x, sPos.y + 1.0f, sPos.z}, 1.0f);
        
        if (collision.hit) {
            if (collision.distance < minHitDist) {
                minHitDist = collision.distance;
                hitSettler = settler;
            }
        }
    }

    if (hitSettler) {
        // Deselect others if not holding shift
        if (!IsKeyDown(KEY_LEFT_SHIFT)) {
            m_colony->clearSelection();
        }
        hitSettler->SetSelected(true);
        return true;
    } else {
        // If we clicked nothing and aren't holding shift, clear selection
        // This mimics standard RTS behavior (click ground to deselect)
        if (!IsKeyDown(KEY_LEFT_SHIFT)) {
            m_colony->clearSelection();
        }
    }
    
    return false;
}

// Implementation of Ray based handleInput
bool InteractionSystem::handleInput(Ray ray) {
    float minHitDist = 9999.0f;
    InteractableObject* hitObj = nullptr;

    // 1. Check Registered Objects (Trees, Doors)
    for (auto* obj : m_interactableObjects) {
        if (!obj || !obj->isActive()) continue;
        
        // Use getBoundingBox for more accurate collision detection (especially for Trees)
        // If getBoundingBox is not available or not virtual in InteractableObject, we should cast
        // But InteractableObject might not have it. Let's check specific types or use Position.
        
        // Try to dynamic cast to Tree to use its bounding box
        Tree* tree = dynamic_cast<Tree*>(obj);
        if (tree) {
            // Check trunk first
            BoundingBox bbox = tree->getBoundingBox();
            RayCollision collision = GetRayCollisionBox(ray, bbox);
            if (!collision.hit) {
                // Fallback to sphere check for foliage/general area if box missed,
                // but box should be primary. Let's stick to Box for accuracy or
                // add a larger sphere for easier clicking if needed.
                // For now, rely on Box as requested for "precise detection".
            }
            
            if (collision.hit) {
                if (collision.distance < minHitDist) {
                    minHitDist = collision.distance;
                    hitObj = obj;
                }
            }
            continue; // Skip standard sphere check
        }

        // Fallback to Sphere check for other objects (like Doors if they don't override)
        // But Doors are usually flat... for now simple sphere check
        Vector3 pos = obj->getPosition();
        // Adjust center for better hit (assuming base origin)
        Vector3 center = { pos.x, pos.y + 1.0f, pos.z };
        
        RayCollision collision = GetRayCollisionSphere(ray, center, 1.0f);
        
        if (collision.hit) {
            if (collision.distance < minHitDist) {
                minHitDist = collision.distance;
                hitObj = obj;
            }
        }
    }

    // 1.5 Check Animals (from Colony)
    // Wymagane forward declaration Colony w nagłówku i dostęp do globalnej instancji lub singletona
    // Zakładamy extern Colony colony; z main.cpp
    extern Colony colony;
    const auto& animals = colony.getAnimals();
    for (const auto& animal : animals) {
        if (!animal || !animal->isActive()) continue;
        
        // Animal hit check
        Vector3 pos = animal->getPosition();
        Vector3 center = { pos.x, pos.y + 0.5f, pos.z };
        
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
        
        auto selectedSettlers = colony.getSelectedSettlers();
        
        // Dispatch based on type
        Tree* hitTree = dynamic_cast<Tree*>(hitObj);
        if (hitTree) {
            for (auto* settler : selectedSettlers) {
                // settler->AssignToChop(hitTree); // Old method
                settler->ForceGatherTarget(hitTree); // New RTS style method
            }
            std::cout << "Interaction: Tree selected at " << hitTree->getPosition().x << std::endl;
            return true; 
        }
        
        Door* hitDoor = dynamic_cast<Door*>(hitObj);
        if (hitDoor) {
            // Player manually toggling door (god mode)
            hitDoor->interact(nullptr);
            return true;
        }

        Animal* hitAnimal = dynamic_cast<Animal*>(hitObj);
        if (hitAnimal) {
            // Handle Hunting or Interaction
            for (auto* settler : selectedSettlers) {
                (void)settler; // Silence warning
                
                // Using interact for now which handles interaction logic
                hitAnimal->interact(nullptr);
            }
            return true;
        }
        
        return true; // Priority over items
    }

    // 1.5 Check Build Tasks (Ghost Buildings)
    // Use GameEngine to get BuildingSystem instance
    BuildingSystem* buildingSystem = m_buildingSystem;
    if (!buildingSystem) {
        buildingSystem = GameEngine::getInstance().getSystem<BuildingSystem>();
    }

    // Simple sphere check against known task positions for now since we don't have direct raycast support in BuildingSystem yet
    // Or use the getBuildTaskAt which is distance based from a point.
    // To do it properly with Ray, we iterate active tasks.
    // Let's try to access tasks via friend or public accessor if available, or just use distance check to ground intersection
    
    // Alternative: Ray-Plane intersection to get ground point, then check distance to tasks
    RayCollision groundHit = GetRayCollisionQuad(ray, { -1000, 0, -1000 }, { -1000, 0, 1000 }, { 1000, 0, 1000 }, { 1000, 0, -1000 });
    if (groundHit.hit && buildingSystem) {
            BuildTask* nearbyTask = buildingSystem->getBuildTaskAt(groundHit.point, 2.0f);
            if (nearbyTask && nearbyTask->isActive()) {
                extern Colony colony; // Ensure visible
                auto selectedSettlers = colony.getSelectedSettlers();
                for (auto* settler : selectedSettlers) {
                    settler->AssignBuildTask(nearbyTask);
                }
                if (!selectedSettlers.empty()) {
                    m_wasInteractionHandled = true;
                    return true;
                }
            }
    }

    // 2. Check Dropped Items
    extern Colony colony;
    const auto* droppedItems = colony.getDroppedItems();
    if (droppedItems) {
        for (const auto& wItem : *droppedItems) {
            if (!wItem.item) continue;

            RayCollision collision = GetRayCollisionSphere(ray, wItem.position, 0.5f);
            
            if (collision.hit) {
                if (collision.distance < minHitDist) {
                    // Assign task directly to settlers for items (as requested in previous tasks)
                    auto selectedSettlers = colony.getSelectedSettlers();
                    for (auto* settler : selectedSettlers) {
                            // Move to item position to simulate pickup
                            settler->MoveTo(wItem.position);
                            std::cout << "Ordering settler to move to item at " << wItem.position.x << std::endl;
                    }
                    
                    if (!selectedSettlers.empty()) {
                            m_wasInteractionHandled = true;
                            // We don't set m_activeChopTarget for items, handled by Settler logic
                            return true;
                    }
                }
            }
        }
    }
    
    return false;
}

void InteractionSystem::renderUI(Camera3D camera) {
    (void)camera;
}

std::type_index InteractionSystem::getSystemType() const {
    return std::type_index(typeid(InteractionSystem));
}

// Implementacja metod raycast (dummy placeholders jeśli nie są używane, ale wymagane przez nagłówek)
RaycastHit InteractionSystem::raycastFromCamera(Camera camera) {
    return RaycastHit();
}

std::vector<RaycastHit> InteractionSystem::raycastAllInRange(Camera camera) {
    return {};
}

void InteractionSystem::showInteractionUI(InteractableObject* target) {}
void InteractionSystem::hideInteractionUI() {}
void InteractionSystem::updateInteractionUI(const InteractionInfo& info) {}

RaycastHit InteractionSystem::findClosestInteractable(Vector3 position, Vector3 direction) { return RaycastHit(); }
RaycastHit InteractionSystem::checkRayObjectCollision(Vector3 rayOrigin, Vector3 rayDirection, InteractableObject* object) { return RaycastHit(); }
void InteractionSystem::renderInteractionPrompt(Camera camera) {}
void InteractionSystem::updateMovement(float deltaTime) {}
bool InteractionSystem::hasReachedTarget() const { return false; }
