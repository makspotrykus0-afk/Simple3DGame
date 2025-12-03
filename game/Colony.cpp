#include "Colony.h"
#include <iostream>
#include <cstdlib> 
#include "raymath.h" 
#include "../systems/ResourceSystem.h"
#include "../systems/BuildingSystem.h"
#include "../systems/InteractionSystem.h"
#include "../core/GameEngine.h"
#include "../core/GameSystem.h"
#include "../game/ColonyAI.h"
#include "../game/BuildingInstance.h"
#include "../systems/StorageSystem.h"

// Access global building system to check for collisions on spawn
extern BuildingSystem* g_buildingSystem;

Colony::Colony() {
    m_ai = nullptr;
}

Colony::~Colony() {
    cleanup();
}

void Colony::initialize() {
    std::cout << "Colony initializing..." << std::endl;
    // Spawn initial settlers with more spacing to avoid collision with future houses
    addSettler({ 0.0f, 0.0f, 0.0f }, "Bob");
    addSettler({ 20.0f, 0.0f, 0.0f }, "Alice");
    addSettler({ 40.0f, 0.0f, 0.0f }, "John", SettlerProfession::HUNTER);

    // Add berry bushes nearby
    for (int i = 0; i < 10; ++i) {
        float angle = (float)(rand() % 360) * DEG2RAD;
        float dist = 15.0f + (float)(rand() % 15); // 15-30 units away
        Vector3 bushPos = { cosf(angle) * dist, 0.0f, sinf(angle) * dist };
        addBush(bushPos);
    }

    // Add Rabbits (Zające)
    for (int i = 0; i < 5; ++i) {
        float angle = (float)(rand() % 360) * DEG2RAD;
        float dist = 10.0f + (float)(rand() % 20);
        Vector3 rabbitPos = { cosf(angle) * dist, 0.0f, sinf(angle) * dist };
        addAnimal(rabbitPos, AnimalType::RABBIT);
    }

    // Initialize Storehouse (using global BuildingSystem if available, simulated here if not fully integrated)
    if (g_buildingSystem) {
        // Force create a storehouse at fixed position
        g_buildingSystem->ensureBlueprintExists("storehouse");
        bool success = false;
        g_buildingSystem->startBuilding("storehouse", { 10.0f, 0.0f, 10.0f }, nullptr, 0.0f, true, true, true, &success);
        if (success) {
            std::cout << "Initialized central storehouse at (10, 0, 10)" << std::endl;
            BuildingInstance* b = g_buildingSystem->getBuildingAt({ 10.0f, 0.0f, 10.0f });
            if (b) b->setOwner("Colony");
        }

        // Create Simple Storage
        bool storageSuccess = false;
        g_buildingSystem->startBuilding("simple_storage", { 20.0f, 0.0f, 10.0f }, nullptr, 0.0f, true, true, true, &storageSuccess);
        if (storageSuccess) {
            std::cout << "Initialized Simple Storage at (20, 0, 10)" << std::endl;
            BuildingInstance* b = g_buildingSystem->getBuildingAt({ 20.0f, 0.0f, 10.0f });
            if (b) {
                b->setOwner("Colony");
                
                // Ensure storage is created if not already
                if (b->getStorageId().empty()) {
                    StorageSystem* storageSys = GameEngine::getInstance().getSystem<StorageSystem>();
                    if (storageSys) {
                        std::string storageId = storageSys->createStorage(StorageType::WAREHOUSE, "Colony");
                        b->setStorageId(storageId);
                        std::cout << "Manual Init: Created storage " << storageId << " for Simple Storage" << std::endl;
                    }
                } else {
                    std::cout << "Manual Init: Simple Storage already has ID: " << b->getStorageId() << std::endl;
                }
            }
        }

        // Generate starter houses for settlers
        for (auto* settler : settlers) {
            std::string blueprintId = "house_4"; // Default size
            if (settler->preferredHouseSize == 6) blueprintId = "house_6";
            else if (settler->preferredHouseSize == 9) blueprintId = "house_9";
            
            g_buildingSystem->ensureBlueprintExists(blueprintId);

            Vector3 startPos = settler->getPosition();
            Vector3 housePos = startPos;
            bool placed = false;
            
            // Search for a valid spot near the settler
            float searchRadius = 5.0f;
            for (int i = 0; i < 20; ++i) {
                float angle = (float)(rand() % 360) * DEG2RAD;
                float dist = 3.0f + (float)(rand() % 100) / 100.0f * searchRadius;
                Vector3 tryPos = Vector3Add(startPos, { cosf(angle) * dist, 0.0f, sinf(angle) * dist });
                
                // Round to grid if needed, BuildingSystem handles snap but let's be nice
                tryPos.x = roundf(tryPos.x);
                tryPos.z = roundf(tryPos.z);

                if (g_buildingSystem->canBuild(blueprintId, tryPos)) {
                    housePos = tryPos;
                    placed = true;
                    break;
                }
                
                if (i % 5 == 0) searchRadius += 2.0f;
            }

            if (placed) {
                bool buildSuccess = false;
                // Instant build
                g_buildingSystem->startBuilding(blueprintId, housePos, settler, 0.0f, true, false, true, &buildSuccess);
                
                if (buildSuccess) {
                    std::cout << "Generated starter house for " << settler->getName() << " at " << housePos.x << ", " << housePos.z << std::endl;
                    // Find the newly created building instance to verify owner (startBuilding doesn't return it for instant builds currently, but sets owner internally)
                    // We can also explicitly find it if needed, but startBuilding(..., settler, ...) handles setOwner.
                    
                    // Optional: assign bed explicitly if Settler logic requires it immediately
                    // BuildingSystem::startBuilding with instant=true creates the building and puts it in m_buildings.
                    // We'd need to query it if we wanted to call settler->assignBed(), but currently Settler finds bed via AI or we trust the system.
                    // Let's try to link it for robustness:
                    BuildingInstance* house = g_buildingSystem->getBuildingAt(housePos);
                    if (house) {
                        settler->assignBed(house); // Assuming house acts as bed container or has a bed component
                        house->setOwner(settler->getName()); // Reinforce ownership
                    }
                } else {
                    std::cout << "Failed to build starter house for " << settler->getName() << std::endl;
                }
            } else {
                std::cout << "Could not find space for starter house for " << settler->getName() << std::endl;
            }
        }
    }

    // Initialize ColonyAI
    m_ai = std::make_unique<ColonyAI>(this, nullptr);
    std::cout << "Colony initialized with " << settlers.size() << " settlers and animals." << std::endl;

    // Initialize ColonyAI
    // m_ai = std::make_unique<ColonyAI>(this, nullptr); // Already initialized above
}

