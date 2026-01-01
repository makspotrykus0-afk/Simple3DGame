#include "Terrain.h"
#include "Colony.h"
#include "Tree.h"
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
#include "../game/Item.h"
#include "../components/StatsComponent.h"
#include "../core/GameSystem.h"
#include "../core/IGameSystem.h"
#include "../systems/BuildingSystem.h"
#include "../game/BuildingBlueprint.h"
#include "../core/GameEngine.h"
#include "../systems/UISystem.h"
#include "../systems/InventorySystem.h"
#include "../systems/TimeCycleSystem.h"
#include "../systems/StorageSystem.h"
#include "../systems/NeedsSystem.h"
#include "../systems/CraftingSystem.h"
#include "../game/NavigationGrid.h"
const int SCREEN_WIDTH  = 1200;
const int SCREEN_HEIGHT = 800;

// Camera view mode enum for 3 camera types (CameraMode conflicts with raylib)
enum class CameraViewMode {
    ISOMETRIC,  // Free isometric camera (default)
    TPS,        // Third Person - behind settler
    FPS         // First Person - from settler's eyes
};

Camera3D       sceneCamera;
Camera3D       settlerCamera;
float          deltaTime           = 0.0f;
CameraViewMode currentCameraMode   = CameraViewMode::ISOMETRIC;
Settler*       controlledSettler   = nullptr; // Settler under player control
float          tpsYaw              = PI;      // TPS Camera Orbit Yaw (starting behind)
float          tpsPitch            = 0.3f;    // TPS Camera Orbit Pitch
float          tpsRadius           = 5.0f;    // TPS Camera Orbit Radius
float          fpsPitch            = 0.0f;    // FPS Camera Looking Pitch
bool           tpsIsRotating       = false;   // TPS Camera Orbit State
float          g_fovyZoom          = 45.0f;   // Camera FOV for Zoom/Scope
class CameraController {
public:
CameraController(Camera3D* camera)
: m_camera(camera), m_debugEnabled(false), m_lastMousePosition({0,0}), m_isRotating(false), m_yaw(0.0f), m_pitch(0.0f), m_moveSpeed(15.0f), m_radius(0.0f) {
// Inicjalizacja yaw/pitch na podstawie aktualnego kierunku kamery
Vector3 dir = Vector3Subtract(m_camera->position, m_camera->target);
float radius = Vector3Length(dir);
m_radius = radius;
if (radius > 0.0f) {
m_yaw = atan2f(dir.x, dir.z);
m_pitch = asinf(dir.y / radius);
}
}
void update(float deltaTime) {
// [CAMERA MOVE] DEBUG: Początek update()
// std::cout << "[CAMERA MOVE] update() called, dt=" << deltaTime
// << " pos=(" << m_camera->position.x << "," << m_camera->position.y << "," << m_camera->position.z << ")" << std::endl;
// ZOOM na scroll myszy
float wheel = GetMouseWheelMove();
if (wheel != 0.0f) {
const float ZOOM_SPEED = 2.0f;
const float MIN_ZOOM = 2.0f;
const float MAX_ZOOM = 50.0f;
float oldRadius = m_radius;
m_radius -= wheel * ZOOM_SPEED;
if (m_radius < MIN_ZOOM) m_radius = MIN_ZOOM;
if (m_radius > MAX_ZOOM) m_radius = MAX_ZOOM;
// Po zmianie radius należy zaktualizować pozycję kamery
updateCameraPosition();
// std::cout << "[CAMERA ZOOM] wheel=" << wheel << " radius " << oldRadius << " -> " << m_radius << std::endl;
}
        // Translacja kamery równolegle do terenu – WASD/QE przesuwają position i target razem
        Vector3 move = { 0.0f, 0.0f, 0.0f };
        float speed = m_moveSpeed * deltaTime;
        
        // Shift doubles speed
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            speed *= 2.0f;
        }
        
