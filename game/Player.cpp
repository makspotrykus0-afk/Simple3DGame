#include "Player.h"
#include "Colony.h" // [ANTIGRAVITY] Needed for UpdateInteraction
#include "InteractableObject.h"
#include "Settler.h" // [FIX] Needed for dynamic_cast and method access
#include "Terrain.h"
#include <algorithm>
#include <iostream>

extern Terrain terrain; // Global terrain reference
extern Terrain terrain; // Global terrain reference

Player::Player(const std::string &name, const Vector3 &startPos,
               Camera3D &camera)
    : GameEntity(name), m_camera(camera), m_position(startPos),
      m_velocity({0, 0, 0}), m_yaw(0.0f), m_pitch(0.0f),
      m_state(PlayerState::IDLE), m_isGrounded(false),
      m_currentCameraHeight(m_cameraHeight), m_bobTimer(0.0f),
      m_bobFrequency(0.0f), m_bobAmplitude(0.0f), m_swayAmount(0.02f),
      m_maxSway(0.1f), m_smoothFactor(8.0f), m_swayOffset({0, 0, 0}),
      m_weaponBasePos(
          {0.2f, -0.25f,
           0.4f}) { // [FIX] Closer to center and camera for better visibility
  // Initialize angles from camera if needed, or set camera to start pos
  m_camera.position = {m_position.x, m_position.y + m_cameraHeight,
                       m_position.z};
  m_camera.target = {m_position.x + 1.0f, m_position.y + m_cameraHeight,
                     m_position.z};
  m_camera.up = {0.0f, 1.0f, 0.0f};

  Vector3 dir = Vector3Subtract(m_camera.target, m_camera.position);
  dir = Vector3Normalize(dir);
  m_yaw = atan2f(dir.x, dir.z);
  m_pitch = asinf(dir.y);
}

void Player::update(float deltaTime) {
  if (!IsCursorHidden())
    return; // Don't move if in menu/cursor visible

  HandleInput();
  UpdatePhysics(deltaTime);
  UpdateHeadBob(deltaTime);
  UpdateWeaponSway(deltaTime);
  UpdateCamera(deltaTime);
  UpdateInteraction(deltaTime); // [ANTIGRAVITY] Update Raycasts
}

void Player::render() {
  // Debug Render: Simple "Hand" Box
  // Must be rendered AFTER camera transformation if we were using raw OpenGL,
  // but Raylib uses a camera stack.
  // Actually, to render "HUD" style weapons, we usually disable depth test or
  // render in a separate pass. For this prototype, we will just calculate world
  // position of the hand.

  Vector3 forward = Vector3Subtract(m_camera.target, m_camera.position);
  forward = Vector3Normalize(forward);
  Vector3 right = Vector3CrossProduct(forward, m_camera.up);
  Vector3 up = m_camera.up;

  // Calculate World Position of the Weapon
  // Base offset + Sway
  Vector3 finalOffset = Vector3Add(m_weaponBasePos, m_swayOffset);

  // Hand Position relative to camera position
  Vector3 handPos = m_camera.position;
  handPos = Vector3Add(handPos, Vector3Scale(right, finalOffset.x));
  handPos = Vector3Add(handPos, Vector3Scale(up, finalOffset.y));
  handPos = Vector3Add(handPos, Vector3Scale(forward, finalOffset.z));

  // Draw simple cube as "weapon"
  DrawCube(handPos, 0.15f, 0.15f, 0.5f, MAROON);
  DrawCubeWires(handPos, 0.15f, 0.15f, 0.5f, DARKGRAY);
}

void Player::UpdateWeaponSway(float deltaTime) {
  Vector2 mouseDelta = GetMouseDelta();

  // Calculate target sway based on input
  // Invert X because moving mouse left moves weapon right (lag)
  float targetX = -mouseDelta.x * m_swayAmount;
  float targetY = -mouseDelta.y * m_swayAmount;

  // Clamp
  targetX = Clamp(targetX, -m_maxSway, m_maxSway);
  targetY = Clamp(targetY, -m_maxSway, m_maxSway);

  Vector3 targetSway = {targetX, targetY, 0.0f};

  // Smoothly interpolate current sway to target
  m_swayOffset.x =
      Lerp(m_swayOffset.x, targetSway.x, deltaTime * m_smoothFactor);
  m_swayOffset.y =
      Lerp(m_swayOffset.y, targetSway.y, deltaTime * m_smoothFactor);
  // Z sway could be added for movement impact
  m_swayOffset.z = 0.0f;
}

