#include "Terrain.h"
#include "Colony.h"
#include "Tree.h" // Required for Tree operations
#include "../systems/InteractionSystem.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <random>
#include <utility>
#include <queue>
#include <map>
#include <memory>
#include <functional>
#include "raylib.h"
#include "raymath.h"
#include "../game/Item.h" // Include for ResourceItem
#include "../components/StatsComponent.h" // Include StatsComponent
#include "../core/GameSystem.h" // Include GameSystem explicitly
#include "../core/IGameSystem.h" // Include IGameSystem explicitly
#include "../systems/BuildingSystem.h" // Include BuildingSystem
#include "../game/BuildingBlueprint.h"
#include "../core/GameEngine.h" // WAŻNE: Include GameEngine
#include "../systems/UISystem.h" // Include UISystem
#include "../systems/InventorySystem.h" // Added InventorySystem
#include "../systems/TimeCycleSystem.h" // Added TimeCycleSystem
#include "../systems/StorageSystem.h" // Added StorageSystem
#include "../game/NavigationGrid.h" // Added missing include

// ============================================================================
// SYSTEM KAMERY - REFACTORYZACJA KOMPLETNA
// ============================================================================

// Global variables
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;

Camera3D sceneCamera;
Camera3D settlerCamera;
bool cameraMode = false; // false = free camera, true = follow settler camera (FPS mode)

// Timing
float deltaTime = 0.0f;

// Forward declarations
// Removed forward declaration to ensure compiler uses full definition from include
extern BuildingSystem* g_buildingSystem;

// ============================================================================
// KLASY OBSŁUGI KAMERY
// ============================================================================

class CameraController {
private:
    // Kamera kontrolowana
    Camera3D* cam;

    // Stan kamery
    Vector3 initialPosition;
    Vector3 initialTarget;
    Vector3 initialUp;

    // Rotacja kamery
    float yaw = 0.0f;   // Obrót horyzontalny (wokół osi Y świata)
    float pitch = 0.0f; // Obrót wertykalny (wokół lokalnej osi X)

    // Parametry ruchu
    float moveSpeed = 15.0f; // Increased from 5.0f to 15.0f
    float rotationSpeed = 0.002f;
    float pitchLimit = 1.2f; // ~69 stopni
    float zoomSpeed = 2.0f;  // Zoom speed

    // Smoothing input
    Vector2 rawMouseDelta = {0.0f, 0.0f};
    Vector2 smoothedMouseDelta = {0.0f, 0.0f};
    const float MOUSE_SMOOTHING = 0.15f; // Współczynnik interpolacji

    // Debug
    bool showDebug = true;

    // Collision radius for camera/player
    float collisionRadius = 0.5f;

public:
    // Constructor now directly takes the camera to control
    CameraController(Camera3D* controlledCamera) : cam(controlledCamera) {
        initialPosition = cam->position;
        initialTarget = cam->target;
        initialUp = cam->up;
        
        // Initialize rotation from camera
        updateRotationFromCamera();
    }
    
    // Reset controller state (useful when switching modes)
    void resetState() {
        rawMouseDelta = {0.0f, 0.0f};
        smoothedMouseDelta = {0.0f, 0.0f};
        updateRotationFromCamera();
    }

    // Aktualizacja stanu kamery
    void update() {
        // Oblicz aktualne kąty z pozycji kamery - ale tylko jeśli nie jesteśmy w trakcie rotacji myszą
        // W trybie FreeCam rotacja jest nadrzędna nad pozycją w updateRotationFromCamera
        // updateRotationFromCamera(); // To powoduje "świrowanie" jeśli updateRotation() potem też zmienia yaw/pitch

        // Obsługa inputu
        handleInput();

        // Zastosuj nową rotację
        applyRotation();
    }