        // Oblicz surowy wektor forward (z Y)
        Vector3 rawForward = Vector3Subtract(m_camera->target, m_camera->position);
        // FLATTEN: Spłaszczenie forward do płaszczyzny XZ (Y=0)
        Vector3 flatForward = { rawForward.x, 0.0f, rawForward.z };
        float flatLen = Vector3Length(flatForward);
        if (flatLen > 0.0001f) {
            flatForward = Vector3Normalize(flatForward);
        } else {
            // Jeśli kamera patrzy prosto w górę/dół, użyj fallback
            flatForward = { 0.0f, 0.0f, -1.0f };
        }
        // Oblicz wektor right od spłaszczonego forward
        Vector3 right = Vector3Normalize(Vector3CrossProduct(flatForward, {0.0f, 1.0f, 0.0f}));
        Vector3 up = {0.0f, 1.0f, 0.0f};
        if (IsKeyDown(KEY_W)) {
            std::cout << "[CAMERA MOVE] W pressed, before move pos=(" << m_camera->position.x << "," << m_camera->position.y << "," << m_camera->position.z
            << "), flatForward=(" << flatForward.x << "," << flatForward.y << "," << flatForward.z << ")" << std::endl;
            move = Vector3Add(move, flatForward);
        }
        if (IsKeyDown(KEY_S)) {
            std::cout << "[CAMERA MOVE] S pressed, before move pos=(" << m_camera->position.x << "," << m_camera->position.y << "," << m_camera->position.z
            << "), flatForward=(" << flatForward.x << "," << flatForward.y << "," << flatForward.z << ")" << std::endl;
            move = Vector3Subtract(move, flatForward);
        }
        if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
        if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
        if (IsKeyDown(KEY_Q)) move = Vector3Subtract(move, up);
        if (IsKeyDown(KEY_E)) move = Vector3Add(move, up);
        if (move.x != 0.0f || move.y != 0.0f || move.z != 0.0f) {
            move = Vector3Normalize(move);
            move = Vector3Scale(move, speed);
            m_camera->position = Vector3Add(m_camera->position, move);
            m_camera->target   = Vector3Add(m_camera->target, move);
std::cout << "[CAMERA MOVE] after move pos=(" << m_camera->position.x << "," << m_camera->position.y << "," << m_camera->position.z
<< "), move=(" << move.x << "," << move.y << "," << move.z << ")" << std::endl;
}
// Obrót kamery TYLKO na ŚRODKOWY przycisk myszy – prawdziwa orbita równoległa do terenu
if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
if (!m_isRotating) {
m_isRotating = true;
HideCursor();
}
Vector2 delta = GetMouseDelta() * 0.003f;
Vector3 pivot = m_camera->target;
Vector3 dir = Vector3Subtract(m_camera->position, pivot);
// Aktualizuj radius na podstawie aktualnej odległości (może się zmienić przez zoom)
m_radius = Vector3Length(dir);
m_yaw += delta.x;
m_pitch -= delta.y;
m_pitch = Clamp(m_pitch, -1.4f, 1.4f);
updateCameraPosition();
// std::cout << "[DEBUG] ORBIT yaw:" << m_yaw << " pitch:" << m_pitch << " radius:" << m_radius << std::endl;
} else if (m_isRotating) {
m_isRotating = false;
ShowCursor();
}
}
bool isDebugEnabled() const { return m_debugEnabled; }
void renderDebug() const {
DrawText(TextFormat("Cam pos: %.1f, %.1f, %.1f", m_camera->position.x, m_camera->position.y, m_camera->position.z), 10, 50, 20, DARKGRAY);
}
private:
void updateCameraPosition() {
Vector3 pivot = m_camera->target;
float xz = m_radius * cosf(m_pitch);
m_camera->position.x = pivot.x + xz * sinf(m_yaw);
m_camera->position.y = pivot.y + m_radius * sinf(m_pitch);
m_camera->position.z = pivot.z + xz * cosf(m_yaw);
}
Camera3D* m_camera;
bool m_debugEnabled;
Vector2 m_lastMousePosition;
bool m_isRotating;
float m_yaw;
float m_pitch;
float m_moveSpeed;
float m_radius;
};
float m_pitch;
float m_moveSpeed;
InteractionSystem* g_interactionSystem = nullptr;
BuildingSystem*    g_buildingSystem    = nullptr;
CraftingSystem*    g_craftingSystem    = nullptr;
UISystem*          g_uiSystem          = nullptr;
TimeCycleSystem*   g_timeSystem        = nullptr;
// Global State
bool               isBuildingMode      = false;
std::string        selectedBlueprintId = "house_4";
float              currentBuildingRotation = 0.0f;
CameraController*  freeCameraController     = nullptr;
CameraController*  settlerCameraController  = nullptr;
bool               showDebugInfo            = false;
bool               showCollisionBoxes       = false;
bool               showPathfindingDebug     = false;
bool               showInaccessibleWarning  = false;
float              inaccessibleWarningTime  = 0.0f;
bool               g_drawDebugGrid          = false;
float              globalTimeScale          = 1.0f; // Globalny współczynnik skalowania czasu gry
Terrain            terrain;
NavigationGrid     navigationGrid(100, 100, 1.0f);
Colony             colony;
std::queue<std::pair<Settler*, Vector3>> commandQueue;
bool               showCommandQueue = false;
float              healthBarWidth   = 50.0f;
float              healthBarHeight  = 5.0f;
float              energyBarWidth   = 50.0f;
float              energyBarHeight  = 5.0f;
std::vector<Vector3> currentPath;
bool                 showPath             = false;
bool                 isDraggingSelectionBox = false;
Vector2              selectionBoxStart;
Vector2              selectionBoxEnd;
bool                 isCharacterSelected = false;
Settler*             selectedCharacter   = nullptr;
bool                 isDragging          = false;
Vector2              dragStartPos;
Vector2              dragEndPos;
Vector3              currentCommandTarget;
bool                 hasCommandTarget = false;
float                cameraFollowSpeed  = 5.0f;
Vector3              cameraTargetPosition;
Vector3              cameraTargetTarget;
// Helper to get mouse world position (simple ground plane)
Vector3 GetMouseWorldPosition(Camera3D camera) {
Ray ray = GetMouseRay(GetMousePosition(), camera);
float groundY = 0.0f;
if (ray.direction.y != 0.0f) {
float t = (groundY - ray.position.y) / ray.direction.y;
if (t > 0) { return Vector3Add(ray.position, Vector3Scale(ray.direction, t)); }
}
return ray.position;
}
void renderScene() {
Camera3D currentCam = sceneCamera;
BeginMode3D(currentCam);
terrain.render();
colony.render(currentCameraMode == CameraViewMode::FPS, controlledSettler);
const std::vector<WorldItem>& droppedItems = colony.getDroppedItems();
for (const auto& wItem : droppedItems) {
if (wItem.item) {
Color color = WHITE;
if (wItem.item->getItemType() == ItemType::RESOURCE) color = BROWN;
else if (wItem.item->getItemType() == ItemType::TOOL) color = GRAY;
else if (wItem.item->getItemType() == ItemType::EQUIPMENT) color = BLUE;
else if (wItem.item->getItemType() == ItemType::CONSUMABLE) {
    color = GREEN;
    if (wItem.item->getDisplayName() == "Raw Meat") color = MAROON; // Mięso - ciemnoczerwone
}
Vector3 itemPos = wItem.position;
itemPos.y += 0.25f;
DrawCube(itemPos, 0.5f, 0.5f, 0.5f, color);
DrawCubeWires(itemPos, 0.5f, 0.5f, 0.5f, BLACK);
}
}

if (isBuildingMode && g_buildingSystem) {
    Vector3 mousePos = GetMouseWorldPosition(currentCam);
    g_buildingSystem->renderPreview(selectedBlueprintId, mousePos, currentBuildingRotation);
}

if (g_buildingSystem) g_buildingSystem->render();
if (g_interactionSystem) g_interactionSystem->render();
if (hasCommandTarget)
    DrawCylinder(currentCommandTarget, 0.5f, 0.5f, 0.1f, 10, Color{0, 255, 0, 150});
if (g_drawDebugGrid) {
    DrawGrid(20, 1.0f);
}
EndMode3D();
// Call UI render (debug)
if (g_interactionSystem) { g_interactionSystem->renderUI(currentCam); }

// Draw Crosshair in TPS/FPS modes
if (currentCameraMode == CameraViewMode::TPS || currentCameraMode == CameraViewMode::FPS) {
    int centerX = GetScreenWidth() / 2;
    int centerY = GetScreenHeight() / 2;
    
    // Outer circle
    DrawCircleLines(centerX, centerY, 10, BLACK);
    // Inner dot
    DrawCircle(centerX, centerY, 2, RED);
    
    // Lines
    DrawLine(centerX - 15, centerY, centerX - 5, centerY, BLACK);
    DrawLine(centerX + 5, centerY, centerX + 15, centerY, BLACK);
    DrawLine(centerX, centerY - 15, centerX, centerY - 5, BLACK);
    DrawLine(centerX, centerY + 5, centerX, centerY + 15, BLACK);
}


}
void processInput() {
Camera3D currentCam = sceneCamera;
// UI clicks handling
if (g_uiSystem && g_uiSystem->IsMouseOverUI()) {
if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
// Handle selection panel tabs
g_uiSystem->HandleSelectionPanelClick(SCREEN_WIDTH, SCREEN_HEIGHT);
// Handle crafting panel
g_uiSystem->HandleCraftingPanelClick(SCREEN_WIDTH, SCREEN_HEIGHT);
}
// If mouse is over UI, skip further interaction processing
return;
}
// Process interaction system input (only when mouse is NOT over UI)
if (g_interactionSystem) {
g_interactionSystem->processPlayerInput(currentCam);
// Handle selection with left mouse click (only when not in building mode)
if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isBuildingMode) {
Ray ray = GetMouseRay(GetMousePosition(), currentCam);
g_interactionSystem->handleSelection(ray);
}
}
// TimeCycleSystem speed control
if (g_timeSystem) {
if (IsKeyPressed(KEY_ONE)) {
globalTimeScale = 1.0f;
g_timeSystem->setTimeScale(globalTimeScale * 100.0f);
std::cout << "[TIME] Global time scale set to " << globalTimeScale << " (normal speed)" << std::endl;
}
if (IsKeyPressed(KEY_TWO)) {
globalTimeScale = 2.0f;
g_timeSystem->setTimeScale(globalTimeScale * 100.0f);
std::cout << "[TIME] Global time scale set to " << globalTimeScale << " (2x speed)" << std::endl;
}
if (IsKeyPressed(KEY_THREE)) {
globalTimeScale = 4.0f;
g_timeSystem->setTimeScale(globalTimeScale * 100.0f);
std::cout << "[TIME] Global time scale set to " << globalTimeScale << " (4x speed)" << std::endl;
}
}
// Building mode toggle
if (IsKeyPressed(KEY_B)) {
isBuildingMode = !isBuildingMode;
if (g_buildingSystem) {
g_buildingSystem->enablePlanningMode(isBuildingMode);
if (isBuildingMode) {
auto blueprints = g_buildingSystem->getAvailableBlueprints();
if (!blueprints.empty() && selectedBlueprintId.empty()) {
selectedBlueprintId = blueprints[0]->getId();
}
}
}
}
// Blueprint cycling with TAB
if (isBuildingMode && IsKeyPressed(KEY_TAB) && g_buildingSystem) {
auto blueprints = g_buildingSystem->getAvailableBlueprints();
if (!blueprints.empty()) {
int currentIndex = -1;
for (size_t i = 0; i < blueprints.size(); ++i) {
if (blueprints[i]->getId() == selectedBlueprintId) {
currentIndex = static_cast<int>(i);
break;
}
}
int nextIndex       = (currentIndex + 1) % blueprints.size();
selectedBlueprintId = blueprints[nextIndex]->getId();
g_buildingSystem->setSelectedBlueprint(selectedBlueprintId);
}
}
// Building placement with left click in building mode
if (isBuildingMode && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_buildingSystem) {
Vector3 mousePos = GetMouseWorldPosition(currentCam);
bool    success  = false;
g_buildingSystem->startBuilding(selectedBlueprintId, mousePos, nullptr,
currentBuildingRotation, false, false, false, &success);
}
// Crafting system - queue a task with C key
if (IsKeyPressed(KEY_C) && g_craftingSystem) {
int taskId = g_craftingSystem->queueTask("stone_axe");
(void)taskId;
}

    // Camera mode switching with V key
    if (IsKeyPressed(KEY_V)) {
        // Cycle through camera modes: ISOMETRIC -> TPS -> FPS -> ISOMETRIC
        if (currentCameraMode == CameraViewMode::ISOMETRIC) {
            currentCameraMode = CameraViewMode::TPS;
            // Select first settler if none selected
            auto settlers = colony.getSettlers();
            if (!settlers.empty() && !controlledSettler) {
                controlledSettler = settlers[0];
            }
            // Enable player control
            if (controlledSettler) {
                controlledSettler->setPlayerControlled(true);
                std::cout << "[CAMERA] Switched to TPS mode - Player control ENABLED" << std::endl;
            }
        } else if (currentCameraMode == CameraViewMode::TPS) {
            currentCameraMode = CameraViewMode::FPS;
            DisableCursor(); // Hide mouse in FPS
            std::cout << "[CAMERA] Switched to FPS mode - Mouse HIDDEN" << std::endl;
        } else if (currentCameraMode == CameraViewMode::FPS) {
            currentCameraMode = CameraViewMode::ISOMETRIC;
            EnableCursor(); // Show mouse back
            // Disable player control
            if (controlledSettler) {
                controlledSettler->setPlayerControlled(false);
                std::cout << "[CAMERA] Switched to ISOMETRIC mode - Player control DISABLED" << std::endl;
            }
            controlledSettler = nullptr;
        }
    }

