#pragma once

#include "../entities/GameEntity.h"
#include <raylib.h>

enum class AntState {
    IDLE,
    SEARCHING,
    CARRYING_FOOD,
    RETURNING_TO_COLONY,
    FIGHTING
};

class Ant : public GameEntity {
public:
    Ant(const std::string& id, Vector3 position);
    Ant(const std::string& id, Vector3 position, Color c, float spd); // Added overloaded constructor
    ~Ant() override = default;

    void update(float deltaTime) override;
    void render() override;
    
    Vector3 getPosition() const override { return m_position; }
    void setPosition(const Vector3& position) override { m_position = position; }

    void SetTarget(Vector3 newTarget); // Added SetTarget method declaration

private:
    void PickUpFood() {} // Mock method to satisfy usage in cpp
    void DropFood() {}   // Mock method to satisfy usage in cpp

    Vector3 m_position;
    Vector3 m_velocity;
    
    // Fields used in cpp but missing in header
    Vector3 target;
    float speed;
    AntState state;
    bool hasFood;
    float health;
    Color color;
};