    // Metoda do ustawiania rotacji (dla trybu FPS)
    void setRotation(float newYaw, float newPitch) {
        yaw = newYaw;
        pitch = newPitch;
        // Clamp pitch
        pitch = std::max(-pitchLimit, std::min(pitch, pitchLimit));
        
        // Apply immediately for FPS mode
        // Note: In FPS mode we set target manually based on yaw/pitch, so applyRotation() might not be needed in the same way
    }

private:
    // Aktualizuje kąty rotacji na podstawie aktualnej pozycji kamery
    void updateRotationFromCamera() {
        Vector3 toTarget = Vector3Subtract(cam->target, cam->position);
        float distance = Vector3Length(toTarget);

        if (distance > 0.001f) {
            // Normalizuj i oblicz kąty
            toTarget = Vector3Normalize(toTarget);
            yaw = atan2f(toTarget.x, toTarget.z); // Horyzontal
            pitch = asinf(toTarget.y); // Wertykal
            
            // Ograniczenie pitch przy inicjalizacji, aby nie było problemu z odwracaniem kamery
            pitch = std::max(-pitchLimit, std::min(pitch, pitchLimit));
        }
    }

    // Obsługa inputu użytkownika
    void handleInput() {
        // W trybie FPS sterowanie kamerą jest inne niż w trybie wolnym
        // Tutaj implementujemy logikę dla FREE CAMERA
        // Settler camera (FPS) input is handled separately or overrides this
        
        if (cameraMode) return; // Skip input handling if in FPS mode (handled elsewhere)

        // Movement WASD
        Vector3 moveDir = {0,0,0};
        if (IsKeyDown(KEY_W)) moveDir = getForwardVector();
        if (IsKeyDown(KEY_S)) moveDir = Vector3Scale(getForwardVector(), -1.0f);
        if (IsKeyDown(KEY_A)) moveDir = getRightVector(true); // left
        if (IsKeyDown(KEY_D)) moveDir = getRightVector(false); // right

        if (Vector3LengthSqr(moveDir) > 0.001f) {
            moveDir.y = 0; // Flatten movement to horizontal plane
            moveDir = Vector3Normalize(moveDir);
            attemptMove(moveDir);
        }

        // Zoom using Mouse Wheel
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            handleZoom(wheel);
        }