void Colony::cleanup() {
    for (auto* settler : settlers) {
        delete settler;
    }
    settlers.clear();
    
    // Unregister trees from InteractionSystem
    InteractionSystem* interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>();
    if (interactionSystem) {
        // We can't easily unregister all, but we should clear them if we delete them.
        // Assuming InteractionSystem handles weak pointers or we clear its list.
        // For now, just deleting the pointers here.
    }
    
    for (auto* bush : bushes) {
        delete bush;
    }
    bushes.clear();
    m_animals.clear();
}

void Colony::update(float deltaTime, const std::vector<std::unique_ptr<Tree>>& trees, std::vector<WorldItem>& droppedItems, const std::vector<BuildingInstance*>& buildings) {
    // Update AI
    if (m_ai) {
        m_ai->update(deltaTime);
    }

    currentDroppedItems = &droppedItems;

    for (auto* settler : settlers) {
        settler->Update(deltaTime, trees, droppedItems, bushes, buildings, m_animals);
    }
    
    // Update animals
    for (auto& animal : m_animals) {
        if (animal->isActive()) {
            animal->update(deltaTime);
        }
    }

    // Update projectiles
    for (auto& proj : m_projectiles) {
        proj->update(deltaTime);
        
        // Check collision with animals
        if (proj->isActive()) {
            for (auto& animal : m_animals) {
                if (animal->isActive() && !animal->isDead()) {
                    // Simple distance check for collision (bullet radius 0.1 + animal radius approx 0.5)
                    if (Vector3Distance(proj->getPosition(), animal->getPosition()) < 0.6f) {
                        animal->takeDamage(proj->getDamage());
                        proj->deactivate();
                        std::cout << "Projectile hit animal!" << std::endl;
                        
                        if (animal->isDead()) {
                            std::cout << "Animal killed by projectile." << std::endl;
                            // Drop raw meat at animal position (Optional, as Settler logic might auto-claim it)
                            // To avoid duplication if Settler claims it instantly, we can check if a hunter is targeting it?
                            // But for now, let's drop it. If settler logic duplicates it, it's a "bonus" for testing.
                            // In a real fix, we'd sync them.
                            // Let's rely on Settler claiming it directly in UpdateHunting for immediate hauling.
                            // addDroppedItem(std::make_unique<ConsumableItem>("Raw Meat", "Fresh raw meat."), animal->getPosition());
                        }
                        break;
                    }
                }
            }
        }
    }
    
    // Remove inactive projectiles
    m_projectiles.erase(
        std::remove_if(m_projectiles.begin(), m_projectiles.end(),
            [](const std::unique_ptr<Projectile>& p) { return !p->isActive(); }),
        m_projectiles.end());
    
    // Update bushes (regrowth)
    for (auto* bush : bushes) {
        if (!bush->hasFruit) {
            bush->regrowthTimer += deltaTime;
            if (bush->regrowthTimer >= bush->maxRegrowthTime) {
                bush->hasFruit = true;
                bush->regrowthTimer = 0.0f;
            }
        }
    }
    
    currentDroppedItems = nullptr;
}

