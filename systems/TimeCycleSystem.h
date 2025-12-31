#pragma once
#include "../core/IGameSystem.h"
#include "raylib.h"
#include <vector>
#include <string>
#include <cmath> // Include cmath for math operations if needed

class TimeCycleSystem : public IGameSystem {
public:
    TimeCycleSystem();
    ~TimeCycleSystem() override = default;

    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override {}
    std::string getName() const override { return "TimeCycleSystem"; }
    int getPriority() const override { return 100; }

    // Returns current time in hours (0.0 - 24.0)
    float getCurrentTime() const;
    
    // Returns formatted time string "HH:MM"
    std::string getFormattedTime() const;

    // Returns current period of day as string
    std::string getDayPeriodString() const {
        if (currentTime >= 5.0f && currentTime < 7.0f) return "Dawn";
        if (currentTime >= 7.0f && currentTime < 19.0f) return "Day";
        if (currentTime >= 19.0f && currentTime < 21.0f) return "Dusk";
        return "Night";
    }

    // Returns true if it is day time (6:00 - 20:00)
    bool isDay() const;

    // Sets time scale (multiplier of real time)
    void setTimeScale(float scale);

    // Returns ambient light level (0.0 - 1.0)
    float getAmbientLightLevel() const;
    
    // Returns ambient color for rendering
    Color getAmbientColor() const;

private:
    float currentTime; // In hours (0-24)
    float timeScale;   // Time multiplier
    
    // Cycle configuration
    const float SUNRISE_START = 5.0f;
    const float SUNRISE_END = 7.0f;
    const float SUNSET_START = 19.0f;
    const float SUNSET_END = 21.0f;
    
    Color dayColor;
    Color nightColor;
    Color dawnColor;
    Color duskColor;
};