#include "../components/StatsComponent.h"
#include "../core/GameEngine.h"
#include "../core/GameSystem.h"
#include "../core/IGameSystem.h"
#include "../game/BuildingBlueprint.h"
#include "../game/DebugConsole.h"
#include "../game/Item.h"
#include "../game/NavigationGrid.h"
#include "../systems/BuildingSystem.h"
#include "../systems/CraftingSystem.h"
#include "../systems/EditorSystem.h" // [EDITOR]
#include "../systems/InteractionSystem.h"
#include "../systems/InventorySystem.h"
#include "../systems/NeedsSystem.h"
#include "../systems/StorageSystem.h"
#include "../systems/TimeCycleSystem.h"
#include "../systems/UISystem.h"
#include "Colony.h"
#include "Terrain.h"
#include "Tree.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <utility>
#include <vector>

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;

// Camera view mode enum for 3 camera types (CameraMode conflicts with raylib)
enum class CameraViewMode {
  ISOMETRIC, // Free isometric camera (default)
  TPS,       // Third Person - behind settler
  FPS        // First Person - from settler's eyes
};

Camera3D sceneCamera;
Camera3D settlerCamera;
float deltaTime = 0.0f;
CameraViewMode currentCameraMode = CameraViewMode::ISOMETRIC;
Settler *controlledSettler = nullptr; // Settler under player control
float tpsYaw = PI;                    // TPS Camera Orbit Yaw (starting behind)
float tpsPitch = 0.3f;                // TPS Camera Orbit Pitch
float tpsRadius = 2.5f;     // TPS Camera Orbit Radius (Closer for OTS)
float fpsPitch = 0.0f;      // FPS Camera Looking Pitch
bool tpsIsRotating = false; // TPS Camera Orbit State
float g_fovyZoom = 45.0f;   // Camera FOV for Zoom/Scope
Vector3 smoothCamPos = {0.0f, 0.0f, 0.0f};
Vector3 smoothCamTarget = {0.0f, 0.0f, 0.0f};

// [EDITOR] Global Editor System
EditorSystem g_editorSystem;

class CameraController {
public:
  CameraController(Camera3D *camera)
      : m_camera(camera), m_debugEnabled(false), m_lastMousePosition({0, 0}),
        m_isRotating(false), m_yaw(0.0f), m_pitch(0.0f), m_moveSpeed(15.0f),
        m_radius(0.0f) {
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
    // << " pos=(" << m_camera->position.x << "," << m_camera->position.y << ","
    // << m_camera->position.z << ")" << std::endl; ZOOM na scroll myszy
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
      const float ZOOM_SPEED = 2.0f;
      const float MIN_ZOOM = 2.0f;
      const float MAX_ZOOM = 50.0f;
      m_radius -= wheel * ZOOM_SPEED;
      if (m_radius < MIN_ZOOM)
        m_radius = MIN_ZOOM;
      if (m_radius > MAX_ZOOM)
        m_radius = MAX_ZOOM;
      // Po zmianie radius należy zaktualizować pozycję kamery
      updateCameraPosition();
      // std::cout << "[CAMERA ZOOM] wheel=" << wheel << " radius " << oldRadius
      // << " -> " << m_radius << std::endl;
    }
    // Translacja kamery równolegle do terenu – WASD/QE przesuwają position i
    // target razem
    Vector3 move = {0.0f, 0.0f, 0.0f};
    float speed = m_moveSpeed * deltaTime;

    // Shift doubles speed
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
      speed *= 2.0f;
    }

    // Oblicz surowy wektor forward (z Y)
    Vector3 rawForward = Vector3Subtract(m_camera->target, m_camera->position);
    // FLATTEN: Spłaszczenie forward do płaszczyzny XZ (Y=0)
    Vector3 flatForward = {rawForward.x, 0.0f, rawForward.z};
    float flatLen = Vector3Length(flatForward);
    if (flatLen > 0.0001f) {
      flatForward = Vector3Normalize(flatForward);
    } else {
      // Jeśli kamera patrzy prosto w górę/dół, użyj fallback
      flatForward = {0.0f, 0.0f, -1.0f};
    }
    // Oblicz wektor right od spłaszczonego forward
    Vector3 right =
        Vector3Normalize(Vector3CrossProduct(flatForward, {0.0f, 1.0f, 0.0f}));
    Vector3 up = {0.0f, 1.0f, 0.0f};
    if (IsKeyDown(KEY_W)) {
      // std::cout << "[CAMERA MOVE] W pressed" << std::endl;
      move = Vector3Add(move, flatForward);
    }
    if (IsKeyDown(KEY_S)) {
      // std::cout << "[CAMERA MOVE] S pressed" << std::endl;
      move = Vector3Subtract(move, flatForward);
    }
    if (IsKeyDown(KEY_A))
      move = Vector3Subtract(move, right);
    if (IsKeyDown(KEY_D))
      move = Vector3Add(move, right);
    if (IsKeyDown(KEY_Q))
      move = Vector3Subtract(move, up);
    if (IsKeyDown(KEY_E))
      move = Vector3Add(move, up);
    if (move.x != 0.0f || move.y != 0.0f || move.z != 0.0f) {
      move = Vector3Normalize(move);
      move = Vector3Scale(move, speed);
      m_camera->position = Vector3Add(m_camera->position, move);
      m_camera->target = Vector3Add(m_camera->target, move);
      // std::cout << "[CAMERA MOVE] Moved" << std::endl;
    }
    // Obrót kamery TYLKO na ŚRODKOWY przycisk myszy – prawdziwa orbita
    // równoległa do terenu
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
      if (!m_isRotating) {
        m_isRotating = true;
        HideCursor();
      }
      Vector2 delta = GetMouseDelta() * 0.003f;
      Vector3 pivot = m_camera->target;
      Vector3 dir = Vector3Subtract(m_camera->position, pivot);
      // Aktualizuj radius na podstawie aktualnej odległości (może się zmienić
      // przez zoom)
      m_radius = Vector3Length(dir);
      m_yaw += delta.x;
      m_pitch -= delta.y;
      m_pitch = Clamp(m_pitch, -1.4f, 1.4f);
      updateCameraPosition();
      // std::cout << "[DEBUG] ORBIT yaw:" << m_yaw << " pitch:" << m_pitch << "
      // radius:" << m_radius << std::endl;
    } else if (m_isRotating) {
      m_isRotating = false;
      ShowCursor();
    }
  }
  bool isDebugEnabled() const { return m_debugEnabled; }
  void renderDebug() const {
    DrawText(TextFormat("Cam pos: %.1f, %.1f, %.1f", m_camera->position.x,
                        m_camera->position.y, m_camera->position.z),
             10, 50, 20, DARKGRAY);
  }