void Colony::render(bool isFPSMode, Settler* selectedSettler) {
    for (auto* settler : settlers) {
        if (isFPSMode && settler == selectedSettler) {
            continue;
        }
        settler->render(); 
    }
    
    // Render animals
    for (auto& animal : m_animals) {
        if (animal->isActive()) {
            animal->render();
        }
    }

    // Render projectiles
    for (const auto& proj : m_projectiles) {
        proj->render();
    }
    
    // Render bushes
    for (auto* bush : bushes) {
        Vector3 drawPos = bush->position;
        drawPos.y += 0.5f; // Lift up so it sits on ground (assuming center is origin)
        
        DrawCube(drawPos, 1.0f, 1.0f, 1.0f, DARKGREEN);
        DrawCubeWires(drawPos, 1.0f, 1.0f, 1.0f, BLACK);
        
        if (bush->hasFruit) {
            // Draw berries
            Vector3 berryPos = drawPos;
            berryPos.y += 0.6f;
            DrawSphere(berryPos, 0.25f, RED);
        }
    }
}

void Colony::addSettler(Vector3 position, std::string name, SettlerProfession profession) {
    std::cout << "Attempting to add settler: " << name << " at " << position.x << ", " << position.z << std::endl;
    
    // Find valid position to avoid spawning inside buildings
    Vector3 spawnPos = position;
    
    if (g_buildingSystem) {
        const float settlerRadius = 0.5f; // Approximate settler radius
        const int maxAttempts = 30;
        bool valid = false;
        
        // Helper to check if position is free
        auto checkPos = [&](Vector3 p) -> bool {
             auto buildings = g_buildingSystem->getBuildingsInRange(p, 5.0f);
             for (auto* b : buildings) {
                 // Ignore floors (optional, but usually we can walk on floors)
                 if (b->getBlueprintId() == "floor") continue; 
                 
                 if (CheckCollisionBoxSphere(b->getBoundingBox(), p, settlerRadius)) {
                     return false;
                 }
             }
             return true;
        };

        if (checkPos(spawnPos)) {
            valid = true;
        } else {
             std::cout << "Spawn position occupied for " << name << ", searching for free space..." << std::endl;
             // Spiral/Random search nearby
             float searchRadius = 2.0f;
             for (int i = 0; i < maxAttempts; ++i) {
                  // Random angle
                  float angle = (float)(rand() % 360) * DEG2RAD;
                  // Random distance within current search radius (biased outwards)
                  float dist = ((float)(rand() % 100) / 100.0f) * searchRadius + 1.0f;
                  
                  Vector3 offset = { cosf(angle) * dist, 0.0f, sinf(angle) * dist };
                  Vector3 tryPos = Vector3Add(position, offset);
                  
                  if (checkPos(tryPos)) {
                      spawnPos = tryPos;
                      valid = true;
                      std::cout << "Found valid spawn pos at: " << tryPos.x << ", " << tryPos.z << std::endl;
                      break;
                  }
                  
                  // Gradually expand search radius if we fail a few times
                  if (i % 5 == 0) searchRadius += 2.0f;
             }
        }
        
        if (!valid) {
            std::cout << "Warning: Could not find valid spawn position for settler " << name << ". Spawning at original pos." << std::endl;
        }
    } else {
        std::cout << "BuildingSystem not available during spawn check." << std::endl;
    }

    // Correct constructor call: name first, then position
    Settler* newSettler = new Settler(name, spawnPos, profession);
    
    // Register with InteractionSystem
    InteractionSystem* interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>();
    if (interactionSystem) {
        interactionSystem->registerInteractableObject(newSettler);
        std::cout << "Registered settler " << name << " with InteractionSystem" << std::endl;
    }
    
    // Assign random preferred house size (4, 6, 9)
    int sizes[] = { 4, 6, 9 };
    int sizeIdx = rand() % 3;
    newSettler->preferredHouseSize = sizes[sizeIdx];
    
    settlers.push_back(newSettler);
    std::cout << "Added settler: " << newSettler->getName() << " (Pref House: " << newSettler->preferredHouseSize << ")" << std::endl;
}

