#include "EditorSystem.h"
#include "UISystem.h"
#include "../game/BuildingInstance.h"
#include "../game/Settler.h"
#include "../game/Tree.h"
#include "../game/Colony.h"
#include "../core/GameEngine.h"
#include "raymath.h"
#include "rlgl.h"
#include "raymath.h"
#include "rlgl.h"
#include <iostream>

// ==================================================================================
// Wrappers
// ==================================================================================

class SettlerWrapper : public EditorObjectWrapper {
  Settler *m_settler;

public:
  SettlerWrapper(Settler *s) : m_settler(s) {}
  Vector3 GetPosition() const override { return m_settler->getPosition(); }
  void SetPosition(const Vector3 &pos) override { m_settler->setPosition(pos); }
  float GetRotation() const override { return m_settler->getRotation(); }
  void SetRotation(float rot) override { m_settler->setRotation(rot); }
  std::string GetName() const override {
    return "Settler: " + m_settler->getName();
  }
  EditorObjectType GetType() const override {
    return EditorObjectType::Settler;
  }
  BoundingBox GetBoundingBox() const override {
    Vector3 pos = m_settler->getPosition();
    return {{pos.x - 0.5f, pos.y, pos.z - 0.5f},
            {pos.x + 0.5f, pos.y + 2.0f, pos.z + 0.5f}};
  }
};

class TreeWrapper : public EditorObjectWrapper {
  Tree *m_tree;

public:
  TreeWrapper(Tree *t) : m_tree(t) {}
  Vector3 GetPosition() const override { return m_tree->getPosition(); }
  void SetPosition(const Vector3 &pos) override { m_tree->setPosition(pos); }
  float GetRotation() const override { return m_tree->getRotation(); }
  void SetRotation(float rot) override { m_tree->setRotation(rot); }
  std::string GetName() const override { return "Tree"; }
  EditorObjectType GetType() const override { return EditorObjectType::Tree; }
  BoundingBox GetBoundingBox() const override {
    return m_tree->getBoundingBox();
  }
};

#include "../game/BuildingTask.h" // Added

// ... existing code ...

class BuildingWrapper : public EditorObjectWrapper {
  BuildingInstance *m_building;

public:
  BuildingWrapper(BuildingInstance *b) : m_building(b) {}
  Vector3 GetPosition() const override { return m_building->getPosition(); }
  void SetPosition(const Vector3 &pos) override {
    m_building->setPosition(pos);
  }
  float GetRotation() const override { return m_building->getRotation(); }
  void SetRotation(float rot) override { m_building->setRotation(rot); }
  std::string GetName() const override {
    return "Building: " + m_building->getBlueprintId();
  }
  EditorObjectType GetType() const override {
    return EditorObjectType::Building;
  }
  BoundingBox GetBoundingBox() const override {
    return m_building->getBoundingBox();
  }
};

class BuildTaskWrapper : public EditorObjectWrapper {
  BuildTask *m_task;

public:
  BuildTaskWrapper(BuildTask *t) : m_task(t) {}
  Vector3 GetPosition() const override { return m_task->getPosition(); }
  void SetPosition(const Vector3 &pos) override {
    m_task->setPosition(pos); // Requires BuildTask::setPosition
  }
  float GetRotation() const override { return m_task->getRotation(); }
  void SetRotation(float rot) override { m_task->setRotation(rot); } // Requires BuildTask::setRotation
  std::string GetName() const override {
    std::string bpName = m_task->getBlueprint() ? m_task->getBlueprint()->getName() : "Unknown";
    return "Construction: " + bpName;
  }
  EditorObjectType GetType() const override {
    return EditorObjectType::Construction;
  }
  BuildTask* GetTask() const { return m_task; }
  BoundingBox GetBoundingBox() const override {
    return m_task->getBoundingBox();
  }
};
class FpsArmWrapper : public EditorObjectWrapper {
  Settler* m_settler; // Reference for position calculation relative to head
public:
  FpsArmWrapper(Settler* s) : m_settler(s) {}

