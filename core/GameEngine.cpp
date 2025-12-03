#include "GameEngine.h"
#include "IGameSystem.h"
#include "EventSystem.h"
#include <algorithm>

// Define the static callback
GameEngine::DropItemCallback GameEngine::dropItemCallback = nullptr;

GameEngine& GameEngine::getInstance() {
    static GameEngine instance;
    return instance;
}

void GameEngine::initialize() {
    for (auto& system : systems) {
        system->initialize();
    }
}

void GameEngine::update(float deltaTime) {
    eventSystem->processEvents();

    for (auto& system : systems) {
        system->update(deltaTime);
    }
}

void GameEngine::render() {
    for (auto& system : systems) {
        system->render();
    }
}

void GameEngine::shutdown() {
    for (auto& system : systems) {
        system->shutdown();
    }
    systems.clear();
}

void GameEngine::addSystem(std::unique_ptr<IGameSystem> system) {
    // Insert sorted by priority (higher priority first)
    auto it = std::upper_bound(systems.begin(), systems.end(), system,
        [](const std::unique_ptr<IGameSystem>& a, const std::unique_ptr<IGameSystem>& b) {
            return a->getPriority() > b->getPriority();
        });
    systems.insert(it, std::move(system));
}

void GameEngine::registerSystem(std::unique_ptr<IGameSystem> system) {
    addSystem(std::move(system));
}

// void GameEngine::registerSystem(IGameSystem* system) deleted

EventSystem* GameEngine::getEventSystem() const {
    return eventSystem.get();
}