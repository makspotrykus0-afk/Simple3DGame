#pragma once

#include "../core/IGameSystem.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <atomic>

/**
 * @brief Typy żywności
 */
enum class FoodType : uint32_t {
    BREAD = 0,
    MEAT = 1,
    FRUIT = 2,
    VEGETABLES = 3,
    FISH = 4,
    GRAIN = 5,
    DAIRY = 6,
    HERBS = 7,
    WATER = 8,
    COOKED_MEAT = 9,
    BAKED_GOODS = 10,
    CUSTOM_START = 1000
};

/**
 * @brief System żywności implementujący mechaniki głodu i zdrowia
 */
class FoodSystem : public IGameSystem {
public:
    /**
     * @brief Stan żywieniowy gracza
     */
    struct PlayerNutrition {
        float currentHunger;     // 0-100 (0 = umierasz z głodu, 100 = najedzony)
        float currentHealth;     // 0-100 (0 = martwy, 100 = doskonałe zdrowie)
        
        PlayerNutrition() : currentHunger(50.0f), currentHealth(100.0f) {}
    };

public:
    /**
     * @brief Konstruktor
     */
    FoodSystem();

    /**
     * @brief Destruktor
     */
    virtual ~FoodSystem() override = default;

    // IGameSystem interface
    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;
    std::string getName() const override;
    int getPriority() const override;

private:
    /** Nazwa systemu */
    std::string m_name;
    
    /** Mapa stanu żywieniowego graczy */
    std::unordered_map<std::string, PlayerNutrition> m_playerNutrition;
};