        // Rotacja środkowym przyciskiem myszy
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            rawMouseDelta = GetMouseDelta();
            updateRotation();
        } else {
            // Reset smoothed deltas gdy przycisk jest zwolniony
            rawMouseDelta = {0.0f, 0.0f};
            smoothedMouseDelta = {0.0f, 0.0f};
        }

        // Toggle debug
        if (IsKeyPressed(KEY_F1)) showDebug = !showDebug;
    }

    // Handle Zoom
    void handleZoom(float wheel) {
        // Move camera position closer to or further from target
        // Calculate vector from target to position
        Vector3 toPos = Vector3Subtract(cam->position, cam->target);
        float distance = Vector3Length(toPos);
        
        // Zoom in (decrease distance), Zoom out (increase distance)
        float zoomAmount = wheel * zoomSpeed;
        float newDistance = distance - zoomAmount;
        
        // Clamp distance limits
        if (newDistance < 2.0f) newDistance = 2.0f;
        if (newDistance > 50.0f) newDistance = 50.0f;
        
        // Apply new distance
        toPos = Vector3Normalize(toPos);
        cam->position = Vector3Add(cam->target, Vector3Scale(toPos, newDistance));
    }

    // Helper to get right vector
    Vector3 getRightVector(bool left) {
        Vector3 fwd = getForwardVector();
        fwd.y = 0;
        fwd = Vector3Normalize(fwd);
        Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, Vector3{0, 1, 0}));
        return left ? Vector3Scale(right, -1.0f) : right;
    }

    // Attempt to move with collision check
    void attemptMove(Vector3 direction) {
        Vector3 offset = Vector3Scale(direction, moveSpeed * deltaTime);
        Vector3 newPos = Vector3Add(cam->position, offset);
        Vector3 newTarget = Vector3Add(cam->target, offset);

        // Collision check
        if (checkCollision(newPos)) {
            // Simple sliding: try removing X component
            Vector3 slideX = {0, direction.y, direction.z};
            if (Vector3LengthSqr(slideX) > 0.01f) {
                 slideX = Vector3Normalize(slideX);
                 Vector3 offsetX = Vector3Scale(slideX, moveSpeed * deltaTime);
                 Vector3 newPosX = Vector3Add(cam->position, offsetX);
                 if (!checkCollision(newPosX)) {
                     cam->position = newPosX;
                     cam->target = Vector3Add(cam->target, offsetX);
                     return;
                 }
            }
             // Try removing Z component
            Vector3 slideZ = {direction.x, direction.y, 0};
            if (Vector3LengthSqr(slideZ) > 0.01f) {
                 slideZ = Vector3Normalize(slideZ);
                 Vector3 offsetZ = Vector3Scale(slideZ, moveSpeed * deltaTime);
                 Vector3 newPosZ = Vector3Add(cam->position, offsetZ);
                 if (!checkCollision(newPosZ)) {
                     cam->position = newPosZ;
                     cam->target = Vector3Add(cam->target, offsetZ);
                     return;
                 }
            }
            // Blocked
        } else {
            cam->position = newPos;
            cam->target = newTarget;
        }
    }

    bool checkCollision(Vector3 pos) {
        if (!g_buildingSystem) return false;

        // Check building instances
        auto buildings = g_buildingSystem->getBuildingsInRange(pos, 5.0f); // Check nearby
        
        for (auto* building : buildings) {
            if (building->getBlueprintId() == "floor") continue; // Ignore floors for collision

            // Use new precise collision check
            if (building->CheckCollision(pos, collisionRadius)) {
                return true;
            }
        }
        return false;
    }

    // Aktualizuje rotację na podstawie smoothed input
    void updateRotation() {
        // Apply smoothing
        smoothedMouseDelta.x = Lerp(smoothedMouseDelta.x, rawMouseDelta.x, MOUSE_SMOOTHING);
        smoothedMouseDelta.y = Lerp(smoothedMouseDelta.y, rawMouseDelta.y, MOUSE_SMOOTHING);

        // Aktualizuj kąty z ograniczeniami
        yaw -= smoothedMouseDelta.x * rotationSpeed; // Horyzontal
        pitch -= smoothedMouseDelta.y * rotationSpeed; // Wertykal

        // Ogranicz pitch (nie pozwól na przewrócenie kamery)
        pitch = std::max(-pitchLimit, std::min(pitch, pitchLimit));
    }

    // Zastosuj obliczoną rotację do kamery
    void applyRotation() {
        if (cameraMode) return; // In FPS mode, applied differently (cam pos is fixed)

        // Oblicz nową pozycję na podstawie kątów
        float distance = Vector3Length(Vector3Subtract(cam->position, cam->target));

        Vector3 offsetFromTarget;
        offsetFromTarget.x = -sinf(yaw) * cosf(pitch); // Minus, żeby obrót myszką był zgodny
        offsetFromTarget.y = -sinf(pitch);             // Minus, żeby pitch > 0 patrzył w dół (kamera wyżej)
        offsetFromTarget.z = -cosf(yaw) * cosf(pitch); // Minus
        
        Vector3 direction;
        // Standardowe współrzędne sferyczne (y up)
        direction.x = cosf(pitch) * sinf(yaw);
        direction.y = sinf(pitch);
        direction.z = cosf(pitch) * cosf(yaw);
        
        direction.x = -cosf(pitch) * sinf(yaw); // Dopasowanie znaków do obrotu
        direction.y = -sinf(pitch);             // Odwracamy Y, żeby ujemny pitch dawał dodatnią wysokość
        direction.z = -cosf(pitch) * cosf(yaw);
        
        // W trybie swobodnym orbitujemy wokół celu
        Vector3 newPosition = Vector3Add(cam->target, Vector3Scale(direction, distance));
        cam->position = newPosition;

        // Normalna góra zawsze (chyba że robimy pętle, ale w RTS raczej nie)
        cam->up = {0, 1, 0};
    }

    // Getters dla debugowania
