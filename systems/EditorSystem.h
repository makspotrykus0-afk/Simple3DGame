#pragma once

#include "EditorShared.h"
#include "raylib.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations
// Forward declarations
class Camera3D;
class Settler;
class Tree;
class BuildingInstance;
class GameEntity;
class BuildTask; // Added

// The main editor system
class EditorSystem {
public:
  EditorSystem();
  ~EditorSystem();

  // Main update loop
  void Update(const Camera3D &camera, std::vector<Settler *> &settlers,
              std::vector<std::unique_ptr<Tree>> &trees,
              std::vector<BuildingInstance *> &buildings,
              std::vector<BuildTask *> &buildTasks); // Added buildTasks

  // Main render loop (draws gizmos and UI)
  // Main render loop (draws gizmos in 3D)
  void Render3D(const Camera3D &camera);

  // Draws UI overlays (must be called outside 3D mode)
  void RenderGUI();
  bool HasSelection() const { return m_selectedObject != nullptr; }

  // Custom Editors (PUBLIC)
  void StartFpsArmEdit(Settler* settler);

  // Logging
  void LogChange(const std::string &objectName, const std::string &changeType,
                 const std::string &oldValue, const std::string &newValue);

private:
  // Selection Logic
  void HandleSelection(const Ray &ray, std::vector<Settler *> &settlers,
                       std::vector<std::unique_ptr<Tree>> &trees,
                       std::vector<BuildingInstance *> &buildings,
                       std::vector<BuildTask *> &buildTasks); // Added buildTasks

  // Gizmo Logic
  void HandleGizmoInput(const Ray &ray);
  void DrawGizmos(const Camera3D &camera);
  void DrawTranslationGizmo(const Vector3 &pos, float scale);
  void DrawRotationGizmo(const Vector3 &pos, float scale);

  // Render UI Panel
  void DrawAttributePanel();

  // Internal Helpers
  std::unique_ptr<EditorObjectWrapper> CreateWrapper(GameEntity *entity);

private:
  // State
  std::unique_ptr<EditorObjectWrapper> m_selectedObject;
  bool m_areGizmosEnabled = true; // Enabled by default until F3 toggled

  // Gizmo State
  int m_processAxis = -1; // -1: None, 0: X, 1: Y, 2: Z
  bool m_isRotating =
      false; // True if using rotation gizmo, false if translation
  bool m_isDragging = false;
  Vector3 m_dragStartPos;
  float m_initialValue = 0.0f; // Initial pos component or rotation
  Vector3 m_initialObjPos;     // Initial object position for relative checks

  // Log History
  struct LogEntry {
    std::string objectName;
    std::string action;
    std::string from;
    std::string to;
  };
  std::vector<LogEntry> m_changeLog;
};