private:
  void updateCameraPosition() {
    Vector3 pivot = m_camera->target;
    float xz = m_radius * cosf(m_pitch);
    m_camera->position.x = pivot.x + xz * sinf(m_yaw);
    m_camera->position.y = pivot.y + m_radius * sinf(m_pitch);
    m_camera->position.z = pivot.z + xz * cosf(m_yaw);
  }
  Camera3D *m_camera;
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
InteractionSystem *g_interactionSystem = nullptr;
BuildingSystem *g_buildingSystem = nullptr;
Colony *g_colony = nullptr;
CraftingSystem *g_craftingSystem = nullptr;
UISystem *g_uiSystem = nullptr;
TimeCycleSystem *g_timeSystem = nullptr;
// Global State
bool isBuildingMode = false;
std::string selectedBlueprintId = "house_4";
float currentBuildingRotation = 0.0f;
CameraController *freeCameraController = nullptr;
CameraController *settlerCameraController = nullptr;
bool showDebugInfo = false;
bool showCollisionBoxes = false;
bool showPathfindingDebug = false;
bool showInaccessibleWarning = false;
float inaccessibleWarningTime = 0.0f;
bool g_drawDebugGrid = false;
float globalTimeScale = 1.0f; // Globalny współczynnik skalowania czasu gry
Terrain terrain;
NavigationGrid navigationGrid(100, 100, 1.0f);
Colony colony;
std::queue<std::pair<Settler *, Vector3>> commandQueue;
bool showCommandQueue = false;
float healthBarWidth = 50.0f;
float healthBarHeight = 5.0f;
float energyBarWidth = 50.0f;
float energyBarHeight = 5.0f;
std::vector<Vector3> currentPath;
bool showPath = false;
bool isDraggingSelectionBox = false;
Vector2 selectionBoxStart;
Vector2 selectionBoxEnd;
bool isCharacterSelected = false;
Settler *selectedCharacter = nullptr;
bool isDragging = false;
Vector2 dragStartPos;
Vector2 dragEndPos;
Vector3 currentCommandTarget;
bool hasCommandTarget = false;
float cameraFollowSpeed = 5.0f;
Vector3 cameraTargetPosition;
Vector3 cameraTargetTarget;
bool wantCursorHidden = false; // Default false (Isometric)
// Helper: Raycast against everything (Terrain + Buildings)
struct WorldRaycastHit {
  bool hit;
  float distance;
  Vector3 point;
  Vector3 normal;
};

WorldRaycastHit RaycastWorld(Ray ray, float maxDist) {
  WorldRaycastHit result = {
      false,
      maxDist,
      Vector3Add(ray.position, Vector3Scale(ray.direction, maxDist)),
      {0, 1, 0}};

  // 1. Check Ground Plane (Simple) - To be improved with Terrain height
  if (ray.direction.y != 0.0f) {
    float t = (0.0f - ray.position.y) / ray.direction.y;
    if (t > 0 && t < result.distance) {
      result.hit = true;
      result.distance = t;
      result.point = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
      result.normal = {0, 1, 0};
    }
  }

  // 2. Check Buildings
  if (g_buildingSystem) {
    auto buildings = g_buildingSystem->getAllBuildings();
    for (auto *building : buildings) {
      BoundingBox box =
          building->getBoundingBox(); // This uses cached blueprint size
      RayCollision boxHit = GetRayCollisionBox(ray, box);
      if (boxHit.hit && boxHit.distance < result.distance) {
        result.hit = true;
        result.distance = boxHit.distance;
        result.point = boxHit.point;
        result.normal = boxHit.normal;
      }
    }
  }

  return result;
}