public:
    Vector3 getForwardVector() {
        return Vector3Normalize(Vector3Subtract(cam->target, cam->position));
    }

    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }
    Vector2 getSmoothedMouseDelta() const { return smoothedMouseDelta; }
    bool isDebugEnabled() const { return showDebug; }

    // Debug rendering
    void renderDebug() {
        if (!showDebug) return;

        DrawText("=== KAMERA DEBUG ===", SCREEN_WIDTH - 320, 160, 14, Color{255, 255, 0, 255});
        DrawText(("Position: (" + std::to_string(cam->position.x).substr(0, 5) + ", " +
                 std::to_string(cam->position.y).substr(0, 5) + ", " +
                 std::to_string(cam->position.z).substr(0, 5) + ")").c_str(),
                SCREEN_WIDTH - 320, 180, 12, Color{255, 255, 255, 255});
        DrawText(("Target: (" + std::to_string(cam->target.x).substr(0, 5) + ", " +
                 std::to_string(cam->target.y).substr(0, 5) + ", " +
                 std::to_string(cam->target.z).substr(0, 5) + ")").c_str(),
                SCREEN_WIDTH - 320, 195, 12, Color{255, 255, 255, 255});
        DrawText(("Yaw: " + std::to_string(yaw).substr(0, 5)).c_str(),
                SCREEN_WIDTH - 320, 210, 12, Color{255, 255, 255, 255});
        DrawText(("Pitch: " + std::to_string(pitch).substr(0, 5)).c_str(),
                SCREEN_WIDTH - 320, 225, 12, Color{255, 255, 255, 255});
    }
};

// ============================================================================
// ZMIENNE GLOBALNE
// ============================================================================

// Interaction System
InteractionSystem* g_interactionSystem = nullptr;

// Building System
BuildingSystem* g_buildingSystem = nullptr;
bool isBuildingMode = false;
std::string selectedBlueprintId = "house_4"; // Default to small house (4)
float currentBuildingRotation = 0.0f; // Rotation for building placement

// UI System
UISystem* g_uiSystem = nullptr;

// Time System
TimeCycleSystem* g_timeSystem = nullptr;

// Controllers dla obu kamer
CameraController* freeCameraController = nullptr;
CameraController* settlerCameraController = nullptr;

// Debug visualization variables
bool showDebugInfo = false;
bool showCollisionBoxes = false;
bool showPathfindingDebug = false;

// Edge case variables
bool showInaccessibleWarning = false;
float inaccessibleWarningTime = 0.0f;

// Terrain and colony
Terrain terrain;
NavigationGrid navigationGrid(100, 100, 1.0f); // Initialize Navigation Grid
Colony colony;

// Command queue variables
std::queue<std::pair<Settler*, Vector3>> commandQueue;
bool showCommandQueue = false;

// Status indicator variables
float healthBarWidth = 50.0f;
float healthBarHeight = 5.0f;
float energyBarWidth = 50.0f;
float energyBarHeight = 5.0f;

// Movement path variables
std::vector<Vector3> currentPath;
bool showPath = false;

// Multi-character selection variables
bool isDraggingSelectionBox = false;
Vector2 selectionBoxStart;
Vector2 selectionBoxEnd;

// Selection and command variables
bool isCharacterSelected = false;
Settler* selectedCharacter = nullptr;
bool isDragging = false;
Vector2 dragStartPos;
Vector2 dragEndPos;
Vector3 currentCommandTarget;
bool hasCommandTarget = false;

// Camera follow variables
float cameraFollowSpeed = 5.0f;
Vector3 cameraTargetPosition;
Vector3 cameraTargetTarget;

// ============================================================================
// FUNKCJE POMOCNICZE
// ============================================================================

// Get mouse position in world coordinates
Vector3 GetMouseWorldPosition(Camera3D cam) {
    Vector2 mousePosition = GetMousePosition();
    Ray ray = GetMouseRay(mousePosition, cam);
    
    // Calculate intersection with ground plane (y=0)
    if (ray.direction.y != 0) {
        float t = -ray.position.y / ray.direction.y;
        if (t >= 0) {
            return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
        }
    }
    
    return { 0, 0, 0 };
}

// ============================================================================
// LOGIKA GRY
// ============================================================================

// Item structure moved to Colony.h

std::vector<WorldItem> worldDroppedItems;

void DropItemAt(Vector3 pos, Item* item) {
    if (!item) return;
    // We need to convert raw pointer to unique_ptr
    // Assuming item was allocated with new and we take ownership
    worldDroppedItems.emplace_back(pos, std::unique_ptr<Item>(item), (float)GetTime(), false, 1);
}