  Vector3 GetPosition() const override {
      // Calculate World Position of the Arm Gizmo
      // Arm is relative to Head (approx).
      Vector3 headPos = m_settler->getPosition();
      headPos.y += 0.82f; // Approx head base

      // Apply modifiers:
      // X (Left) = -UserRight
      // Y (Up)   = UserUp
      // Z (Fwd)  = UserFwd
      // BUT this is model space. Model is rotated by Settler Rotation!
      
      // Calculate Local Offset Vector
      Vector3 localOffset = { -Settler::s_fpsUserRight, Settler::s_fpsUserUp, Settler::s_fpsUserFwd };
      
      // Rotate by Settler Rotation (Y-axis)
      Vector3 worldOffset = Vector3RotateByAxisAngle(localOffset, {0,1,0}, m_settler->getRotation() * DEG2RAD);
      
      return Vector3Add(headPos, worldOffset);
  }

  void SetPosition(const Vector3& newPos) override {
      // Inverse operation: Calculate new Offsets from World Pos
      Vector3 headPos = m_settler->getPosition();
      headPos.y += 0.82f;

      Vector3 worldOffset = Vector3Subtract(newPos, headPos);
      
      // Un-rotate to get back to Local Space
      Vector3 localOffset = Vector3RotateByAxisAngle(worldOffset, {0,1,0}, -m_settler->getRotation() * DEG2RAD);

      // Update Statics
      Settler::s_fpsUserRight = -localOffset.x; // X is -UserRight
      Settler::s_fpsUserUp    = localOffset.y;
      Settler::s_fpsUserFwd   = localOffset.z;
  }

  float GetRotation() const override {
      return Settler::s_fpsYaw;
  }

  void SetRotation(float rot) override {
      Settler::s_fpsYaw = rot;
  }

  std::string GetName() const override {
      return "FPS Arm Config (Local Offsets)";
  }

  EditorObjectType GetType() const override {
      return EditorObjectType::Settler; // Reusing generic type for now
  }
  
  BoundingBox GetBoundingBox() const override {
     Vector3 p = GetPosition();
     return {{p.x-0.1f, p.y-0.1f, p.z-0.1f}, {p.x+0.1f, p.y+0.1f, p.z+0.1f}};
  }
  
  // Custom: Return LOCAL offset for UI display (not world position)
  Vector3 GetLocalOffset() const {
      return {Settler::s_fpsUserFwd, Settler::s_fpsUserUp, Settler::s_fpsUserRight};
  }
};

void EditorSystem::StartFpsArmEdit(Settler* settler) {
    if (!settler) return;
    std::cout << "[Editor] Starting FPS Arm Edit Mode" << std::endl;
    m_selectedObject = std::make_unique<FpsArmWrapper>(settler);
}

EditorSystem::EditorSystem() {}

EditorSystem::~EditorSystem() {}

