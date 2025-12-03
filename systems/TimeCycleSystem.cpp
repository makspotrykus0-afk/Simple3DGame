#include "TimeCycleSystem.h"
#include "raymath.h"
#include <cstdio>
#include <cmath>
#include <string>

// Helper function for Color interpolation
// Made static to avoid linker redefinition errors if this function name conflicts with others
// or is included multiple times. Raylib defines ColorLerp in textures.c usually as internal but sometimes exposed?
// A safer way is to rename or make static.
static Color TimeColorLerp(Color c1, Color c2, float amount) {
    Color result;
    result.r = (unsigned char)(c1.r + (int)((c2.r - c1.r) * amount));
    result.g = (unsigned char)(c1.g + (int)((c2.g - c1.g) * amount));
    result.b = (unsigned char)(c1.b + (int)((c2.b - c1.b) * amount));
    result.a = 255;
    return result;
}

TimeCycleSystem::TimeCycleSystem() 
    : currentTime(12.0f), // Start at noon
      timeScale(100.0f),   // Faster time for testing
      dayColor(WHITE),
      nightColor{20, 20, 60, 255}, // Dark blue
      dawnColor{200, 120, 80, 255}, // Orange
      duskColor{100, 60, 120, 255}  // Purple
{
}

void TimeCycleSystem::initialize() {
    printf("TimeCycleSystem initialized. Start time: 12:00\n");
}

void TimeCycleSystem::update(float deltaTime) {
    float hoursPassed = (deltaTime * timeScale) / 3600.0f;
    currentTime += hoursPassed;
    
    if (currentTime >= 24.0f) {
        currentTime -= 24.0f;
        printf("New Day Started!\n");
    }
}

void TimeCycleSystem::render() {
    // Rendering handled by main loop using getters
}

float TimeCycleSystem::getCurrentTime() const {
    return currentTime;
}

std::string TimeCycleSystem::getFormattedTime() const {
    int hours = (int)currentTime;
    int minutes = (int)((currentTime - hours) * 60.0f);
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
    return std::string(buffer);
}

bool TimeCycleSystem::isDay() const {
    return currentTime >= 6.0f && currentTime < 20.0f;
}

void TimeCycleSystem::setTimeScale(float scale) {
    timeScale = scale;
}

Color TimeCycleSystem::getAmbientColor() const {
    // Night (0 - 5) -> Dawn (5 - 7) -> Day (7 - 19) -> Dusk (19 - 21) -> Night (21 - 24)
    
    if (currentTime < SUNRISE_START) return nightColor;
    
    if (currentTime < SUNRISE_END) {
        float t = (currentTime - SUNRISE_START) / (SUNRISE_END - SUNRISE_START);
        if (t < 0.5f) return TimeColorLerp(nightColor, dawnColor, t * 2.0f);
        else return TimeColorLerp(dawnColor, dayColor, (t - 0.5f) * 2.0f);
    }
    
    if (currentTime < SUNSET_START) return dayColor;
    
    if (currentTime < SUNSET_END) {
        float t = (currentTime - SUNSET_START) / (SUNSET_END - SUNSET_START);
        if (t < 0.5f) return TimeColorLerp(dayColor, duskColor, t * 2.0f);
        else return TimeColorLerp(duskColor, nightColor, (t - 0.5f) * 2.0f);
    }
    
    return nightColor;
}

float TimeCycleSystem::getAmbientLightLevel() const {
    Color ambient = getAmbientColor();
    return (ambient.r + ambient.g + ambient.b) / (3.0f * 255.0f);
}