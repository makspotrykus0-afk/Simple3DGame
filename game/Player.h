#ifndef SIMPLE3DGAME_GAME_PLAYER_H
#define SIMPLE3DGAME_GAME_PLAYER_H

#include "../entities/GameEntity.h"
#include "raylib.h"
#include "raymath.h"

// Player movement states
enum class PlayerState { IDLE, WALKING, SPRINTING, CROUCHING, AIRBORNE };

class Player : public GameEntity {
public:
  Player(const std::string &name, const Vector3 &startPos, Camera3D &camera);
  virtual ~Player() = default;

  // GameEntity interface overrides
  void update(float deltaTime) override;
  void render() override;
  Vector3 getPosition() const override { return m_position; }
  void setPosition(const Vector3 &pos) override { m_position = pos; }

  // Input Handling
  void HandleInput();

  // Interaction
  void
  CheckInteractions(const std::vector<class InteractableObject *> &objects);
  class InteractableObject *getFocusedObject() const { return m_focusedObject; }

  // Getters/Setters
  float getYaw() const { return m_yaw; }
  float getPitch() const { return m_pitch; }
  PlayerState getState() const { return m_state; }
  Camera3D getCamera() const { return m_camera; }

private:
  // Core Physics
  void UpdatePhysics(float deltaTime);
  void UpdateCamera(float deltaTime);
  void UpdateHeadBob(float deltaTime);
  void UpdateWeaponSway(float deltaTime);

  // Internal helpers
  void ApplyGravity(float deltaTime);
  void CheckGround();

private:
  // References
  Camera3D &m_camera;

  // Transform
  Vector3 m_position;
  Vector3 m_velocity;
  float m_yaw;
  float m_pitch;

  // Settings
  float m_walkSpeed = 5.0f;
  float m_sprintSpeed = 10.0f;
  float m_crouchSpeed = 2.5f;
  float m_jumpForce = 7.0f;
  float m_gravity = 18.0f;
  float m_cameraHeight = 1.8f;
  float m_crouchHeight = 1.0f;
  float m_mouseSensitivity = 0.003f;

  // State
  PlayerState m_state;
  bool m_isGrounded;
  float m_currentCameraHeight; // For smooth crouching

  // Head Bob
  float m_bobTimer;
  float m_bobFrequency;
  float m_bobAmplitude;

  // Weapon/Tool Sway
  // implemented in UpdateWeaponSway

  // Sway Settings
  float m_swayAmount = 0.02f;
  float m_maxSway = 0.1f;
  float m_smoothFactor = 8.0f;
  Vector3 m_swayOffset = {0.0f, 0.0f, 0.0f}; // Applied to weapon position
  Vector3 m_weaponBasePos = {0.4f, -0.3f,
                             0.6f}; // Relative to camera (Right hand)

  // Interaction State
  class InteractableObject *m_focusedObject = nullptr;

  // Leadership System
  std::vector<class Settler *> m_squad;
  float m_authority = 100.0f; // Starting authority

  // Commands
  void RecruitFollower(class Settler *settler);
  void IssueCommand(int commandType); // 0: Follow/Hold, 1: MoveTo

  void UpdateInteraction(float deltaTime); // [NEW]
};

#endif // SIMPLE3DGAME_GAME_PLAYER_H