void Player::HandleInput() {
  // Rotation (Mouse)
  Vector2 mouseDelta = GetMouseDelta();
  m_yaw -= mouseDelta.x * m_mouseSensitivity;
  m_pitch -= mouseDelta.y * m_mouseSensitivity;

  // Clamp Pitch (avoid flipping)
  if (m_pitch > 1.5f)
    m_pitch = 1.5f;
  if (m_pitch < -1.5f)
    m_pitch = -1.5f;

  // Movement States
  m_state = PlayerState::IDLE;
  float currentSpeed = m_walkSpeed;

  if (IsKeyDown(KEY_LEFT_SHIFT)) {
    m_state = PlayerState::SPRINTING;
    currentSpeed = m_sprintSpeed;
  }

  if (IsKeyDown(KEY_LEFT_CONTROL)) {
    m_state = PlayerState::CROUCHING;
    currentSpeed = m_crouchSpeed;
  }

  // Direction calculation
  Vector3 forward = {sinf(m_yaw), 0.0f, cosf(m_yaw)};
  Vector3 right = {cosf(m_yaw), 0.0f, -sinf(m_yaw)};

  Vector3 wishDir = {0.0f, 0.0f, 0.0f};

  if (IsKeyDown(KEY_W))
    wishDir = Vector3Add(wishDir, forward);
  if (IsKeyDown(KEY_S))
    wishDir = Vector3Subtract(wishDir, forward);
  if (IsKeyDown(KEY_D))
    wishDir = Vector3Subtract(wishDir, right); // [FIX] Inverted Strafe
  if (IsKeyDown(KEY_A))
    wishDir = Vector3Add(wishDir, right); // [FIX] Inverted Strafe

  if (Vector3Length(wishDir) > 0.001f) {
    wishDir = Vector3Normalize(wishDir);
    if (m_state == PlayerState::IDLE)
      m_state = PlayerState::WALKING;

    // Instant acceleration for responsiveness (Classic FPS feel)
    m_velocity.x = wishDir.x * currentSpeed;
    m_velocity.z = wishDir.z * currentSpeed;
  } else {
    m_velocity.x = 0;
    m_velocity.z = 0;
  }

  // Jump
  if (IsKeyPressed(KEY_SPACE) && m_isGrounded) {
    m_velocity.y = m_jumpForce;
    m_isGrounded = false;
    m_state = PlayerState::AIRBORNE;
  }

  // Interaction (Recruit)
  if (IsKeyPressed(KEY_E)) {
    if (m_focusedObject) {
      Settler *s = dynamic_cast<Settler *>(m_focusedObject);
      if (s) {
        RecruitFollower(s);
      } else {
        // Standard interact
        // m_focusedObject->interact(this); // TODO: General interaction
      }
    }
  }

  // Squad Commands
  if (IsKeyPressed(KEY_F)) {
    IssueCommand(0); // Toggle Follow/Hold
  }
}

void Player::UpdatePhysics(float deltaTime) {
  ApplyGravity(deltaTime);

  // Apply Velocity
  m_position.x += m_velocity.x * deltaTime;
  m_position.y += m_velocity.y * deltaTime;
  m_position.z += m_velocity.z * deltaTime;

  CheckGround();
}

void Player::ApplyGravity(float deltaTime) {
  m_velocity.y -= m_gravity * deltaTime;
}

void Player::CheckGround() {
  float terrainHeight = terrain.getHeightAt(m_position.x, m_position.z);

  if (m_position.y <= terrainHeight) {
    m_position.y = terrainHeight;
    m_velocity.y = 0;
    m_isGrounded = true;
  } else {
    // Simple "is grounded" check tolerance
    if (m_position.y < terrainHeight + 0.2f && m_velocity.y <= 0) {
      m_isGrounded = true;
    } else {
      m_isGrounded = false;
    }
  }
}