// Helper to get mouse world position (simple ground plane)
Vector3 GetMouseWorldPosition(Camera3D camera) {
  Ray ray = GetMouseRay(GetMousePosition(), camera);
  float groundY = 0.0f;
  if (ray.direction.y != 0.0f) {
    float t = (groundY - ray.position.y) / ray.direction.y;
    if (t > 0) {
      return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    }
  }
  return ray.position;
}

// 2D Health Bar Helper
static void DrawHealthBar2D(Vector3 worldPos, float progress, Color color) {
  Vector2 screenPos = GetWorldToScreen(worldPos, sceneCamera);
  if (screenPos.x < 0 || screenPos.x > GetScreenWidth() || screenPos.y < 0 ||
      screenPos.y > GetScreenHeight())
    return;

  float width = 40.0f;
  float height = 6.0f;
  Rectangle bg = {screenPos.x - width / 2, screenPos.y - height / 2, width,
                  height};
  Rectangle fg = {screenPos.x - width / 2, screenPos.y - height / 2,
                  width * progress, height};

  DrawRectangleRec(bg, BLACK);
  DrawRectangleRec(fg, color);
  DrawRectangleLinesEx(bg, 1.0f, DARKGRAY);
}
void renderScene() {
  Camera3D currentCam = sceneCamera;

  // CRITICAL FIX: Set custom projection matrix with FAR clipping plane
  // Default raylib far plane is ~1000, which can clip distant objects
  // Increase to 5000 for large open world
  float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
  float fovy = currentCam.fovy;
  if (fovy == 0.0f)
    fovy = 45.0f; // Default if not set

  // Calculate projection matrix (perspective)
  float top = 0.01f * tanf(fovy * 0.5f * DEG2RAD); // Near=0.01
  float right = top * aspect;

  rlDrawRenderBatchActive(); // Flush previous batch
  rlMatrixMode(RL_PROJECTION);
  rlPushMatrix();
  rlLoadIdentity();
  rlFrustum(-right, right, -top, top, 0.01, 5000.0); // Near=0.01, Far=5000
  rlMatrixMode(RL_MODELVIEW);
  rlLoadIdentity();

  BeginMode3D(currentCam);
  terrain.render();
  colony.render(currentCameraMode == CameraViewMode::FPS, controlledSettler);
  const std::vector<WorldItem> &droppedItems = colony.getDroppedItems();
  for (const auto &wItem : droppedItems) {
    if (wItem.item) {
      Vector3 itemPos = wItem.position;
      itemPos.y += 0.25f; // Lift from ground

      std::string name = wItem.item->getDisplayName();
      if (name == "Stone Axe") {
        // Draw Axe Geometry
        // Handle
        rlPushMatrix();
        rlTranslatef(itemPos.x, itemPos.y, itemPos.z);

        // Handle rotates to lay flat? or stand? Let's make it float/spin
        // slightly
        float time = (float)GetTime();
        rlRotatef(time * 30.0f, 0, 1, 0); // Spin

        // Handle (Brown)
        DrawCube({0, 0, 0}, 0.05f, 0.6f, 0.05f, BROWN);

        // Blade (Gray Stone) - attached to top
        DrawCube({0, 0.25f, 0.1f}, 0.05f, 0.25f, 0.15f, DARKGRAY);

        rlPopMatrix();
      } else {
        // Default Cube
        Color color = WHITE;
        if (wItem.item->getItemType() == ItemType::RESOURCE)
          color = BROWN;
        else if (wItem.item->getItemType() == ItemType::TOOL)
          color = GRAY;
        else if (wItem.item->getItemType() == ItemType::EQUIPMENT)
          color = BLUE;
        else if (wItem.item->getItemType() == ItemType::CONSUMABLE) {
          color = GREEN;
          if (name == "Raw Meat")
            color = MAROON;
        }
        DrawCube(itemPos, 0.5f, 0.5f, 0.5f, color);
        DrawCubeWires(itemPos, 0.5f, 0.5f, 0.5f, BLACK);
      }
    }
  }

  if (isBuildingMode && g_buildingSystem) {
    Vector3 mousePos = GetMouseWorldPosition(currentCam);
    g_buildingSystem->renderPreview(selectedBlueprintId, mousePos,
                                    currentBuildingRotation);
  }

  if (g_buildingSystem)
    g_buildingSystem->render();
  if (g_interactionSystem)
    g_interactionSystem->render();
  if (hasCommandTarget)
    DrawCylinder(currentCommandTarget, 0.5f, 0.5f, 0.1f, 10,
                 Color{0, 255, 0, 150});
  if (g_drawDebugGrid) {
    DrawGrid(20, 1.0f);
  }

  // [EDITOR] Render Editor Gizmos on top (depth test handling is inside Render)
  // [EDITOR] Render Editor Gizmos on top (depth test handling is inside
  // Render3D)
  g_editorSystem.Render3D(sceneCamera);

  EndMode3D();

  // Health Bars 2D for World Objects
  // 1. Trees
  const auto &trees = terrain.getTrees();
  for (const auto &tree : trees) {
    if (tree->isActive() && !tree->isStump()) {
      auto stats = tree->getComponent<StatsComponent>();
      if (stats && stats->getCurrentHealth() < stats->getMaxHealth()) {
        Vector3 barPos = tree->getPosition();
        barPos.y += 4.5f; // Above foliage
        DrawHealthBar2D(
            barPos, stats->getCurrentHealth() / stats->getMaxHealth(), GREEN);
      }
    }
  }
  // 2. Resource Nodes
  const auto &nodes = terrain.getResourceNodes();
  for (const auto &node : nodes) {
    if (node->isActive()) {
      float progress =
          (float)node->getCurrentAmount() / (float)node->getMaxAmount();
      if (progress < 1.0f) {
        Vector3 barPos = node->getPosition();
        barPos.y += 1.5f;
        DrawHealthBar2D(barPos, progress, ORANGE);
      }
    }
  }

  // 3. Building Tasks Labels - REMOVED to avoid clutter with modular buildings
  /*
  if (g_buildingSystem) {
    auto tasks = g_buildingSystem->getActiveBuildTasks();
    for (auto *task : tasks) {
      Vector3 pos = task->getPosition();
      pos.y += 3.8f; // Above bars
      Vector2 screenPos = GetWorldToScreen(pos, sceneCamera);
      if (screenPos.x > 0 && screenPos.x < GetScreenWidth() &&
          screenPos.y > 0 && screenPos.y < GetScreenHeight()) {
        std::string name = task->getBlueprint()->getName();
        int fontSize = 20;
        int textWidth = MeasureText(name.c_str(), fontSize);
        DrawRectangle(screenPos.x - textWidth / 2 - 5,
                      screenPos.y - fontSize / 2 - 2, textWidth + 10,
                      fontSize + 4, ColorAlpha(BLACK, 0.6f));
        DrawText(name.c_str(), screenPos.x - textWidth / 2,
                 screenPos.y - fontSize / 2, fontSize, WHITE);
      }
    }
  }
  */
  // Call UI render (debug)
  if (g_interactionSystem) {
    g_interactionSystem->renderUI(currentCam);
  }

  // Draw Crosshair in TPS/FPS modes
  // Draw Dynamic Crosshair in TPS/FPS modes
  if ((currentCameraMode == CameraViewMode::TPS ||
       currentCameraMode == CameraViewMode::FPS) &&
      controlledSettler) {

    // 1. Determine "Aim Point" (Camera Center -> World)
    Ray camRay;
    camRay.position = currentCam.position;
    camRay.direction = Vector3Normalize(
        Vector3Subtract(currentCam.target, currentCam.position));

    WorldRaycastHit aimHit = RaycastWorld(camRay, 100.0f);
    Vector3 aimPoint = aimHit.point;

    // 2. Determine "Muzzle Point"
    Vector3 muzzlePos = controlledSettler->getMuzzlePosition();

    // 3. Trace Barrel Ray (Muzzle -> AimPoint)
    Vector3 barrelDir = Vector3Normalize(Vector3Subtract(aimPoint, muzzlePos));
    float distToAim = Vector3Distance(muzzlePos, aimPoint);
    Ray barrelRay = {muzzlePos, barrelDir};

    WorldRaycastHit barrelHit =
        RaycastWorld(barrelRay, distToAim); // Check ONLY up to the aim point

    Vector3 finalCrosshairWorldPos = aimPoint;
    Color crosshairColor = GREEN;

    if (barrelHit.hit && barrelHit.distance < distToAim - 0.5f) { // Tolerance
      // Obstruction detected!
      finalCrosshairWorldPos = barrelHit.point;
      crosshairColor = RED; // Warning color: Barrel is blocked
    }

    // 4. Project key points to screen
    Vector2 screenCrosshair =
        GetWorldToScreen(finalCrosshairWorldPos, currentCam);

    // Draw Crosshair
    if (screenCrosshair.x > 0 && screenCrosshair.x < GetScreenWidth() &&
        screenCrosshair.y > 0 && screenCrosshair.y < GetScreenHeight()) {

      DrawCircleLines((int)screenCrosshair.x, (int)screenCrosshair.y, 10,
                      crosshairColor);
      DrawCircle((int)screenCrosshair.x, (int)screenCrosshair.y, 2,
                 crosshairColor);

      // Draw blocking indicator line if blocked
      if (crosshairColor.r == 255 && crosshairColor.g == 0) { // Check if RED
        DrawLine((int)screenCrosshair.x - 5, (int)screenCrosshair.y - 5,
                 (int)screenCrosshair.x + 5, (int)screenCrosshair.y + 5,
                 crosshairColor);
        DrawLine((int)screenCrosshair.x + 5, (int)screenCrosshair.y - 5,
                 (int)screenCrosshair.x - 5, (int)screenCrosshair.y + 5,
                 crosshairColor);
      }
    }
  }
  rlMatrixMode(RL_PROJECTION);
  rlPopMatrix();
  rlMatrixMode(RL_MODELVIEW);

  // [EDITOR] Render Editor GUI (2D) - Must be outside BeginMode3D
  g_editorSystem.RenderGUI();
}
void processInput() {
  // Debug Console Toggle
  if (IsKeyPressed(KEY_GRAVE) || IsKeyPressed(KEY_F1)) { // Tilde or F1
    DebugConsole::getInstance().toggle();
  }

  if (IsKeyPressed(KEY_TAB)) {
    if (g_uiSystem) g_uiSystem->toggleColonyStats();
  }

  if (DebugConsole::getInstance().isVisible()) {
    DebugConsole::getInstance().update();
    return; // Block other input
  }

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
      std::cout << "[TIME] Global time scale set to " << globalTimeScale
                << " (normal speed)" << std::endl;
    }
    if (IsKeyPressed(KEY_TWO)) {
      globalTimeScale = 2.0f;
      g_timeSystem->setTimeScale(globalTimeScale * 100.0f);
      std::cout << "[TIME] Global time scale set to " << globalTimeScale
                << " (2x speed)" << std::endl;
    }
    if (IsKeyPressed(KEY_THREE)) {
      globalTimeScale = 4.0f;
      g_timeSystem->setTimeScale(globalTimeScale * 100.0f);
      std::cout << "[TIME] Global time scale set to " << globalTimeScale
                << " (4x speed)" << std::endl;
    }
  }

  // [EDITOR] Update Editor System
  // Pass lists. We need non-const unique_ptr<Tree> for modification,
  // but terrain.getTrees() surely returns const reference?
  // Let's assume we can access them.
  // Actually terrain.getTrees() returns const
  // std::vector<std::unique_ptr<Tree>>&. Casting away const for editor is
  // acceptable in this context but dangerous. Better: Add non-const getter to
  // Terrain or just simple cast for now. We'll use const_cast since we know we
  // own them.

  // NOTE: This assumes Terrain has standard getters.
  std::vector<std::unique_ptr<Tree>> &trees =
      const_cast<std::vector<std::unique_ptr<Tree>> &>(terrain.getTrees());
  std::vector<Settler *> &settlers =
      const_cast<std::vector<Settler *> &>(colony.getSettlers());
  std::vector<BuildingInstance *> buildings =
      g_buildingSystem->getAllBuildings();
  std::vector<BuildTask *> activeBuildTasks = g_buildingSystem->getActiveBuildTasks();

  g_editorSystem.Update(sceneCamera, settlers, trees, buildings, activeBuildTasks);

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
      int nextIndex = (currentIndex + 1) % blueprints.size();
      selectedBlueprintId = blueprints[nextIndex]->getId();
      g_buildingSystem->setSelectedBlueprint(selectedBlueprintId);
    }
  }
  // Building placement with left click in building mode
  if (isBuildingMode && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      g_buildingSystem) {
    Vector3 mousePos = GetMouseWorldPosition(currentCam);
    bool success = false;
    g_buildingSystem->startBuilding(selectedBlueprintId, mousePos, nullptr,
                                    currentBuildingRotation, false, false,
                                    false, &success);
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

      // Priority: Control Selected Settler, otherwise First Settler
      auto selectedSettlers = colony.getSelectedSettlers();
      if (!selectedSettlers.empty()) {
        controlledSettler = selectedSettlers[0];
      } else {
        // Fallback to first available if none selected
        auto settlers = colony.getSettlers();
        if (!settlers.empty()) {
          controlledSettler = settlers[0];
        }
      }

      // Enable player control
      if (controlledSettler) {
        controlledSettler->setPlayerControlled(true);
        // TPS Mode: Hide cursor for mouse look
        DisableCursor();
        wantCursorHidden = true; // Lock cursor for TPS
        std::cout << "[CAMERA] Switched to TPS mode - Controlling: "
                  << controlledSettler->getName() << std::endl;
      }
    } else if (currentCameraMode == CameraViewMode::TPS) {
      currentCameraMode = CameraViewMode::FPS;
      DisableCursor();         // ensure hidden
      wantCursorHidden = true; // Lock cursor for FPS
      std::cout << "[CAMERA] Switched to FPS mode" << std::endl;
    } else if (currentCameraMode == CameraViewMode::FPS) {
      currentCameraMode = CameraViewMode::ISOMETRIC;
      EnableCursor();           // Show mouse back
      wantCursorHidden = false; // Release cursor for Isometric
      // Disable player control
      if (controlledSettler) {
        controlledSettler->setPlayerControlled(false);
        std::cout
            << "[CAMERA] Switched to ISOMETRIC mode - Player control DISABLED"
            << std::endl;
      }
      controlledSettler = nullptr;
    }
  }

  // Toggle debug grid with F3
  if (IsKeyPressed(KEY_F3)) {
    g_drawDebugGrid = !g_drawDebugGrid;
  }
  
  // FPS ARM EDITOR (F2)
  if (IsKeyPressed(KEY_F2)) {
      if (controlledSettler) {
          g_editorSystem.StartFpsArmEdit(controlledSettler);
      }
  }

  // Player input for settler control in TPS/FPS modes
  if ((currentCameraMode == CameraViewMode::TPS ||
       currentCameraMode == CameraViewMode::FPS) &&
      controlledSettler) {

    // --- SENSITIVITY & AIMING ---
    float baseSens = 0.002f; // Reduced from 0.005f
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      baseSens *= 0.3f; // 30% sensitivity when aiming
    }
    Vector2 delta = GetMouseDelta();

    // --- MOUSE LOOK ---
    // --- MOUSE LOOK ---
    // Handle Cursor Lock State
    // Persistent state to know if we WANT the cursor hidden
    // wantCursorHidden is now GLOBAL

    // Toggle Menu Mode with ALT
    if (IsKeyPressed(KEY_LEFT_ALT)) {
      wantCursorHidden = !wantCursorHidden;
      if (wantCursorHidden)
        DisableCursor();
      else
        EnableCursor();
    }

    // Force strict state (If we want it hidden, ensure it IS hidden)
    if (wantCursorHidden) {
      if (!IsCursorHidden())
        DisableCursor();

      // TPS: Always rotate camera with mouse movement
      if (currentCameraMode == CameraViewMode::TPS) {
        tpsYaw -= delta.x * baseSens;
        tpsPitch += delta.y * baseSens;
        tpsPitch = Clamp(tpsPitch, -0.4f, 1.2f); // Limit vertical orbit

        controlledSettler->setRotationFromMouse((tpsYaw + PI) * RAD2DEG);
      }

      // FPS: Mouse look
      if (currentCameraMode == CameraViewMode::FPS) {
        float yawChangeDeg = -delta.x * baseSens * RAD2DEG;
        controlledSettler->setRotationFromMouse(
            controlledSettler->getRotation() + yawChangeDeg);

        fpsPitch += delta.y * baseSens;
        fpsPitch = Clamp(fpsPitch, -1.4f, 1.4f); // Limit up/down
      }
    }
    // ------------------
    // ------------------

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

    // Shooting (LMB)
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector3 targetPoint = {0, 0, 0};

      // RAYCAST to find aim point (same logic as crosshair)
      Ray camRay;
      camRay.position = currentCam.position;
      camRay.direction = Vector3Normalize(
          Vector3Subtract(currentCam.target, currentCam.position));

      // Use existing helper
      WorldRaycastHit aimHit = RaycastWorld(camRay, 200.0f);
      targetPoint = aimHit.point;

      controlledSettler->useHeldItem(targetPoint);
    }

    // Interaction (Pickup) 'E'
    if (IsKeyPressed(KEY_E)) {
      // Find nearest dropped item
      const auto &dropped = colony.getDroppedItems();
      int bestIdx = -1;
      float minDist = 2.0f; // Pickup range

      for (size_t i = 0; i < dropped.size(); ++i) {
        float d = Vector3Distance(controlledSettler->getPosition(),
                                  dropped[i].position);
        if (d < minDist) {
          minDist = d;
          bestIdx = (int)i;
        }
      }

      if (bestIdx != -1) {
        // Pickup!
        std::unique_ptr<Item> item = colony.takeDroppedItem(bestIdx);
        if (item) {
          std::cout << "[Player] Picked up: " << item->getDisplayName()
                    << std::endl;

          // Auto-equip if hands empty
          if (controlledSettler->getHeldItem() == nullptr) {
            std::cout << "[Player] Auto-equipped: " << item->getDisplayName()
                      << std::endl;
            controlledSettler->setHeldItem(std::move(item));
          } else {
            controlledSettler->getInventory().addItem(std::move(item));
          }
        }
      }
    }

    // Płynne FOV (Lerp)
    currentFov += (targetFov - currentFov) * 10.0f * deltaTime;
    sceneCamera.fovy = currentFov;

    // WASD movement for settler
    Vector3 currentSettlerPos = controlledSettler->getPosition();
    Vector3 moveDir = {0, 0, 0};
    float moveSpeed = 5.0f * deltaTime;

    // Calculate direction relative to camera view
    Vector3 camForward = Vector3Normalize(
        Vector3Subtract(sceneCamera.target, sceneCamera.position));
    camForward.y = 0.0f; // Flatten to ground plane
    camForward = Vector3Normalize(camForward);
    Vector3 camRight =
        Vector3Normalize(Vector3CrossProduct(camForward, {0.0f, 1.0f, 0.0f}));

    if (IsKeyDown(KEY_W))
      moveDir = Vector3Add(moveDir, camForward);
    if (IsKeyDown(KEY_S))
      moveDir = Vector3Subtract(moveDir, camForward);
    if (IsKeyDown(KEY_A))
      moveDir = Vector3Subtract(moveDir, camRight);
    if (IsKeyDown(KEY_D))
      moveDir = Vector3Add(moveDir, camRight);

    // Apply movement with SLIDING COLLISION
    if (Vector3Length(moveDir) > 0.001f) {
      moveDir = Vector3Normalize(moveDir);
      
      // Separate axes for sliding
      Vector3 moveX = { moveDir.x * moveSpeed, 0, 0 };
      Vector3 moveZ = { 0, 0, moveDir.z * moveSpeed };
      
      Vector3 finalPos = currentSettlerPos;
      bool moved = false;

      // Define Collision Checker Helper locally for captured scope
      auto IsPositionFree = [&](Vector3 testPos) -> bool {
          // 1. Check Buildings (Increased radius to 10.0f to catch large buildings)
          if (g_buildingSystem) {
              auto buildings = g_buildingSystem->getBuildingsInRange(testPos, 10.0f);
              for (auto* b : buildings) {
                  if (b->getBlueprintId() == "floor") continue;
                  if (b->CheckCollision(testPos, 0.4f)) {
                      // std::cout << "[Collision] Blocked by building: " << b->getBlueprintId() << std::endl;
                      return false; 
                  }
              }
          }
           // 2. Check Trees
           const auto& trees = terrain.getTrees();
           for(const auto& t : trees) {
               if(t->isActive() && !t->isStump()) {
                   if(CheckCollisionBoxSphere(t->getBoundingBox(), testPos, 0.4f)) {
                       // std::cout << "[Collision] Blocked by tree" << std::endl;
                       return false;
                   }
               }
           }
           return true;
      };

      // Try X Movement
      if (IsPositionFree(Vector3Add(finalPos, moveX))) {
          finalPos = Vector3Add(finalPos, moveX);
          moved = true;
      }
      
      // Try Z Movement
      if (IsPositionFree(Vector3Add(finalPos, moveZ))) {
          finalPos = Vector3Add(finalPos, moveZ);
          moved = true;
      }

      if (moved) {
          controlledSettler->setPosition(finalPos);
          controlledSettler->setState(SettlerState::MOVING);
      } else {
          controlledSettler->setState(SettlerState::IDLE);
      }

    } else {
      controlledSettler->setState(SettlerState::IDLE);
    }
  }

  // Camera controller updates based on current mode
  if (currentCameraMode == CameraViewMode::ISOMETRIC) {
    if (freeCameraController)
      freeCameraController->update(deltaTime);
  } else if (currentCameraMode == CameraViewMode::TPS && controlledSettler) {
    // Third-person camera: orbiting around settler
    Vector3 settlerPos = controlledSettler->getPosition();
    // Base Pivot: Head height
    Vector3 pivotBase = Vector3Add(settlerPos, {0.0f, 1.5f, 0.0f});

    // --- OFFSETS for "Bottom-Left" Settler Position ---
    // To put settler on Left: Camera must move Right.
    // To put settler on Bottom: Camera must move Up.

    // Calculate Camera Basis
    // Forward is (Pivot -> Camera) inverted? No, usually Camera Forward is
    // (Target - Pos). Let's use our Orbit Yaw to determine "Right" relative to
    // the view. tpsYaw is angle around Y. View Direction (Orbit):
    float viewX = sinf(tpsYaw);
    float viewZ = cosf(tpsYaw);
    Vector3 viewDir = {viewX, 0.0f, viewZ}; // Flattened view dir
    Vector3 viewRight =
        Vector3CrossProduct(viewDir, {0.0f, 1.0f, 0.0f}); // Right vector

    // Apply Offsets to the PIVOT (The point we look AT)
    // Actually, if we want the settler to be Left, we should look at a point to
    // the RIGHT of the settler. If we look at (Settler + Right * Offset), the
    // Settler appears to the Left.
    float offsetRight =
        -0.8f; // OTS Offset Right (Negative because viewRight is inverted/Left)
    float offsetUp = 0.6f; // OTS Offset Up (look slightly above head)

    Vector3 pivotOffset = Vector3Scale(viewRight, offsetRight);
    pivotOffset.y += offsetUp;

    Vector3 finalPivot = Vector3Add(pivotBase, pivotOffset);

    // Calculate Desired Camera Position based on Orbit around FINAL PIVOT
    // SMOOTHING:
    // Only smooth the target (Pivot). Do not smooth the resulting rotation to
    // prevent input lag.
    float lerpFactor = 15.0f * deltaTime; // Responsiveness
    smoothCamTarget = Vector3Lerp(smoothCamTarget, finalPivot, lerpFactor);

    // Calculate Camera Position based on SMOOTHED PIVOT + RAW ROTATION
    float xz = tpsRadius * cosf(tpsPitch);
    Vector3 orbitPos;
    orbitPos.x = smoothCamTarget.x + xz * sinf(tpsYaw);
    orbitPos.y = smoothCamTarget.y + tpsRadius * sinf(tpsPitch);
    orbitPos.z = smoothCamTarget.z + xz * cosf(tpsYaw);

    sceneCamera.position = orbitPos;
    sceneCamera.target = smoothCamTarget;

    // Sync smoothCamPos for transitions
    smoothCamPos = orbitPos;
  } else if (currentCameraMode == CameraViewMode::FPS && controlledSettler) {
    // First-person camera: from settler's eyes
    Vector3 settlerPos = controlledSettler->getPosition();
    float settlerRot = controlledSettler->getRotation();

    // Camera at settler's eye height
    Vector3 eyePos = settlerPos;
    eyePos.y += 1.4f; // CORRECTED: Eye level (Body 0.5 + Neck/Head 0.82 ~ 1.32 + offset)
                     // 1.6 was top of head. 1.4 is eyes.

    // Look direction calculated from settler rotation and fpsPitch
    Vector3 lookDir;
    // Correct forward calculation (sync with getForwardVector)
    // Fixed: Remove negative signs to look FORWARD relative to body rotation
    lookDir.x = sinf(settlerRot * DEG2RAD) * cosf(fpsPitch);
    lookDir.y = -sinf(fpsPitch); // Up/Down
    lookDir.z = cosf(settlerRot * DEG2RAD) * cosf(fpsPitch);

    // Apply backward offset to see more of the arms/body
    Vector3 flatForward = {sinf(settlerRot * DEG2RAD), 0.0f,
                           cosf(settlerRot * DEG2RAD)};
    
    // User complaint: "Too far behind head". Removed -0.35f offset.
    // Now exactly at eye position (or slightly forward if needed)
    Vector3 backOffset = Vector3Scale(flatForward, 0.2f); // Slightly FORWARD (Eyes in front of neck) 

    sceneCamera.position = Vector3Add(eyePos, backOffset);
    sceneCamera.target = Vector3Add(sceneCamera.position, lookDir);

    // Sync smoothers so when we switch back to TPS it's ready
    smoothCamPos = sceneCamera.position;
    smoothCamTarget = sceneCamera.target;
  }
}

