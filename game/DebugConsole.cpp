#include "DebugConsole.h"
#include "raylib.h"
#include <sstream>
#include <iostream>
#include <algorithm>

// Include necessary game systems
#include "../core/GameSystem.h"
#include "Colony.h"
#include "Settler.h"
#include "Item.h"
#include "../systems/TimeCycleSystem.h"
#include "../game/ResourceNode.h"
#include "../core/GameEntity.h"

// External variable references (if needed directly, otherwise use GameSystem)
extern float globalTimeScale;
extern Settler* controlledSettler;

DebugConsole& DebugConsole::getInstance() {
    static DebugConsole instance;
    return instance;
}

DebugConsole::DebugConsole() 
    : m_isVisible(false), m_historyIndex(-1), m_cursorBlinkTimer(0.0f), m_showCursor(true), m_scrollOffset(0) {
    registerDefaultCommands();
    log("Debug Console Initialized. Type 'help' for commands.");
}

void DebugConsole::toggle() {
    m_isVisible = !m_isVisible;
    if (m_isVisible) {
        // Reset input state when opening
        m_currentInput.clear();
        m_historyIndex = -1;
    }
}

bool DebugConsole::isVisible() const {
    return m_isVisible;
}

void DebugConsole::update() {
    if (!m_isVisible) return;

    handleInput();

    // Blink cursor
    m_cursorBlinkTimer += GetFrameTime();
    if (m_cursorBlinkTimer >= 0.5f) {
        m_cursorBlinkTimer = 0.0f;
        m_showCursor = !m_showCursor;
    }
}

void DebugConsole::handleInput() {
    // Close on Tilde (managed in main.cpp mostly, but here for safety if called directly)
    // Actually, Input mapping should be handled by caller or specific key check here.
    // main.cpp handles the toggle logic usually.
    
    // Character input
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125)) {
            m_currentInput += (char)key;
        }
        key = GetCharPressed();
    }

    // Special keys
    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (!m_currentInput.empty()) {
            m_currentInput.pop_back();
        }
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (!m_currentInput.empty()) {
            log("> " + m_currentInput);
            m_commandHistory.push_back(m_currentInput);
            executeCommand(m_currentInput);
            m_currentInput.clear();
            m_historyIndex = -1;
            m_scrollOffset = 0; // Reset scroll on new command
        }
    }

    // History navigation
    if (IsKeyPressed(KEY_UP)) {
        if (!m_commandHistory.empty()) {
            if (m_historyIndex == -1) {
                m_historyIndex = (int)m_commandHistory.size() - 1;
            } else if (m_historyIndex > 0) {
                m_historyIndex--;
            }
            if (m_historyIndex >= 0 && m_historyIndex < (int)m_commandHistory.size()) {
                m_currentInput = m_commandHistory[m_historyIndex];
            }
        }
    }

    if (IsKeyPressed(KEY_DOWN)) {
        if (m_historyIndex != -1) {
            if (m_historyIndex < (int)m_commandHistory.size() - 1) {
                m_historyIndex++;
                m_currentInput = m_commandHistory[m_historyIndex];
            } else {
                m_historyIndex = -1;
                m_currentInput.clear();
            }
        }
    }
    
    // Scrolling logic could be added here (PageUp/PageDown)
}

void DebugConsole::render() {
    if (!m_isVisible) return;

    renderOverlay();
    renderLog();
    renderInput();
}

void DebugConsole::renderOverlay() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Semi-transparent black background covering top half or full screen?
    // Let's do top 40% of screen
    int height = (int)(screenHeight * 0.4f);
    DrawRectangle(0, 0, screenWidth, height, Fade(BLACK, 0.85f));
    DrawRectangleLines(0, 0, screenWidth, height, DARKGRAY);
}

void DebugConsole::renderLog() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int consoleHeight = (int)(screenHeight * 0.4f);
    int fontSize = 20;
    int padding = 10;
    int lineHeight = fontSize + 2;
    int maxLines = (consoleHeight - 40) / lineHeight; // -40 for input area

    int startY = consoleHeight - 40 - lineHeight;
    
    // Draw from bottom up
    int count = 0;
    for (int i = (int)m_logBuffer.size() - 1 - m_scrollOffset; i >= 0 && count < maxLines; --i) {
        DrawText(m_logBuffer[i].c_str(), padding, startY - (count * lineHeight), fontSize, LIGHTGRAY);
        count++;
    }
}

void DebugConsole::renderInput() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int consoleHeight = (int)(screenHeight * 0.4f);
    int fontSize = 20;
    int padding = 10;
    
    int y = consoleHeight - 30; // Bottom of the console area
    
    DrawText(">", padding, y, fontSize, YELLOW);
    
    std::string textToDraw = m_currentInput;
    if (m_showCursor) {
        textToDraw += "_";
    }
    
    DrawText(textToDraw.c_str(), padding + 20, y, fontSize, WHITE);
}

void DebugConsole::log(const std::string& message) {
    m_logBuffer.push_back(message);
    // Keep buffer size managed if needed
    if (m_logBuffer.size() > 1000) {
        m_logBuffer.erase(m_logBuffer.begin());
    }
}