void Player::UpdateHeadBob(float deltaTime) {
  if (m_state == PlayerState::IDLE || !m_isGrounded) {
    m_bobTimer = 0.0f;
    // Dampen back to 0
    return;
  }

  // Parameters based on state
  if (m_state == PlayerState::SPRINTING) {
    m_bobFrequency = 14.0f;
    m_bobAmplitude = 0.15f;
  } else if (m_state == PlayerState::CROUCHING) {
    m_bobFrequency = 6.0f;
    m_bobAmplitude = 0.05f;
  } else { // WALKING
    m_bobFrequency = 10.0f;
    m_bobAmplitude = 0.08f;
  }

  m_bobTimer += deltaTime * m_bobFrequency;
}

void Player::UpdateCamera(float deltaTime) {
  // Crouch Smoothing
  float targetHeight =
      (m_state == PlayerState::CROUCHING) ? m_crouchHeight : m_cameraHeight;
  m_currentCameraHeight =
      Lerp(m_currentCameraHeight, targetHeight, deltaTime * 10.0f);

  // Head Bob Offset
  float bobOffset = 0.0f;
  if (m_isGrounded && m_state != PlayerState::IDLE) {
    bobOffset = sinf(m_bobTimer) * m_bobAmplitude;
  }

  // Set Camera Pos
  m_camera.position = {m_position.x,
                       m_position.y + m_currentCameraHeight + bobOffset,
                       m_position.z};

  // Set Camera Target
  Vector3 forward;
  forward.x = sinf(m_yaw) * cosf(m_pitch);
  forward.y = sinf(m_pitch);
  forward.z = cosf(m_yaw) * cosf(m_pitch);

  m_camera.target = Vector3Add(m_camera.position, forward);
}

// [ANTIGRAVITY] New Interaction System
void Player::UpdateInteraction(float deltaTime) {
  (void)deltaTime;
  m_focusedObject = nullptr; // Reset

  // Raycast from Camera Center
  Ray ray;
  ray.position = m_camera.position;
  ray.direction = Vector3Subtract(m_camera.target, m_camera.position);
  ray.direction = Vector3Normalize(ray.direction);

  // Check collision with Settlers
  extern Colony *g_colony; // [ANTIGRAVITY] Local extern to fix visibility
  if (g_colony) {
    float minDist = 3.0f; // Max interaction range
    auto settlers = g_colony->getSettlers();

    for (auto *s : settlers) {
      // Simple Sphere/Box check
      // We use BoundingBox from Settler (GameEntity)
      // But RayBoxIntersection is reliable
      // Or use RayCollision Sphere
      RayCollision collision = GetRayCollisionSphere(ray, s->getPosition(),
                                                     0.5f); // 0.5f radius human

      if (collision.hit) {
        if (collision.distance < minDist) {
          minDist = collision.distance;
          m_focusedObject = s;
        }
      }
    }
  }

  // Debug UI Prompt
  if (m_focusedObject) {
    // This is a quick hack to draw text on screen, ideally use UI system
    // DrawText("Press 'E' to Interact", GetScreenWidth()/2 - 100,
    // GetScreenHeight()/2 + 50, 20, WHITE); But DrawText must be in Drawing
    // Loop. We can store a flag or string that main.cpp or Render uses. For
    // now, let's just Log once on focus change? No, spammy. We will add a
    // render UI hook in Player::render()
  }
}

void Player::RecruitFollower(Settler *settler) {
  if (!settler)
    return;

  // Check if already in squad
  for (auto *s : m_squad) {
    if (s == settler)
      return;
  }

  // Cost check (placeholder)
  if (m_authority < 10.0f) {
    std::cout << "[Player] Not enough Authority to recruit!" << std::endl;
    return;
  }
  m_authority -= 10.0f;

  m_squad.push_back(settler);
  std::cout << "[Player] Recruited: " << settler->getName() << std::endl;

  // Link Squad Logic
  // 0 is the hardcoded Player ID for this prototype
  settler->JoinSquad(0);
}

void Player::IssueCommand(int commandType) {
  // 0: Toggle Follow/Hold
  if (commandType == 0) {
    std::cout << "[Player] Squad Command: TOGGLE FOLLOW/HOLD (Placeholder)"
              << std::endl;
    // Logic to iterate squad and change state will go here
  }
}