int main() {

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Simple 3D Game");
  SetTargetFPS(60);
  // Initialize cameras
  sceneCamera = {0};
  sceneCamera.position = Vector3{10.0f, 10.0f, 10.0f};
  sceneCamera.target = Vector3{0.0f, 0.0f, 0.0f};
  sceneCamera.up = Vector3{0.0f, 1.0f, 0.0f};
  sceneCamera.fovy = 45.0f;
  sceneCamera.projection = CAMERA_PERSPECTIVE;
  settlerCamera = {0};
  settlerCamera.position = Vector3{0.0f, 5.0f, 10.0f};
  settlerCamera.target = Vector3{0.0f, 0.0f, 0.0f};
  settlerCamera.up = Vector3{0.0f, 1.0f, 0.0f};
  settlerCamera.fovy = 45.0f;
  settlerCamera.projection = CAMERA_PERSPECTIVE;
  // Create camera controllers
  freeCameraController = new CameraController(&sceneCamera);
  settlerCameraController = new CameraController(&settlerCamera);
  // Initialize GameEngine
  GameEngine &engine = GameEngine::getInstance();
  // Create and register systems
  auto buildingSystem = std::make_unique<BuildingSystem>();
  auto interactionSystem = std::make_unique<InteractionSystem>();
  auto uiSystem = std::make_unique<UISystem>();
  auto timeSystem = std::make_unique<TimeCycleSystem>();
  auto inventorySystem = std::make_unique<InventorySystem>();
  auto storageSystem = std::make_unique<StorageSystem>();
  auto needsSystem = std::make_unique<NeedsSystem>();
  auto craftingSystem = std::make_unique<CraftingSystem>();
  // Set global pointers
  g_buildingSystem = buildingSystem.get();
  g_interactionSystem = interactionSystem.get();
  g_uiSystem = uiSystem.get();
  g_timeSystem = timeSystem.get();
  g_craftingSystem = craftingSystem.get();
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
  g_colony = &colony;
  // Set drop item callback for trees
  GameEngine::dropItemCallback = [](Vector3 position, Item *item) {
    colony.addDroppedItem(std::unique_ptr<Item>(item), position);
    std::cout << "[GameEngine] Dropped item at (" << position.x << ", "
              << position.y << ", " << position.z << ")" << std::endl;
  };
  if (g_interactionSystem) {
    g_interactionSystem->setColony(&colony);
    g_interactionSystem->setUIVisible(true);
    std::cout << "[DEBUG] InteractionSystem colony set, UI visible enabled."
              << std::endl;
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
    terrain.update(deltaTime);
    float scaledDeltaTime = deltaTime * globalTimeScale;
    float gameTime = g_timeSystem ? g_timeSystem->getCurrentTime() : 0.0f;
    colony.update(scaledDeltaTime, gameTime, terrain.getTrees(),
                  g_buildingSystem->getAllBuildings());

    // Update navigation grid with current obstacles
    {
      auto buildings = g_buildingSystem->getAllBuildings();

      // Convert unique_ptr<Tree> to Tree*
      std::vector<Tree *> treePtrs;
      const auto &treesRef = terrain.getTrees();
      treePtrs.reserve(treesRef.size());
      for (const auto &treePtr : treesRef) {
        treePtrs.push_back(treePtr.get());
      }

      // Resources already use unique_ptr in signature
      const auto &resources = terrain.getResourceNodes();

      navigationGrid.UpdateGrid(buildings, treePtrs, resources);
    }

    engine.update(scaledDeltaTime);
    BeginDrawing();
    ClearBackground(RAYWHITE);
    renderScene();
    engine.render();
    // Note: All UI is now handled by UISystem::render() called from engine.render() above
    if (freeCameraController && freeCameraController->isDebugEnabled()) {
      freeCameraController->renderDebug();
    }

    // Draw Debug Console Overlay
    DebugConsole::getInstance().render();

    // --- FORCE CURSOR LOCK ---
    // Aggressively re-apply cursor lock every frame if it should be hidden.
    if (wantCursorHidden) { // Use our persistent state
      if (!IsCursorHidden())
        DisableCursor();

      // NUCLEAR OPTION: Force cursor to center to prevent escaping
      // Only if we are focused
      if (IsWindowFocused()) {
        SetMousePosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
      }
    }

    EndDrawing();
  }
  engine.shutdown();
}