void updateGameLogic() {
    // Check drops from settlers
    for (auto settler : colony.getSettlers()) {
        if (settler->pendingDropItem) {
            // Drop slightly in front
            Vector3 dropPos = settler->position;
            dropPos.x += 1.0f; // Offset
            
            DropItemAt(dropPos, settler->pendingDropItem);
            settler->pendingDropItem = nullptr;
        }
    }
    
    // Aktualizacja terenu (usuwanie martwych drzew)
    terrain.update();

    // Update Navigation Grid
    std::vector<BuildingInstance*> buildings;
    if (g_buildingSystem) {
        buildings = g_buildingSystem->getBuildingsInRange({0,0,0}, 10000.0f);
    }
    
    std::vector<Tree*> treePtrs;
    for (const auto& t : terrain.getTrees()) {
        if (t) treePtrs.push_back(t.get());
    }
    
    navigationGrid.UpdateGrid(buildings, treePtrs, terrain.getResourceNodes());

    // Projectiles are now updated in colony.update()
    
    // Update systems
    GameEngine::getInstance().update(deltaTime);
    
    // Update Colony
    // Ensure we pass the right building list format
    std::vector<BuildingInstance*> buildingPtrs;
    if (g_buildingSystem) {
        buildingPtrs = g_buildingSystem->getBuildingsInRange({0,0,0}, 10000.0f);
    }
    
    // Pass raw pointers to buildings as expected by updated Colony::update signature
    colony.update(deltaTime, terrain.getTrees(), worldDroppedItems, buildingPtrs);
}

// ============================================================================
// RENDEROWANIE
// ============================================================================