Settler* Colony::getSettlerAt(Vector3 position, float radius) {
    for (auto* settler : settlers) {
        // Using manual collision check instead of CheckCollisionPointSphere if missing
        // DistanceSqr check is efficient
        if (Vector3DistanceSqr(position, settler->getPosition()) <= radius * radius) {
            return settler;
        }
    }
    return nullptr;
}

void Colony::clearSelection() {
    for (auto* settler : settlers) {
        settler->SetSelected(false);
    }
}

std::vector<Settler*> Colony::getSelectedSettlers() const {
    std::vector<Settler*> selected;
    for (auto* settler : settlers) {
        if (settler->IsSelected()) {
            selected.push_back(settler);
        }
    }
    return selected;
}

// Helper for ray-sphere intersection
bool RaySphereIntersect(Ray ray, Vector3 center, float radius, float& outDist) {
    Vector3 m = Vector3Subtract(ray.position, center);
    float b = Vector3DotProduct(m, ray.direction);
    float c = Vector3DotProduct(m, m) - radius * radius;

    if (c > 0.0f && b > 0.0f) return false;

    float discr = b * b - c;

    if (discr < 0.0f) return false;

    float t = -b - sqrtf(discr);

    if (t < 0.0f) t = 0.0f;

    outDist = t;
    return true;
}

bool Colony::checkHit(Ray ray, float maxDist, std::string& outHitInfo) {
    float closestDist = maxDist + 1.0f;
    bool hit = false;

    for (auto* settler : settlers) {
        float hitDist = 0.0f;
        if (RaySphereIntersect(ray, settler->getPosition(), 1.0f, hitDist)) { 
            if (hitDist < maxDist && hitDist < closestDist) {
                closestDist = hitDist;
                outHitInfo = "Settler " + settler->getName();
                hit = true;
            }
        }
    }
    
    return hit;
}

void Colony::registerHouse(Vector3 pos, int size, bool occupied) {
    if (m_ai) {
        m_ai->registerHouse(pos, size, occupied);
    }
}

void Colony::addAnimal(Vector3 position, AnimalType type) {
    m_animals.push_back(std::make_unique<Animal>(type, position));
}

void Colony::addBush(Vector3 position) {
    bushes.push_back(new Bush(position));
}

void Colony::addProjectile(std::unique_ptr<Projectile> projectile) {
    m_projectiles.push_back(std::move(projectile));
}

void Colony::addResource(const std::string& resourceName, int amount) {
    // TODO: Implement proper resource storage system
    std::cout << "Colony gained resource: " << resourceName << " x" << amount << std::endl;
}

void Colony::addDroppedItem(std::unique_ptr<Item> item, Vector3 position) {
    if (currentDroppedItems) {
        currentDroppedItems->emplace_back(position, std::move(item), (float)GetTime(), false, 1);
    } else {
        m_droppedItemsStorage.push_back(WorldItem(position, std::move(item), (float)GetTime(), false, 1));
    }
}