void EditorSystem::Update(const Camera3D &camera,
                          std::vector<Settler *> &settlers,
                          std::vector<std::unique_ptr<Tree>> &trees,
                          std::vector<BuildingInstance *> &buildings,
                          std::vector<BuildTask *> &buildTasks) {

  // VALIDATION: Check if selected object (Task) is still alive
  if (m_selectedObject && m_selectedObject->GetType() == EditorObjectType::Construction) {
      BuildTaskWrapper* wrapper = static_cast<BuildTaskWrapper*>(m_selectedObject.get());
      bool found = false;
      for (auto* t : buildTasks) {
          if (t == wrapper->GetTask()) {
              found = true;
              break;
          }
      }
      if (!found) {
          std::cout << "[EditorSystem] Selected Task deleted. Deselecting." << std::endl;
          m_selectedObject = nullptr;
      }
  }

  Ray ray = GetMouseRay(GetMousePosition(), camera);

  // 1. Gizmo Interaction First (if selected)
  if (m_selectedObject) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      HandleGizmoInput(ray);
    }

    if (m_isDragging) {
      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        // Initial Log when releasing
        // Log Change
        if (m_isRotating) {
          float currentRot = m_selectedObject->GetRotation();
          if (abs(currentRot - m_initialValue) > 0.01f) {
            LogChange(m_selectedObject->GetName(), "Rotation",
                      std::to_string(m_initialValue),
                      std::to_string(currentRot));
          }
        } else {
          Vector3 currentPos = m_selectedObject->GetPosition();
          if (Vector3Distance(currentPos, m_initialObjPos) > 0.01f) {
            std::string oldS = TextFormat("%.2f,%.2f,%.2f", m_initialObjPos.x,
                                          m_initialObjPos.y, m_initialObjPos.z);
            std::string newS = TextFormat("%.2f,%.2f,%.2f", currentPos.x,
                                          currentPos.y, currentPos.z);
            LogChange(m_selectedObject->GetName(), "Position", oldS, newS);
          }
        }

        m_isDragging = false;
        m_processAxis = -1;
      } else {
        // Simplified: Use Mouse Delta with sensitivity.
        Vector2 delta = GetMouseDelta();
        float sensitivity = 0.05f;
        // Adjust sensitivity by distance?
        float dist =
            Vector3Distance(camera.position, m_selectedObject->GetPosition());
        sensitivity *= (dist * 0.02f);

        if (m_isRotating) {
          // Rotate
          // Process Axis: 0:X, 1:Y, 2:Z
          float rotDelta = delta.x; // Simplified
          float currentRot = m_selectedObject->GetRotation();
          float newRot = currentRot + rotDelta * 2.0f; // Scale speed

          m_selectedObject->SetRotation(newRot);
        } else {
          // Translate
          Vector3 pos = m_selectedObject->GetPosition();

          if (m_processAxis == 0) {         // X Axis
            pos.x += delta.x * sensitivity; // Simple screen mapping
          }
          if (m_processAxis == 1) {         // Y Axis
            pos.y -= delta.y * sensitivity; // Inverse Y screen
          }
          if (m_processAxis == 2) {         // Z Axis
            pos.z += delta.y * sensitivity; // Use Y mouse for Z depth usually?
          }
          m_selectedObject->SetPosition(pos);
        }
      }
    }
  }

  // 2. Selection (Only if not dragging gizmo)
  if (!m_isDragging && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    // Check if we hit a gizmo part? We did that in HandleGizmoInput.
    // If m_processAxis is -1, then we try to select objects.
    if (m_processAxis == -1) {
      HandleSelection(ray, settlers, trees, buildings, buildTasks);
    }
  }

  // 3. Status F1
  if (IsKeyPressed(KEY_F1)) {
    std::cout << "=== EDITOR CHANGE LOG ===" << std::endl;
    for (const auto &log : m_changeLog) {
      std::cout << "[EDITOR LOG] Object: " << log.objectName
                << " | Action: " << log.action << " | From: " << log.from
                << " | To: " << log.to << std::endl;
    }
    std::cout << "=========================" << std::endl;
  }
}

// Updated signature to match header
void EditorSystem::HandleSelection(const Ray &ray,
                                   std::vector<Settler *> &settlers,
                                   std::vector<std::unique_ptr<Tree>> &trees,
                                   std::vector<BuildingInstance *> &buildings,
                                   std::vector<BuildTask *> &buildTasks) {

  // Simple Raycast against BoundingBoxes
  float minDst = 99999.0f;
  GameEntity *bestEntity = nullptr;
  BuildTask *bestTask = nullptr;

  // 1. Settlers
  for (auto *s : settlers) {
    BoundingBox box = {{s->getPosition().x - 0.5f, s->getPosition().y,
                        s->getPosition().z - 0.5f},
                       {s->getPosition().x + 0.5f, s->getPosition().y + 2.0f,
                        s->getPosition().z + 0.5f}};
    RayCollision col = GetRayCollisionBox(ray, box);
    if (col.hit && col.distance < minDst) {
      minDst = col.distance;
      bestEntity = s;
      bestTask = nullptr;
    }
  }

  // 2. Trees
  for (size_t i = 0; i < trees.size(); ++i) {
    if (!trees[i]->isActive() || trees[i]->isStump())
      continue;
    BoundingBox box = trees[i]->getBoundingBox();
    RayCollision col = GetRayCollisionBox(ray, box);
    if (col.hit && col.distance < minDst) {
      minDst = col.distance;
      bestEntity = trees[i].get();
      bestTask = nullptr;
    }
  }

  // 3. Buildings (Skip invisible placeholders!)
  for (auto *b : buildings) {
    if (!b->isVisible()) continue; // CRITICAL: Skip composite placeholders
    
    BoundingBox box = b->getBoundingBox();
    RayCollision col = GetRayCollisionBox(ray, box);
    if (col.hit && col.distance < minDst) {
      minDst = col.distance;
      bestEntity = b;
      bestTask = nullptr;
    }
  }

  // 4. Build Tasks (NEW)
  for (auto *task : buildTasks) {
    BoundingBox box = task->getBoundingBox();
    RayCollision col = GetRayCollisionBox(ray, box);
    if (col.hit && col.distance < minDst) {
      minDst = col.distance;
      bestTask = task;
      bestEntity = nullptr;
    }
  }

  if (bestTask) {
    m_selectedObject = std::make_unique<BuildTaskWrapper>(bestTask);
  } else if (bestEntity && minDst < 100.0f) {
    m_selectedObject = CreateWrapper(bestEntity);
  }
}