// Toggle debug grid with F3
if (IsKeyPressed(KEY_F3)) {
g_drawDebugGrid = !g_drawDebugGrid;
}

    // Player input for settler control in TPS/FPS modes
    if ((currentCameraMode == CameraViewMode::TPS || currentCameraMode == CameraViewMode::FPS) && controlledSettler) {
        float settlerRot = controlledSettler->getRotation();
        
        // MMB Camera Orbiting in TPS mode (also rotates settler)
        if (currentCameraMode == CameraViewMode::TPS && IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            if (!tpsIsRotating) { tpsIsRotating = true; HideCursor(); }
            Vector2 delta = GetMouseDelta();
            tpsYaw += delta.x * 0.003f;
            tpsPitch -= delta.y * 0.003f;
            tpsPitch = Clamp(tpsPitch, -0.2f, 1.2f); // Limit vertical orbit
            
            // Sync settler rotation with camera yaw (so body turns with camera)
            // Offset PI because tpsYaw=PI is behind the player (looking forward)
            controlledSettler->setRotationFromMouse(tpsYaw - PI);
        } else if (tpsIsRotating && currentCameraMode == CameraViewMode::TPS) {
            tpsIsRotating = false;
            ShowCursor();
        }

        // Mouse rotation in FPS mode (full look, cursor hidden)
        if (currentCameraMode == CameraViewMode::FPS) {
            Vector2 mouseDelta = GetMouseDelta();
            if (mouseDelta.x != 0 || mouseDelta.y != 0) {
                // Yaw rotates the settler body - inverted sense to match mouse movement
                float yawChange = -mouseDelta.x * 0.003f; 
                controlledSettler->setRotationFromMouse((controlledSettler->getRotation() * DEG2RAD) + yawChange);
                
                // Pitch rotates only the camera view - inverted sign to match mouse movement
                fpsPitch += mouseDelta.y * 0.003f; 
                fpsPitch = Clamp(fpsPitch, -1.4f, 1.4f); // Limit up/down
            }
        }

        // Scope/Zoom handling with Right Mouse Button
        static float currentFov = 45.0f;
        float targetFov = 45.0f;

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            targetFov = 10.0f; // Stronger zoom (4.5x)
            controlledSettler->setScoping(true);
        } else {
            targetFov = 45.0f;
            controlledSettler->setScoping(false);
        }
        
        // Płynne FOV (Lerp) - dopasowane do lerpa broni (10.0f)
        currentFov += (targetFov - currentFov) * 10.0f * deltaTime;
        sceneCamera.fovy = currentFov;

        // WASD movement for settler
        Vector3 settlerPos = controlledSettler->getPosition();
        Vector3 moveDir = {0, 0, 0};
        float moveSpeed = 5.0f * deltaTime;

        // Calculate direction relative to camera view
        Vector3 camForward = Vector3Normalize(Vector3Subtract(sceneCamera.target, sceneCamera.position));
        camForward.y = 0.0f; // Flatten to ground plane
        camForward = Vector3Normalize(camForward);
        Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, {0.0f, 1.0f, 0.0f}));

        if (IsKeyDown(KEY_W)) moveDir = Vector3Add(moveDir, camForward);
        if (IsKeyDown(KEY_S)) moveDir = Vector3Subtract(moveDir, camForward);
        if (IsKeyDown(KEY_A)) moveDir = Vector3Add(moveDir, camRight);
        if (IsKeyDown(KEY_D)) moveDir = Vector3Subtract(moveDir, camRight);

        // Apply movement
        if (Vector3Length(moveDir) > 0.001f) {
            moveDir = Vector3Normalize(moveDir);
            Vector3 newPos = Vector3Add(settlerPos, Vector3Scale(moveDir, moveSpeed));
            controlledSettler->setPosition(newPos);
            
            // In TPS, if we move without holding MMB, we might want to face movement?
            // User requested settler rotates with camera, so we keep MMB sync or just general sync.
            // If NOT orbiting, we can face movement direction:
            if (!tpsIsRotating && currentCameraMode == CameraViewMode::TPS) {
                 float targetRot = atan2f(moveDir.x, moveDir.z);
                 controlledSettler->setRotationFromMouse(targetRot);
                 // Keep tpsYaw in sync so camera stays behind when we start orbiting again
                 tpsYaw = targetRot + PI;
            }
            
            controlledSettler->setState(SettlerState::MOVING);
        } else {
            controlledSettler->setState(SettlerState::IDLE);
        }
    }

