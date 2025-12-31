#include "Spider.h"

#include <iostream>

Spider::Spider(const Vector3& position)

: Animal(position, "Spider") {

std::cout 

speed = 2.0f; // Spiders are faster than generic animals

}

void Spider::update(float deltaTime) {

// Update web cooldown

if (webCooldown > 0.0f) {

webCooldown -= deltaTime;

if (webCooldown 

webCooldown = 0.0f;

}

}



text


// Simple AI: if aggressive, move randomly (for demonstration)
if (aggressive) {
    // Simple random movement - in real game would use proper AI
    // For now, just output a message
    std::cout 
}

// Call parent update if needed
Animal::update(deltaTime);


}

void Spider::render() const {

// Specific spider rendering logic

std::cout 



}

void Spider::webAttack() {

if (webCooldown 

std::cout 

webCooldown = WEB_COOLDOWN_TIME;

} else {

std::cout 

}

}