std::unique_ptr<EditorObjectWrapper>
EditorSystem::CreateWrapper(GameEntity *entity) {
  // Check Settler
  Settler *s = dynamic_cast<Settler *>(entity);
  if (s)
    return std::make_unique<SettlerWrapper>(s);

  Tree *t = dynamic_cast<Tree *>(entity);
  if (t)
    return std::make_unique<TreeWrapper>(t);

  BuildingInstance *b = dynamic_cast<BuildingInstance *>(entity);
  if (b)
    return std::make_unique<BuildingWrapper>(b);

  return nullptr;
}

void EditorSystem::HandleGizmoInput(const Ray &ray) {
  if (!m_selectedObject)
    return;

  Vector3 pos = m_selectedObject->GetPosition();
  float scale = Vector3Distance(ray.position, pos) * 0.2f;

  // Hit boxes for arrows
  // X Axis (Red)
  BoundingBox boxX = {
      {pos.x + scale * 0.5f, pos.y - scale * 0.1f, pos.z - scale * 0.1f},
      {pos.x + scale * 1.5f, pos.y + scale * 0.1f, pos.z + scale * 0.1f}};
  if (GetRayCollisionBox(ray, boxX).hit) {
    m_isDragging = true;
    m_processAxis = 0;
    m_isRotating = false;
    m_initialObjPos = pos;
    return;
  }

  // Y Axis (Green)
  BoundingBox boxY = {
      {pos.x - scale * 0.1f, pos.y + scale * 0.5f, pos.z - scale * 0.1f},
      {pos.x + scale * 0.1f, pos.y + scale * 1.5f, pos.z + scale * 0.1f}};
  if (GetRayCollisionBox(ray, boxY).hit) {
    m_isDragging = true;
    m_processAxis = 1;
    m_isRotating = false;
    m_initialObjPos = pos;
    return;
  }

  // Z Axis (Blue)
  BoundingBox boxZ = {
      {pos.x - scale * 0.1f, pos.y - scale * 0.1f, pos.z + scale * 0.5f},
      {pos.x + scale * 0.1f, pos.y + scale * 0.1f, pos.z + scale * 1.5f}};
  if (GetRayCollisionBox(ray, boxZ).hit) {
    m_isDragging = true;
    m_processAxis = 2;
    m_isRotating = false;
    m_initialObjPos = pos;
    return;
  }

  // Sphere (Center) for Rotation Toggle? Or maybe a Ring
  BoundingBox centerBox = {
      {pos.x - scale * 0.2f, pos.y - scale * 0.2f, pos.z - scale * 0.2f},
      {pos.x + scale * 0.2f, pos.y + scale * 0.2f, pos.z + scale * 0.2f}};
  if (GetRayCollisionBox(ray, centerBox).hit) {
    m_isDragging = true;
    m_isRotating = true; // Simple toggle via center click drag
    m_initialValue = m_selectedObject->GetRotation();
    return;
  }
}

void EditorSystem::Render3D(const Camera3D &camera) {
  if (m_selectedObject) {
    rlDisableDepthTest();
    DrawGizmos(camera);
    rlEnableDepthTest();
  }
}

void EditorSystem::RenderGUI() {
  if (!m_selectedObject) return;

  // MUTEX: If HUD is active for any settler, or colony stats/crafting is visible,
  // suppress Editor Panel to avoid clutter (Art Director Rule: No Overlap)
  extern Colony* g_colony;
  auto ui = GameEngine::getInstance().getSystem<UISystem>();
  if (ui && (ui->IsCraftingPanelVisible() || (g_colony && !g_colony->getSelectedSettlers().empty()) || ui->IsBuildingSelectionPanelVisible())) {
      return; 
  }

  DrawAttributePanel();
}

