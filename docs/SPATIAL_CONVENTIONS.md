# Spatial & Animation Conventions

## World Coordinate System

- **Up Axis**: Y+ (Standard Raylib/OpenGL)
- **Forward Axis**: Z+ (or Z- depending on camera, typically Z+ is "South/Back", Z- is "North/Forward" in OpenGL, but Raylib often treats Z+ as forward for cameras?)
  - **Convention**: Entity **Forward** is **Z+**.
- **Right Axis**: X- (Right-Handed System check: Cross(Forward Z+, Up Y+) = Right -X) (Wait, Raylib is Right-Handed? If Z+ is forward, Side is -X?)
  - **VERIFIED SETTLER CODE**: Right Hand is visually at **X = -0.3**.
  - Therefore: **-X is RIGHT**, **+X is LEFT**.
  - All logic must follow this.

## Entity Local Space (Settler)

- **Pivot**: Feet (0,0,0)
- **Body Center**: Y + 0.5 (approx)
- **Head**: Y + 1.5
- **Right Hand**: X -0.3 (Body Relative)
- **Left Hand**: X +0.3 (Body Relative)

## Rotation

- **Yaw (Rotation Y)**:
  - 0 degrees: Facing Z+ (Forward) ?? No, usually 0 is aligned with Z axis.
  - Positive Rotation: Rotates towards X+ (Left).
  - **Rule**: `rlRotatef(yaw, 0, 1, 0)` -> Positive is CCW around Y.

## Animation Conventions

- **Pitch (Arm Angle)**:
  - X-Axis Rotation.
  - 0 deg: Vertical Down (Relaxed).
  - -90 deg: Horizontal Forward (Pointing).
  - +90 deg: Horizontal Backward.
  - -180 deg: Vertical Up.
- **Hook Punch (TPS)**:
  - **Start**: Right arm pulled back (Yaw negative/outward).
  - **Action**: Swing horizontally (Pitch ~-90) to Left (Yaw positive/inward).

## FPS Camera

- **Position**: Eye level (Y + 1.6).
- **Hand Rendering**:
  - Should be relative to Camera Frame (not World).
  - **Base Pose (Arm Vertical Down)**:
    - **Arm**: Points -Y.
    - **Axe Handle**: Points +Z (Forward).
    - **Axe Blade**: Points -Y (Down).
  - **Ready Pose (Arm Horizontal Forward)**:
    - Rotate Arm -90 X.
    - **Arm**: Points -Z (Forward).
    - **Axe Handle**: Points -Y (Down).
    - **Axe Blade**: Points -Z (Forward).
  - **Action**: Swing from Ready (-85 Pitch) to Base (0 Pitch).