void DebugConsole::registerCommand(const std::string& name, std::function<void(const std::vector<std::string>&)> callback, const std::string& helpText) {
    m_commands[name] = { callback, helpText };
}

void DebugConsole::executeCommand(const std::string& commandLine) {
    if (commandLine.empty()) return;

    std::istringstream iss(commandLine);
    std::string commandName;
    iss >> commandName;
    
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    auto it = m_commands.find(commandName);
    if (it != m_commands.end()) {
        try {
            it->second.action(args);
        } catch (const std::exception& e) {
            log("Error executing command: " + std::string(e.what()));
        }
    } else {
        log("Unknown command: '" + commandName + "'. Type 'help' for list.");
    }
}

void DebugConsole::registerDefaultCommands() {
    // HELP
    registerCommand("help", [this](const std::vector<std::string>& args) {
        (void)args;
        log("--- Available Commands ---");
        for (const auto& kv : m_commands) {
            log(kv.first + " : " + kv.second.helpText);
        }
        log("--------------------------");
    }, "Shows this help message");

    // CLEAR
    registerCommand("clear", [this](const std::vector<std::string>& args) {
        (void)args;
        m_logBuffer.clear();
    }, "Clears the console log");

    // FPS
    registerCommand("fps", [this](const std::vector<std::string>& args) {
        (void)args;
        // Toggle FPS logic if we had a variable, strictly speaking Raylib creates window with FPS control
        // For now just log current FPS
        log("Current FPS: " + std::to_string(GetFPS()));
    }, "Displays current FPS");

    // SPEED
    registerCommand("speed", [this](const std::vector<std::string>& args) {
        if (args.empty()) {
            log("Usage: speed <value>");
            return;
        }
        try {
            float speed = std::stof(args[0]);
            globalTimeScale = speed;
            
            // Try to update TimeCycleSystem if available
            // We can't access g_timeSystem directly easily without extern or getter
            // Assuming globalTimeScale is used in main loop update
            
            log("Time scale set to: " + std::to_string(speed));
        } catch (...) {
            log("Invalid number format.");
        }
    }, "Sets global time scale. Usage: speed 1.0");

    // GIVE
    registerCommand("give", [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            log("Usage: give <item_name> <amount>");
            return;
        }
        // Need access to colony storage or player inventory (controlled settler)
        if (!controlledSettler) {
            log("No settler controlled to give items to!");
            return;
        }
        
        std::string itemId = args[0];
        int amount = 1;
        try {
            amount = std::stoi(args[1]);
        } catch (...) {
            log("Invalid amount.");
            return;
        }
        
        // Create item and add to inventory
        // Using ResourceItem as Item is abstract
        // Constructor: ResourceItem(string type, string displayName, string desc)
        auto newItem = std::make_unique<ResourceItem>(itemId, "Debug Item", "Spawned via console");
        controlledSettler->getInventory().addItem(std::move(newItem));
        if (amount > 1) {
            for (int i=1; i<amount; ++i) {
                 controlledSettler->getInventory().addItem(std::make_unique<ResourceItem>(itemId, "Debug Item", "Spawned via console"));
            }
        }
        log("Gave " + std::to_string(amount) + " " + itemId + " to controlled settler.");
        
    }, "Gives item to controlled settler");

    // SPAWN
    registerCommand("spawn", [this](const std::vector<std::string>& args) {
        if (args.empty()) {
            log("Usage: spawn <settler|tree|stone>");
            return;
        }
        std::string type = args[0];
        Colony* colony = GameSystem::getColony();
        if (!colony) {
            log("Error: Colony system not available.");
            return;
        }
        
        // Spawn position: in front of camera or controlled settler
        Vector3 spawnPos = {0,0,0};
        if (controlledSettler) {
            spawnPos = Vector3Add(controlledSettler->getPosition(), {2.0f, 0.0f, 2.0f});
        }
        
        if (type == "settler") {
            colony->addSettler(spawnPos);
             log("Spawned Settler at " + std::to_string(spawnPos.x) + ", " + std::to_string(spawnPos.z));
        } else if (type == "tree") {
            // Need Terrain access
            // Assuming Terrain has addTree... 
            // Terrain usually generates trees, maybe addTree is available?
            log("Spawning trees dynamically not fully implemented via console yet.");
        } else {
             log("Unknown entity type.");
        }
       
    }, "Spawns an entity nearby");

    // GOD MODE
    registerCommand("god", [this](const std::vector<std::string>& args) {
         if (!controlledSettler) {
            log("No settler controlled.");
            return;
        }
        // Assuming we can set stats to max
        // Assuming we can set stats to max
        auto& stats = controlledSettler->getStats();
        // stats is a reference, access members directly via . or methods
        stats.setHunger(100.0f);
        stats.setEnergy(100.0f);
        stats.setHealth(100.0f);
             log("Refilled stats for controlled settler.");
    }, "Refills health/hunger/energy for controlled settler");
}