void EditorSystem::DrawGizmos(const Camera3D &camera) {
  Vector3 pos = m_selectedObject->GetPosition();
  float dist = Vector3Distance(camera.position, pos);
  float scale = dist * 0.15f;

  // Center
  DrawSphere(pos, scale * 0.2f, WHITE);

  // X Axis - Red
  DrawCylinderEx(pos, {pos.x + scale, pos.y, pos.z}, scale * 0.05f,
                 scale * 0.05f, 8, RED);
  DrawCylinderEx({pos.x + scale, pos.y, pos.z},
                 {pos.x + scale * 1.3f, pos.y, pos.z}, scale * 0.15f, 0.0f, 8,
                 RED);

  // Y Axis - Green
  DrawCylinderEx(pos, {pos.x, pos.y + scale, pos.z}, scale * 0.05f,
                 scale * 0.05f, 8, GREEN);
  DrawCylinderEx({pos.x, pos.y + scale, pos.z},
                 {pos.x, pos.y + scale * 1.3f, pos.z}, scale * 0.15f, 0.0f, 8,
                 GREEN);

  // Z Axis - Blue
  DrawCylinderEx(pos, {pos.x, pos.y, pos.z + scale}, scale * 0.05f,
                 scale * 0.05f, 8, BLUE);
  DrawCylinderEx({pos.x, pos.y, pos.z + scale},
                 {pos.x, pos.y, pos.z + scale * 1.3f}, scale * 0.15f, 0.0f, 8,
                 BLUE);
}

void EditorSystem::DrawAttributePanel() {
  int screenW = GetScreenWidth();
  int width = 300;
  int height = 220;
  Rectangle rec = { (float)screenW - width - 10, 10, (float)width, (float)height };

  // Use Centralized Premium Drawing System
  auto ui = GameEngine::getInstance().getSystem<UISystem>();
  if (ui) {
      ui->DrawPremiumPanel(rec, "EDITOR ATTRIBUTES", 0.85f);
  } else {
      DrawRectangleRec(rec, Fade(BLACK, 0.8f));
      DrawRectangleLinesEx(rec, 1.0f, DARKGRAY);
      DrawText("EDITOR ATTRIBUTES", (int)rec.x + 10, (int)rec.y + 10, 20, WHITE);
  }

  int textX = (int)rec.x + 25;
  int textY = (int)rec.y + 60;
  int spacing = 22;

  DrawText(TextFormat("OBJECT: %s", m_selectedObject->GetName().c_str()), textX, textY, 10, ui ? ui->GetThemeColor("teal") : SKYBLUE);
  textY += spacing;

  FpsArmWrapper* fpsArm = dynamic_cast<FpsArmWrapper*>(m_selectedObject.get());
  if (fpsArm) {
      Vector3 local = fpsArm->GetLocalOffset();
      DrawText(TextFormat("FWD: %.2f | RIGHT: %.2f | UP: %.2f", local.x, local.z, local.y), textX, textY, 11, ui ? ui->GetThemeColor("gold") : YELLOW);
  } else {
      Vector3 pos = m_selectedObject->GetPosition();
      DrawText(TextFormat("POS: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z), textX, textY, 10, LIGHTGRAY);
  }
  textY += spacing;

  float rot = m_selectedObject->GetRotation();
  DrawText(TextFormat("ROT: %.2f", rot), textX, textY, 10, LIGHTGRAY);
  textY += spacing + 5;

  DrawLineEx({rec.x + 20, (float)textY}, {rec.x + width - 20, (float)textY}, 1.0f, Fade(ui ? ui->GetThemeColor("teal") : SKYBLUE, 0.3f));
  textY += 10;

  DrawText("INPUT_DRAG: MOVE // CENTER: ROT", textX, textY, 9, Fade(GRAY, 0.7f));
}

void EditorSystem::LogChange(const std::string &objectName,
                             const std::string &changeType,
                             const std::string &oldValue,
                             const std::string &newValue) {
  m_changeLog.push_back({objectName, changeType, oldValue, newValue});
}