void renderScene() {
    Camera3D currentCam = cameraMode ? settlerCamera : sceneCamera;
    
    BeginMode3D(currentCam);
        // Draw terrain
        terrain.render();
        
        // Draw colony
        colony.render(cameraMode, selectedCharacter);
        
        // Draw items on ground
        for (const auto& wItem : worldDroppedItems) {
            if (wItem.item) {
                // Simple representation
                Color color = WHITE;
                if (wItem.item->getItemType() == ItemType::RESOURCE) color = BROWN;
                else if (wItem.item->getItemType() == ItemType::TOOL) color = GRAY;
                else if (wItem.item->getItemType() == ItemType::EQUIPMENT) color = RED;
                
                Vector3 itemPos = wItem.position;
                itemPos.y += 0.25f;
                DrawCube(itemPos, 0.5f, 0.5f, 0.5f, color);
                DrawCubeWires(itemPos, 0.5f, 0.5f, 0.5f, BLACK);
            }
        }
        
        // Draw building preview if in building mode
        if (isBuildingMode && g_buildingSystem) {
            Vector3 mousePos = GetMouseWorldPosition(currentCam);
            g_buildingSystem->renderPreview(selectedBlueprintId, mousePos, currentBuildingRotation);
        }
        
        // Draw buildings (active tasks and completed)
        if (g_buildingSystem) {
            g_buildingSystem->render();
        }
        
        // Draw interaction system (highlights)
        if (g_interactionSystem) {
            g_interactionSystem->render();
        }
        
        // Draw command target
        if (hasCommandTarget) {
            DrawCylinder(currentCommandTarget, 0.5f, 0.5f, 0.1f, 10, Color{0, 255, 0, 150});
        }

        // Projectiles are now rendered in colony.render()
        
        // Draw debug
        DrawGrid(20, 1.0f);
        
    EndMode3D();
    
    // 2D UI Overlay
    if (g_interactionSystem) {
        g_interactionSystem->renderUI(currentCam);
    }
    
    // Selection box (2D)
    if (isDraggingSelectionBox) {
        float x = std::min(selectionBoxStart.x, selectionBoxEnd.x);
        float y = std::min(selectionBoxStart.y, selectionBoxEnd.y);
        float width = std::abs(selectionBoxEnd.x - selectionBoxStart.x);
        float height = std::abs(selectionBoxEnd.y - selectionBoxStart.y);
        
        DrawRectangleLines((int)x, (int)y, (int)width, (int)height, GREEN);
        DrawRectangle((int)x, (int)y, (int)width, (int)height, Color{0, 255, 0, 50});
    }
    
    // --- NEW UI SYSTEM INTEGRATION ---
    if (g_uiSystem) {
        // Top Resource Bar (Mock data for now, should come from Colony/Inventory)
        g_uiSystem->DrawResourceBar(100, 50, 10, SCREEN_WIDTH);
        
        // Bottom Action Panel
        g_uiSystem->DrawBottomPanel(isBuildingMode, cameraMode, SCREEN_WIDTH, SCREEN_HEIGHT);
        
        // Building Selection (if active)
        if (isBuildingMode && g_buildingSystem) {
            g_uiSystem->DrawBuildingSelectionPanel(g_buildingSystem->getAvailableBlueprints(), selectedBlueprintId, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
        
        // Selection Info
        auto selectedSettlers = colony.getSelectedSettlers();
        g_uiSystem->DrawSelectionInfo(selectedSettlers, SCREEN_WIDTH, SCREEN_HEIGHT);

        // Building Info (if selected)
        BuildingInstance* selectedBuilding = g_uiSystem->getSelectedBuilding();
        if (selectedBuilding) {
             g_uiSystem->ShowBuildingInfo(selectedBuilding, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
        
        // Stats Overlay (TAB key)
        if (IsKeyDown(KEY_TAB)) {
            g_uiSystem->DrawSettlerStatsOverlay(colony.getSettlers(), currentCam, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    }
    
    // Debug info
    if (freeCameraController && freeCameraController->isDebugEnabled()) {
        if (cameraMode) settlerCameraController->renderDebug();
        else freeCameraController->renderDebug();
    }
}

void processInput() {
    // Handle camera input
    if (cameraMode) {
        // Settler camera mode
        if (settlerCameraController) {
            // Update yaw/pitch based on input if needed, or lock to settler view
            // settlerCameraController->update(); 
        }
    } else {
        // Free camera mode
        if (freeCameraController) {
            freeCameraController->update();
        }
    }

    // Toggle Camera Mode (Space)
    if (IsKeyPressed(KEY_SPACE)) {
        cameraMode = !cameraMode;
        
        if (cameraMode) {
            // Switch to FPS mode - select first settler if none selected
            if (!selectedCharacter && !colony.getSettlers().empty()) {
                selectedCharacter = colony.getSettlers()[0];
                selectedCharacter->SetSelected(true);
            }
            
            if (selectedCharacter) {
                // Setup camera for FPS
                settlerCamera.position = Vector3Add(selectedCharacter->getPosition(), {0, 1.7f, 0});
                settlerCamera.target = Vector3Add(selectedCharacter->getPosition(), {0, 1.7f, 1.0f}); // Look forward
                settlerCamera.up = {0, 1, 0};
                
                // Reset controller
                settlerCameraController->resetState();
            } else {
                cameraMode = false; // Failed to switch
            }
        } else {
            // Switch to Free mode
            // sceneCamera.target = selectedCharacter ? selectedCharacter->getPosition() : Vector3{0,0,0};
            // Keep position, just switch control
            freeCameraController->resetState();
        }
    }

    // Handle interaction input (Right Click) via System
    if (g_interactionSystem) {
        g_interactionSystem->beginFrame();
        Camera3D currentCam = cameraMode ? settlerCamera : sceneCamera;
        g_interactionSystem->processPlayerInput(currentCam);
    }

    // Handle Building Mode Toggle (B key)
    if (IsKeyPressed(KEY_B)) {
        isBuildingMode = !isBuildingMode;
        if (isBuildingMode) {
            printf("Building Mode ON\n");
        } else {
            printf("Building Mode OFF\n");
        }
    }

    // Handle Building Rotation (R key)
    if (isBuildingMode && IsKeyPressed(KEY_R)) {
        currentBuildingRotation += 90.0f;
        if (currentBuildingRotation >= 360.0f) currentBuildingRotation -= 360.0f;
    }

    // Handle Building Placement (Left Click)
    if (isBuildingMode && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_buildingSystem) {
        Camera3D currentCam = cameraMode ? settlerCamera : sceneCamera;
        Vector3 buildPos = GetMouseWorldPosition(currentCam);
        
        // Determine builders (all selected settlers)
        auto selectedSettlers = colony.getSelectedSettlers();
        
        Settler* builder = nullptr;
        if (!selectedSettlers.empty()) {
            builder = selectedSettlers[0];
        }
        
        bool success = false;
        // Note: We use 'false' for instant build to simulate construction process
        // forceNoSnap = false, ignoreCollision = false
        g_buildingSystem->startBuilding(selectedBlueprintId, buildPos, builder, currentBuildingRotation, false, false, false, &success);
        
        if (success) {
            printf("Started building %s at (%.1f, %.1f)\n", selectedBlueprintId.c_str(), buildPos.x, buildPos.z);
        } else {
            printf("Cannot build here!\n");
        }
    }

    // Handle Unit Selection (Left Click / Drag)
    if (!isBuildingMode && !cameraMode) { // Only in free mode and not building
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            isDraggingSelectionBox = true;
            selectionBoxStart = GetMousePosition();
            selectionBoxEnd = selectionBoxStart;
        }
        
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && isDraggingSelectionBox) {
            selectionBoxEnd = GetMousePosition();
        }
        
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && isDraggingSelectionBox) {
            isDraggingSelectionBox = false;
            selectionBoxEnd = GetMousePosition();
            
            // Calculate selection rect
            float x = std::min(selectionBoxStart.x, selectionBoxEnd.x);
            float y = std::min(selectionBoxStart.y, selectionBoxEnd.y);
            float width = std::abs(selectionBoxEnd.x - selectionBoxStart.x);
            float height = std::abs(selectionBoxEnd.y - selectionBoxStart.y);
            Rectangle selectionRect = { x, y, width, height };
            
            // Clear previous selection unless Shift is held (Add to selection)
            if (!IsKeyDown(KEY_LEFT_SHIFT)) {
                colony.clearSelection();
                selectedCharacter = nullptr;
            }
            
            // Check for single click vs box select
            if (width < 5 && height < 5) {
                // Single click selection
                // Raycast to find settler
                Ray ray = GetMouseRay(selectionBoxStart, sceneCamera);
                std::string hitInfo;
                if (colony.checkHit(ray, 100.0f, hitInfo)) {
                    float minD = 1000.0f;
                    Settler* hitSettler = nullptr;
                    for (auto* s : colony.getSettlers()) {
                        Vector3 sPos = s->getPosition();
                        RayCollision col = GetRayCollisionSphere(ray, sPos, 1.0f);
                        if (col.hit && col.distance < minD) {
                            minD = col.distance;
                            hitSettler = s;
                        }
                    }
                    
                    if (hitSettler) {
                        hitSettler->SetSelected(true);
                        selectedCharacter = hitSettler;
                        printf("Selected: %s\n", hitSettler->getName().c_str());
                    }
                }
                
                // Check building selection
                if (!selectedCharacter && g_buildingSystem) {
                    BuildingInstance* clickedBuilding = nullptr;
                    if (g_buildingSystem->getBuildingAtRay(ray, &clickedBuilding)) {
                        if (clickedBuilding && g_uiSystem) {
                            g_uiSystem->SelectBuilding(clickedBuilding);
                        }
                    } else if (g_uiSystem) {
                        g_uiSystem->DeselectBuilding();
                    }
                }
                
            } else {
                // Box selection
                for (auto* settler : colony.getSettlers()) {
                    Vector3 sPos = settler->getPosition();
                    Vector2 screenPos = GetWorldToScreen(sPos, sceneCamera);
                    
                    if (CheckCollisionPointRec(screenPos, selectionRect)) {
                        settler->SetSelected(true);
                        // Update last selected for camera tracking
                        selectedCharacter = settler; 
                    }
                }
                printf("Box selection finished\n");
            }
        }
    }

    // === NEW: RIGHT CLICK MOVEMENT FOR SETTLERS ===
    // If no building mode and settlers selected, right click moves them
    if (!isBuildingMode && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        auto selectedSettlers = colony.getSelectedSettlers();
        if (!selectedSettlers.empty()) {
            Camera3D currentCam = cameraMode ? settlerCamera : sceneCamera;
            Vector3 targetPos = GetMouseWorldPosition(currentCam);
            
            printf("DEBUG: Right click at (%.1f, %.1f). Selected settlers: %zu\n", targetPos.x, targetPos.z, selectedSettlers.size());

            for (auto* settler : selectedSettlers) {
                // Use assignTask with TaskType::MOVE
                settler->assignTask(TaskType::MOVE, nullptr, targetPos);
                printf("DEBUG: Assigned MOVE task to settler %s to (%.1f, %.1f)\n", settler->getName().c_str(), targetPos.x, targetPos.z);
            }
            
            // Visual feedback
            currentCommandTarget = targetPos;
            hasCommandTarget = true;
        }
    }
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "RTS Colony Sim - Enhanced System");
    SetTargetFPS(60);

    // Initialize Global Navigation Grid
    GameSystem::setNavigationGrid(&navigationGrid);

    // Initialize Camera
    sceneCamera.position = { 10.0f, 10.0f, 10.0f };
    sceneCamera.target = { 0.0f, 0.0f, 0.0f };
    sceneCamera.up = { 0.0f, 1.0f, 0.0f };
    sceneCamera.fovy = 45.0f;
    sceneCamera.projection = CAMERA_PERSPECTIVE;

    // Initialize Settler Camera (FPS)
    settlerCamera.position = { 0.0f, 1.7f, 0.0f };
    settlerCamera.target = { 0.0f, 1.7f, 1.0f };
    settlerCamera.up = { 0.0f, 1.0f, 0.0f };
    settlerCamera.fovy = 60.0f;
    settlerCamera.projection = CAMERA_PERSPECTIVE;

    // Create Controllers
    freeCameraController = new CameraController(&sceneCamera);
    settlerCameraController = new CameraController(&settlerCamera);

    // Initialize Systems via GameEngine
    GameEngine& engine = GameEngine::getInstance();
    
    // Inventory
    auto inventorySystemPtr = std::make_unique<InventorySystem>();
    engine.registerSystem(std::move(inventorySystemPtr));
    
    // Time
    auto timeSystemPtr = std::make_unique<TimeCycleSystem>();
    g_timeSystem = timeSystemPtr.get();
    engine.registerSystem(std::move(timeSystemPtr));

    // Building System
    auto buildingSystemPtr = std::make_unique<BuildingSystem>();
    g_buildingSystem = buildingSystemPtr.get();
    engine.registerSystem(std::move(buildingSystemPtr)); // Register unique_ptr directly
    
    // Storage System
    auto storageSystemPtr = std::make_unique<StorageSystem>();
    StorageSystem* g_storageSystem = storageSystemPtr.get();
    engine.registerSystem(std::move(storageSystemPtr));

    // UI System
    auto uiSystemPtr = std::make_unique<UISystem>();
    g_uiSystem = uiSystemPtr.get();
    engine.registerSystem(std::move(uiSystemPtr));

    // Interaction System (Init before Terrain/Colony to allow object registration)
    auto interactionSystemPtr = std::make_unique<InteractionSystem>();
    g_interactionSystem = interactionSystemPtr.get();
    
    // Register immediately so it's available for Terrain and Colony during generation/initialization
    engine.registerSystem(std::move(interactionSystemPtr));
    
    // Initialize Terrain (now can register trees)
    terrain.generate(100, 100, 1.0f);
    
    // Initialize Colony (now can register settlers)
    colony.initialize();
    
    // Manual dependency injection for interaction system (needs raw pointers)
    g_interactionSystem->setColony(&colony);
    g_interactionSystem->setBuildingSystem(g_buildingSystem);
    // Register global callbacks
    GameEngine::dropItemCallback = DropItemAt;

    // Post-init setup
    if (g_buildingSystem) {
        g_buildingSystem->setInteractionSystem(g_interactionSystem);
        g_buildingSystem->setColony(&colony);
        g_buildingSystem->setStorageSystem(g_storageSystem);
    }

    // Main game loop
    while (!WindowShouldClose()) {
        // Delta time
        deltaTime = GetFrameTime();

        // Input
        processInput();

        // Logic
        updateGameLogic();

        // Render
        BeginDrawing();
            ClearBackground(SKYBLUE);
            renderScene();
            DrawFPS(10, 10);
        EndDrawing();
    }

    // Cleanup
    delete freeCameraController;
    delete settlerCameraController;
    
    // Systems cleaned up by GameEngine destructor or unique_ptrs
    
    CloseWindow();

    return 0;
}
