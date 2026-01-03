#pragma once

#include "raylib.h"
#include <iostream>
#include <string>

// Enum to identify the type of object
enum class EditorObjectType { None, Settler, Tree, Building, Prop, Construction };

// Abstract Base Wrapper
class EditorObjectWrapper {
public:
  virtual ~EditorObjectWrapper() = default;

  virtual Vector3 GetPosition() const = 0;
  virtual void SetPosition(const Vector3 &pos) = 0;

  virtual float GetRotation() const = 0; // Degrees
  virtual void SetRotation(float rot) = 0;

  virtual std::string GetName() const = 0;
  virtual EditorObjectType GetType() const = 0;

  // Optional: Only if needed for highlighting or bounding box
  virtual BoundingBox GetBoundingBox() const = 0;
};