// Camera controller updates based on current mode
if (currentCameraMode == CameraViewMode::ISOMETRIC) {
if (freeCameraController) freeCameraController->update(deltaTime);
    } else if (currentCameraMode == CameraViewMode::TPS && controlledSettler) {
        // Third-person camera: orbiting around settler
        Vector3 settlerPos = controlledSettler->getPosition();
        Vector3 pivot = Vector3Add(settlerPos, {0.0f, 1.5f, 0.0f}); // Look at settler's head
        
        float xz = tpsRadius * cosf(tpsPitch);
        sceneCamera.position.x = pivot.x + xz * sinf(tpsYaw);
        sceneCamera.position.y = pivot.y + tpsRadius * sinf(tpsPitch);
        sceneCamera.position.z = pivot.z + xz * cosf(tpsYaw);
        sceneCamera.target = pivot;
    } else if (currentCameraMode == CameraViewMode::FPS && controlledSettler) {
        // First-person camera: from settler's eyes
        Vector3 settlerPos = controlledSettler->getPosition();
        float settlerRot = controlledSettler->getRotation();
        
        // Camera at settler's eye height
        Vector3 eyePos = settlerPos;
        eyePos.y += 1.7f; // Eye height
        
        // Look direction calculated from settler rotation and fpsPitch
        Vector3 lookDir;
        // Correct forward calculation (sync with getForwardVector)
        lookDir.x = -sinf(settlerRot * DEG2RAD) * cosf(fpsPitch);
        lookDir.y = -sinf(fpsPitch); // Up/Down
        lookDir.z = -cosf(settlerRot * DEG2RAD) * cosf(fpsPitch);
        
        sceneCamera.position = eyePos;
        sceneCamera.target = Vector3Add(eyePos, lookDir);
}
}
int main() {
InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Simple 3D Game");
SetTargetFPS(60);
// Initialize cameras
sceneCamera           = {0};
sceneCamera.position  = Vector3{10.0f, 10.0f, 10.0f};
sceneCamera.target    = Vector3{0.0f, 0.0f, 0.0f};
sceneCamera.up        = Vector3{0.0f, 1.0f, 0.0f};
sceneCamera.fovy      = 45.0f;
sceneCamera.projection = CAMERA_PERSPECTIVE;
settlerCamera           = {0};
settlerCamera.position  = Vector3{0.0f, 5.0f, 10.0f};
settlerCamera.target    = Vector3{0.0f, 0.0f, 0.0f};
settlerCamera.up        = Vector3{0.0f, 1.0f, 0.0f};
settlerCamera.fovy      = 45.0f;
settlerCamera.projection = CAMERA_PERSPECTIVE;
// Create camera controllers
freeCameraController    = new CameraController(&sceneCamera);
settlerCameraController = new CameraController(&settlerCamera);
// Initialize GameEngine
GameEngine& engine = GameEngine::getInstance();
// Create and register systems
auto buildingSystem    = std::make_unique<BuildingSystem>();
auto interactionSystem = std::make_unique<InteractionSystem>();
auto uiSystem          = std::make_unique<UISystem>();
auto timeSystem        = std::make_unique<TimeCycleSystem>();
auto inventorySystem   = std::make_unique<InventorySystem>();
auto storageSystem     = std::make_unique<StorageSystem>();
auto needsSystem       = std::make_unique<NeedsSystem>();
auto craftingSystem    = std::make_unique<CraftingSystem>();
// Set global pointers
g_buildingSystem    = buildingSystem.get();
g_interactionSystem = interactionSystem.get();
g_uiSystem          = uiSystem.get();
g_timeSystem        = timeSystem.get();
g_craftingSystem    = craftingSystem.get();
// Register systems with engine
engine.registerSystem(std::move(buildingSystem));
engine.registerSystem(std::move(interactionSystem));
engine.registerSystem(std::move(uiSystem));
engine.registerSystem(std::move(timeSystem));
engine.registerSystem(std::move(inventorySystem));
engine.registerSystem(std::move(storageSystem));
engine.registerSystem(std::move(needsSystem));
engine.registerSystem(std::move(craftingSystem));
// Terrain & colony
engine.registerTerrain(&terrain);
terrain.generate(100, 100, 1.0f);
// Set static references for GameSystem
GameSystem::setNavigationGrid(&navigationGrid);
GameSystem::setColony(&colony);
GameSystem::setTerrain(&terrain);
colony.initialize();
// Set drop item callback for trees
GameEngine::dropItemCallback = [](Vector3 position, Item* item) {
colony.addDroppedItem(std::unique_ptr<Item>(item), position);
std::cout << "[GameEngine] Dropped item at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
};
if (g_interactionSystem) {
g_interactionSystem->setColony(&colony);
g_interactionSystem->setUIVisible(true);
std::cout << "[DEBUG] InteractionSystem colony set, UI visible enabled." << std::endl;
}
// CRITICAL: Set colony in UISystem
if (g_uiSystem) {
g_uiSystem->setColony(&colony);
std::cout << "[DEBUG] UISystem colony set." << std::endl;
}
engine.initialize();
// Main loop
// Main loop
while (!WindowShouldClose()) {
deltaTime = GetFrameTime();
processInput();
terrain.update();
float scaledDeltaTime = deltaTime * globalTimeScale;
        colony.update(scaledDeltaTime, terrain.getTrees(),
                      g_buildingSystem->getAllBuildings());
        
        // Update navigation grid with current obstacles
        {
            auto buildings = g_buildingSystem->getAllBuildings();
            
            // Convert unique_ptr<Tree> to Tree*
            std::vector<Tree*> treePtrs;
            const auto& treesRef = terrain.getTrees();
            treePtrs.reserve(treesRef.size());
            for (const auto& treePtr : treesRef) {
                treePtrs.push_back(treePtr.get());
            }
            
            // Resources already use unique_ptr in signature
            const auto& resources = terrain.getResourceNodes();
            
            navigationGrid.UpdateGrid(buildings, treePtrs, resources);
        }
        
        engine.update(scaledDeltaTime);
BeginDrawing();
ClearBackground(RAYWHITE);
renderScene();
engine.render();
// Draw UI elements
if (g_uiSystem) {
int wood = 0;
int food = 0;
int stone = 0;
g_uiSystem->DrawResourceBar(wood, food, stone, SCREEN_WIDTH);
g_uiSystem->DrawBottomPanel(isBuildingMode, currentCameraMode != CameraViewMode::ISOMETRIC, SCREEN_WIDTH, SCREEN_HEIGHT);
auto selectedSettlers = colony.getSelectedSettlers();
if (!selectedSettlers.empty()) {
g_uiSystem->DrawSelectionInfo(selectedSettlers, SCREEN_WIDTH, SCREEN_HEIGHT);
}
}
if (freeCameraController && freeCameraController->isDebugEnabled()) {
freeCameraController->renderDebug();
}
EndDrawing();
}
engine.shutdown();
}
