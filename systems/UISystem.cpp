#include "UISystem.h"
#include "../core/GameEngine.h"
#include "../systems/InteractionSystem.h" // Include full definition
#include "../systems/InventorySystem.h"   // Include full definition
#include "../game/Colony.h" // Include full definition for Settler
#include "../components/StatsComponent.h" // Include StatsComponent
#include <iostream>
#include <algorithm>

// ==========================================
// UIElement Implementation
// ==========================================

Vector3 UISystem::UIElement::getScreenPosition() const {
    return Vector3{position.x, position.y, 0.0f}; // Assuming Z is 0 for 2D UI
}

// ==========================================
// UIFactory Implementation
// ==========================================

UISystem::UIElement UISystem::UIFactory::createUIElement(UIElementType type, const std::string& playerId, const UIPosition& position) {
    UIElement element;
    element.type = type;
    element.position = position;
    (void)playerId; // Unused
    // Set default style based on type if needed
    return element;
}

// ==========================================
// UILayoutManager Implementation
// ==========================================

void UISystem::UILayoutManager::applyLayout(std::vector<UIElement*>& elements, LayoutType layoutType, const Vector2& containerSize) {
    switch (layoutType) {
        case LayoutType::ABSOLUTE:
            applyAbsoluteLayout(elements);
            break;
        case LayoutType::FLOW_H:
            applyFlowLayout(elements, true);
            break;
        case LayoutType::FLOW_V:
            applyFlowLayout(elements, false);
            break;
        case LayoutType::GRID:
            applyGridLayout(elements, containerSize);
            break;
        default:
            break;
    }
}

void UISystem::UILayoutManager::makeResponsive(std::vector<UIElement*>& elements, const Vector2& screenSize) {
    // Basic responsiveness logic
    for (auto* element : elements) {
        if (element->position.x > screenSize.x) {
            element->position.x = screenSize.x - element->position.width;
        }
        if (element->position.y > screenSize.y) {
            element->position.y = screenSize.y - element->position.height;
        }
    }
}

void UISystem::UILayoutManager::applyAbsoluteLayout(std::vector<UIElement*>& elements) {
    (void)elements; // Unused
    // Do nothing, keep absolute positions
}

void UISystem::UILayoutManager::applyFlowLayout(std::vector<UIElement*>& elements, bool horizontal) {
    float currentPos = 0.0f;
    for (auto* element : elements) {
        if (horizontal) {
            element->position.x = currentPos;
            currentPos += element->position.width + 5.0f; // 5px padding
        } else {
            element->position.y = currentPos;
            currentPos += element->position.height + 5.0f;
        }
    }
}

void UISystem::UILayoutManager::applyGridLayout(std::vector<UIElement*>& elements, const Vector2& containerSize) {
    // Simple grid implementation
    if (elements.empty()) return;
    
    int cols = static_cast<int>(sqrt(elements.size()));
    if (cols == 0) cols = 1;
    
    float cellWidth = containerSize.x / cols;
    float cellHeight = 100.0f; // Fixed height for now
    
    for (size_t i = 0; i < elements.size(); ++i) {
        int row = static_cast<int>(i) / cols;
        int col = static_cast<int>(i) % cols;
        
        elements[i]->position.x = col * cellWidth;
        elements[i]->position.y = row * cellHeight;
        elements[i]->position.width = cellWidth - 10.0f; // Margin
        elements[i]->position.height = cellHeight - 10.0f;
    }
}

// ==========================================
// UISystem Implementation
// ==========================================

UISystem::UISystem() 
    : m_name("UISystem"), 
      m_nextUIElementId(1),
      m_cacheHits(0),
      m_cacheMisses(0),
      m_totalRenderTime(0),
      m_interactionCount(0),
      m_activeSelectionTab(0),
      m_selectedInventorySlot(-1),
      m_selectedBuilding(nullptr) {
}

UISystem::~UISystem() {
    shutdown();
}

void UISystem::initialize() {
    initializeBaseUIElementTypes();
    std::cout << "UISystem initialized." << std::endl;
}

void UISystem::update(float deltaTime) {
    updateUIAnimations(deltaTime);
    m_cache.cleanup();
}

void UISystem::render() {
    // Render logic would go here, iterating over visible elements
    // For now, we can just iterate over players and render their UI
    for (const auto& playerPair : m_playerUIElements) {
        renderPlayerUI(playerPair.first);
    }
    
    // --- Render Building Owner Names ---
    GameEngine& engine = GameEngine::getInstance();
    auto* buildingSystem = engine.getSystem<BuildingSystem>();
    if (buildingSystem) {
        // ... (code omitted for brevity as it needs camera context)
    }
}

void UISystem::shutdown() {
    m_playerUIElements.clear();
    m_uiElementTypes.clear();
    std::cout << "UISystem shutdown." << std::endl;
}

bool UISystem::registerUIElementType(const UIElementConfig& config) {
    if (m_uiElementTypes.find(config.type) != m_uiElementTypes.end()) {
        return false;
    }
    m_uiElementTypes.emplace(config.type, config);
    return true;
}

std::string UISystem::createUIElement(UIElementType type, const std::string& playerId, const UIPosition& position) {
    std::string id = generateUIElementId();
    UIElement element = UIFactory::createUIElement(type, playerId, position);
    element.id = id;
    
    m_playerUIElements[playerId][id] = std::move(element);
    m_playerElementIds[playerId].push_back(id);
    
    return id;
}

bool UISystem::deleteUIElement(const std::string& elementId, const std::string& playerId) {
    auto& playerElements = m_playerUIElements[playerId];
    auto it = playerElements.find(elementId);
    if (it != playerElements.end()) {
        playerElements.erase(it);
        
        auto& ids = m_playerElementIds[playerId];
        auto idIt = std::find(ids.begin(), ids.end(), elementId);
        if (idIt != ids.end()) {
            ids.erase(idIt);
        }
        return true;
    }
    return false;
}

UISystem::UIElement* UISystem::getUIElement(const std::string& elementId, const std::string& playerId) const {
    auto playerIt = m_playerUIElements.find(playerId);
    if (playerIt != m_playerUIElements.end()) {
        auto elemIt = playerIt->second.find(elementId);
        if (elemIt != playerIt->second.end()) {
            return const_cast<UIElement*>(&elemIt->second);
        }
    }
    return nullptr;
}

std::vector<UISystem::UIElement*> UISystem::getPlayerUIElements(const std::string& playerId) const {
    std::vector<UIElement*> elements;
    auto playerIt = m_playerUIElements.find(playerId);
    if (playerIt != m_playerUIElements.end()) {
        for (auto& pair : playerIt->second) {
            elements.push_back(const_cast<UIElement*>(&pair.second));
        }
    }
    return elements;
}

bool UISystem::updateUIElementPosition(const std::string& elementId, const std::string& playerId, const UIPosition& position) {
    UIElement* element = getUIElement(elementId, playerId);
    if (element) {
        element->position = position;
        return true;
    }
    return false;
}

bool UISystem::updateUIElementText(const std::string& elementId, const std::string& playerId, const std::string& text) {
    UIElement* element = getUIElement(elementId, playerId);
    if (element) {
        element->text = text;
        return true;
    }
    return false;
}

bool UISystem::setUIElementState(const std::string& elementId, const std::string& playerId, UIState state) {
    UIElement* element = getUIElement(elementId, playerId);
    if (element) {
        UIState oldState = element->state;
        element->state = state;
        
        if (oldState != state) {
            UIStateChangedEvent event;
            event.elementId = elementId;
            event.oldState = oldState;
            event.newState = state;
            event.playerId = playerId;
            notifyUIEvent(&event);
        }
        return true;
    }
    return false;
}

bool UISystem::addUIElementAnimation(const std::string& elementId, const std::string& playerId, const UIAnimation& animation) {
    (void)playerId;
    m_activeAnimations[elementId].push_back(animation);
    return true;
}

bool UISystem::clearUIElementAnimations(const std::string& elementId, const std::string& playerId) {
    (void)playerId;
    m_activeAnimations.erase(elementId);
    return true;
}

bool UISystem::createPlayerDashboard(const std::string& playerId) {
    // Create default dashboard elements
    createUIElement(UIElementType::PANEL, playerId, UIPosition(10, 10, 200, 100));
    return true;
}

void UISystem::updatePlayerUI(const std::string& playerId) {
    (void)playerId; // Unused
    // Logic to sync UI with game state
}

bool UISystem::handleUIClick(const std::string& elementId, const std::string& playerId, float clickX, float clickY) {
    UIElement* element = getUIElement(elementId, playerId);
    if (element && element->isEnabled()) {
        if (element->onClick) {
            element->onClick();
        }
        
        UIClickedEvent event;
        event.elementId = elementId;
        event.playerId = playerId;
        event.clickX = clickX;
        event.clickY = clickY;
        event.clickTime = std::chrono::high_resolution_clock::now();
        notifyUIEvent(&event);
        
        m_interactionCount++;
        return true;
    }
    return false;
}

UISystem::UISystemStats UISystem::getStats() const {
    UISystemStats stats;
    stats.totalUIElementTypes = m_uiElementTypes.size();
    stats.totalUIElements = 0;
    for (const auto& pair : m_playerUIElements) {
        stats.totalUIElements += pair.second.size();
    }
    stats.totalAnimations = 0; // Simplify
    stats.cacheHits = m_cacheHits;
    stats.cacheMisses = m_cacheMisses;
    stats.totalInteractions = m_interactionCount;
    return stats;
}

void UISystem::initializeBaseUIElementTypes() {
    auto types = UIFactory::createBaseElementTypes();
    for (const auto& pair : types) {
        m_uiElementTypes[pair.first] = pair.second;
    }
}

std::string UISystem::generateUIElementId() {
    return "ui_" + std::to_string(m_nextUIElementId++);
}

void UISystem::updateUIAnimations(double deltaTime) {
    (void)deltaTime; // Unused
    // Animation update logic
}

void UISystem::renderPlayerUI(const std::string& playerId) {
    auto it = m_playerUIElements.find(playerId);
    if (it != m_playerUIElements.end()) {
        for (const auto& pair : it->second) {
            const UIElement& element = pair.second;
            if (element.isVisible()) {
                // Actual rendering using Raylib or other backend would happen here
                // DrawRectangle(element.position.x, element.position.y, element.position.width, element.position.height, GRAY);
            }
        }
    }
}

void UISystem::lazyLoadPlayerUI(const std::string& playerId) {
    (void)playerId; // Unused
    // Lazy load logic
}

void UISystem::notifyUIEvent(void* event) {
    (void)event; // Unused
    // Event system notification
}

// ==========================================
// IMMEDIATE MODE GUI HELPERS (IMPLEMENTATION)
// ==========================================

void UISystem::DrawResourceBar(int wood, int food, int stone, int screenWidth) {
    int height = 40;
    // Background
    DrawRectangle(0, 0, screenWidth, height, Color{ 30, 30, 30, 230 });
    DrawRectangleLines(0, 0, screenWidth, height, Color{ 100, 100, 100, 255 });

    // Resources
    int startX = 20;
    int gap = 150;
    int fontSize = 20;
    
    // Wood
    DrawText("Wood:", startX, 10, fontSize, BROWN);
    DrawText(std::to_string(wood).c_str(), startX + 60, 10, fontSize, WHITE);
    
    // Food
    DrawText("Food:", startX + gap, 10, fontSize, ORANGE);
    DrawText(std::to_string(food).c_str(), startX + gap + 60, 10, fontSize, WHITE);
    
    // Stone (Mock)
    DrawText("Stone:", startX + gap*2, 10, fontSize, GRAY);
    DrawText(std::to_string(stone).c_str(), startX + gap*2 + 70, 10, fontSize, WHITE);
}

void UISystem::DrawBottomPanel(bool isBuildingMode, bool cameraMode, int screenWidth, int screenHeight) {
    int height = 60; // Action bar height (not building selection)
    int y = screenHeight - height;
    
    // Background
    DrawRectangle(0, y, screenWidth, height, Color{ 20, 20, 20, 240 });
    DrawLine(0, y, screenWidth, y, GRAY);
    
    // Buttons / Hints
    int startX = 20;
    int btnWidth = 150;
    int btnHeight = 40;
    int btnY = y + 10;
    int gap = 20;
    
    // Build Button
    Color buildColor = isBuildingMode ? GREEN : LIGHTGRAY;
    DrawRectangleLines(startX, btnY, btnWidth, btnHeight, buildColor);
    DrawText(isBuildingMode ? "BUILD ACTIVE [B]" : "BUILD MODE [B]", startX + 10, btnY + 10, 10, buildColor);
    
    // Camera Button
    startX += btnWidth + gap;
    Color camColor = cameraMode ? YELLOW : LIGHTGRAY;
    DrawRectangleLines(startX, btnY, btnWidth, btnHeight, camColor);
    DrawText(cameraMode ? "CAM: SETTLER [C]" : "CAM: FREE [C]", startX + 10, btnY + 10, 10, camColor);
    
    // Hints
    startX += btnWidth + gap;
    DrawText("LMB: Select/Action  RMB: Move/Building Info  WASD: Move Cam  TAB: Stats", startX, btnY + 12, 15, DARKGRAY);
}

void UISystem::DrawBuildingSelectionPanel(const std::vector<BuildingBlueprint*>& blueprints, const std::string& selectedId, int screenWidth, int screenHeight) {
    int panelHeight = 140;
    int y = screenHeight - 60 - panelHeight; // Above bottom panel
    
    // Background
    DrawRectangle(0, y, screenWidth, panelHeight, Color{ 40, 40, 40, 200 });
    DrawLine(0, y, screenWidth, y, GRAY);
    
    DrawText("Select Blueprint:", 20, y + 10, 20, WHITE);
    
    int cardWidth = 120;
    int cardHeight = 90;
    int startX = 20;
    int startY = y + 40;
    int gap = 15;
    
    for (size_t i = 0; i < blueprints.size(); ++i) {
        BuildingBlueprint* bp = blueprints[i];
        int x = startX + static_cast<int>(i) * (cardWidth + gap);
        
        bool isSelected = (bp->getId() == selectedId);
        Color bgColor = isSelected ? Color{ 80, 100, 120, 255 } : Color{ 60, 60, 60, 255 };
        Color borderColor = isSelected ? YELLOW : LIGHTGRAY;
        
        // Card Body
        DrawRectangle(x, startY, cardWidth, cardHeight, bgColor);
        DrawRectangleLines(x, startY, cardWidth, cardHeight, borderColor);
        
        // Title
        DrawText(bp->getName().c_str(), x + 5, startY + 5, 10, WHITE);
        
        // Cost
        int resY = startY + 60;
        for(const auto& req : bp->getRequirements()) {
            // Simple cost display
            std::string cost = req.resourceType + ": " + std::to_string(req.amount);
            DrawText(cost.c_str(), x + 5, resY, 10, LIGHTGRAY);
            resY += 12;
        }
        
        // Icon placeholder
        Color iconColor = BROWN;
        if (bp->getId() == "floor") iconColor = DARKBROWN;
        DrawRectangle(x + 40, startY + 25, 40, 30, iconColor);
    }
}

bool UISystem::HandleSelectionPanelClick(int screenWidth, int screenHeight) {
    int width = 250;
    int height = 250;
    int x = screenWidth - width - 10;
    int y = screenHeight - height - 70;
    
    Vector2 mouse = GetMousePosition();
    
    // Check if mouse is within the entire panel
    if (mouse.x >= x && mouse.x <= x + width && mouse.y >= y && mouse.y <= y + height) {
        
        // Check Tabs
        int tabCount = 3;
        int tabWidth = width / tabCount;
        int tabHeight = 25;
        
        for (int i = 0; i < tabCount; ++i) {
            int tx = x + i * tabWidth;
            if (mouse.x >= tx && mouse.x <= tx + tabWidth && mouse.y >= y && mouse.y <= y + tabHeight) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    m_activeSelectionTab = i;
                    // Reset selection when switching tabs
                    m_selectedInventorySlot = -1;
                }
                return true; // Interaction handled
            }
        }

        // Handle Inventory Clicks
        if (m_activeSelectionTab == 2) { // REMOVED explicit IsMouseButtonPressed check here to allow handling Release
            // Use Colony instead of InteractionSystem for selection
            if (!m_colony) return true;

            const auto& settlers = m_colony->getSelectedSettlers();
            
            // Calculate UI slot positions
            int contentY = y + tabHeight + 10;
            int mainHandSize = 40;
            
            // Check Main Hand (Slot 0) - Updated Hitbox
            // Position defined in DrawSelectionInfo: x + 80, contentY + 25
            if (mouse.x >= x + 80 && mouse.x <= x + 80 + mainHandSize && mouse.y >= contentY + 25 && mouse.y <= contentY + 25 + mainHandSize) {
                 
                 if (settlers.size() == 1) {
                     auto* entity = settlers[0];
                     auto inv = entity->getComponent<InventoryComponent>();
                     if (inv) {
                         int clickedSlot = 0;
                         bool actionTriggered = false;
                         bool isPressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
                         bool isReleased = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

                         // Click-to-Select or Click-to-Move
                         if (isPressed) actionTriggered = true;

                         // Drag-and-Drop Release (only if different slot to avoid immediate deselect on click)
                         if (isReleased && m_selectedInventorySlot != -1 && m_selectedInventorySlot != clickedSlot) {
                             actionTriggered = true;
                         }

                         if (actionTriggered) {
                             if (m_selectedInventorySlot == -1) {
                                 m_selectedInventorySlot = clickedSlot;
                             } else {
                                 // Get InventorySystem
                                 GameEngine& engine = GameEngine::getInstance();
                                 auto* invSys = engine.getSystem<InventorySystem>();
                                 if (invSys) {
                                     invSys->moveItem(inv.get(), m_selectedInventorySlot, clickedSlot); // Fixed: use inv.get()
                                     m_selectedInventorySlot = -1;
                                 }
                             }
                         }
                     }
                 }
                 // Return true if ANY mouse interaction happened in this area to block other inputs
                 if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) return true;
            }
            
            // Check Backpack Grid (Slots 1-10)
            int slotSize = 30;
            int slotGap = 5;
            int slotsPerRow = 5;
            
            for(int i = 0; i < 10; ++i) {
                int col = i % slotsPerRow;
                int row = i / slotsPerRow;
                
                int sx = x + 20 + col * (slotSize + slotGap);
                int sy = contentY + 80 + row * (slotSize + slotGap); // shifted down to make room for Main Hand box
                
                if (mouse.x >= sx && mouse.x <= sx + slotSize && mouse.y >= sy && mouse.y <= sy + slotSize) {
                     int clickedSlot = i + 1; // +1 offset because 0 is Main Hand
                     
                     if (settlers.size() == 1) {
                         auto* entity = settlers[0];
                         auto inv = entity->getComponent<InventoryComponent>();
                         if (inv) {
                             bool actionTriggered = false;
                             bool isPressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
                             bool isReleased = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

                             if (isPressed) actionTriggered = true;
                             
                             // Drag-and-Drop Release
                             if (isReleased && m_selectedInventorySlot != -1 && m_selectedInventorySlot != clickedSlot) {
                                 actionTriggered = true;
                             }

                             if (actionTriggered) {
                                 if (m_selectedInventorySlot == -1) {
                                     m_selectedInventorySlot = clickedSlot;
                                 } else {
                                     GameEngine& engine = GameEngine::getInstance();
                                     auto* invSys = engine.getSystem<InventorySystem>();
                                     if (invSys) {
                                         invSys->moveItem(inv.get(), m_selectedInventorySlot, clickedSlot); // Fixed: use inv.get()
                                         m_selectedInventorySlot = -1;
                                     }
                                 }
                             }
                         }
                     }
                     if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) return true;
                }
            }
        }

        return true; // Consumed click inside panel
    }
    
    // Click outside panel -> deselect slot
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        m_selectedInventorySlot = -1;
    }
    
    return false;
}

void UISystem::DrawSelectionInfo(const std::vector<Settler*>& selectedSettlers, int screenWidth, int screenHeight) {
    if (selectedSettlers.empty()) return;
    
    int width = 250;
    int height = 250; // Increased height for skills
    int x = screenWidth - width - 10;
    int y = screenHeight - height - 70; // Above bottom panel
    
    // Background
    DrawRectangle(x, y, width, height, Color{ 20, 30, 40, 230 });
    DrawRectangleLines(x, y, width, height, SKYBLUE);
    
    // Tabs
    int tabCount = 3;
    int tabWidth = width / tabCount;
    int tabHeight = 25;
    
    const char* tabs[] = { "Stats", "Skills", "Inv" };
    
    for (int i = 0; i < tabCount; ++i) {
        int tx = x + i * tabWidth;
        bool isActive = (m_activeSelectionTab == i);
        Color tabColor = isActive ? Color{ 60, 70, 90, 255 } : Color{ 30, 40, 50, 255 };
        Color textColor = isActive ? WHITE : GRAY;
        
        DrawRectangle(tx, y, tabWidth, tabHeight, tabColor);
        DrawRectangleLines(tx, y, tabWidth, tabHeight, SKYBLUE);
        DrawText(tabs[i], tx + 10, y + 5, 10, textColor);
    }
    
    int contentY = y + tabHeight + 10;
    
    // Content
    if (selectedSettlers.size() == 1) {
        Settler* s = selectedSettlers[0];
        
        if (m_activeSelectionTab == 0) { // STATS
            std::string name = s->getName();
            if (name.empty()) name = "Settler";
            DrawText(("Unit: " + name).c_str(), x + 10, contentY, 10, WHITE);
            DrawText(("State: " + s->actionState).c_str(), x + 10, contentY + 15, 10, LIGHTGRAY);
            
            // Housing Info
            std::string housingText = "House: " + std::string(s->hasHouse ? "Yes" : "No"); // Fixed hasHouse usage
            housingText += " (Pref: " + std::to_string((int)s->preferredHouseSize) + ")";
            DrawText(housingText.c_str(), x + 10, contentY + 30, 10, s->hasHouse ? GREEN : RED); // Fixed hasHouse usage

            // Health bar
            DrawText("Health:", x + 10, contentY + 45, 10, WHITE);
            auto stats = s->getComponent<StatsComponent>();
            float hpPct = 0.0f;
            float enPct = 0.0f;
            float hungerPct = 0.0f;
            float fullnessPct = 0.0f;
            
            if (stats) {
                hpPct = stats->getCurrentHealth() / stats->getMaxHealth();
                enPct = stats->getCurrentEnergy() / stats->getMaxEnergy();
                hungerPct = stats->getCurrentHunger() / stats->getMaxHunger();
                fullnessPct = hungerPct; // 0=starving, 1=full
                
                // Debug removed to reduce spam
            }
            
            DrawRectangle(x + 70, contentY + 45, 100, 10, RED);
            DrawRectangle(x + 70, contentY + 45, (int)(100 * hpPct), 10, GREEN);
            
            // Energy bar
            DrawText("Energy:", x + 10, contentY + 65, 10, WHITE);
            DrawRectangle(x + 70, contentY + 65, 100, 10, DARKGRAY);
            DrawRectangle(x + 70, contentY + 65, (int)(100 * enPct), 10, YELLOW);
    
            // Hunger bar (GUI)
            DrawText("Hunger:", x + 10, contentY + 85, 10, WHITE);
            if(fullnessPct < 0) fullnessPct = 0;
            
            Color hungerColor = ORANGE;
            if(fullnessPct < 0.25f) hungerColor = RED;
            
            DrawRectangle(x + 70, contentY + 85, 100, 10, DARKGRAY);
            DrawRectangle(x + 70, contentY + 85, (int)(100 * fullnessPct), 10, hungerColor);
            
        } else if (m_activeSelectionTab == 1) { // SKILLS
            DrawText("Skills:", x + 10, contentY, 10, YELLOW); // Changed GOLD to YELLOW
            int skillY = contentY + 20;
            const auto& skills = s->getSkills().getAllSkills(); // Fixed access via getter
            for (const auto& pair : skills) {
                std::string skillText = pair.second.name + ": Lvl " + std::to_string(pair.second.level);
                DrawText(skillText.c_str(), x + 10, skillY, 10, WHITE);
                
                // XP Bar
                float xpPct = pair.second.currentXP / pair.second.xpToNextLevel;
                DrawRectangle(x + 100, skillY + 2, 80, 6, DARKGRAY);
                DrawRectangle(x + 100, skillY + 2, (int)(80 * xpPct), 6, SKYBLUE);
                
                skillY += 15;
                if (skillY > y + height - 10) break;
            }
            
        } else if (m_activeSelectionTab == 2) { // INVENTORY
            DrawText("Equipment:", x + 10, contentY, 10, YELLOW);
            
            // Main Hand Slot Area - Now as a box
            int mainHandSize = 40;
            int mhX = x + 80;
            int mhY = contentY + 25;
            
            DrawText("Main Hand:", x + 10, mhY + 10, 10, WHITE);
            
            // Selection Highlight for Main Hand (Slot 0)
            Color mhBorderColor = GRAY;
            if (m_selectedInventorySlot == 0) {
                 mhBorderColor = GREEN;
                 DrawRectangleLines(mhX - 2, mhY - 2, mainHandSize + 4, mainHandSize + 4, GREEN);
            }
            DrawRectangleLines(mhX, mhY, mainHandSize, mainHandSize, mhBorderColor);
            
            auto inv = s->getComponent<InventoryComponent>();
            
            // Draw item in Main Hand
            if (inv) {
                InventoryItem* item = inv->getItemAt(0);
                if (item) {
                    DrawRectangle(mhX + 4, mhY + 4, mainHandSize - 8, mainHandSize - 8, BROWN);
                    // If has name, tooltip or icon?
                    DrawText("I", mhX + 12, mhY + 12, 20, WHITE);
                } else {
                    DrawText("Empty", mhX + 5, mhY + 15, 8, DARKGRAY);
                }
            }

            DrawText("Backpack:", x + 10, contentY + 70, 10, WHITE);
            
            // Draw backpack slots
            int slotSize = 30;
            int slotGap = 5;
            int slotsPerRow = 5;
            
            for(int i = 0; i < 10; ++i) {
                int col = i % slotsPerRow;
                int row = i / slotsPerRow;
                
                int sx = x + 20 + col * (slotSize + slotGap);
                int sy = contentY + 80 + row * (slotSize + slotGap);
                
                Color borderColor = GRAY;
                int actualSlotIndex = i + 1; // +1 offset for Main Hand
                
                if (m_selectedInventorySlot == actualSlotIndex) {
                    borderColor = GREEN;
                    DrawRectangleLines(sx - 1, sy - 1, slotSize + 2, slotSize + 2, GREEN);
                }
                
                DrawRectangleLines(sx, sy, slotSize, slotSize, borderColor);
                
                // Draw item if exists
                if (inv) {
                    InventoryItem* item = inv->getItemAt(actualSlotIndex);
                    if (item) {
                        // Simple indicator
                        DrawRectangle(sx + 2, sy + 2, slotSize - 4, slotSize - 4, BROWN);
                        // Amount
                        if (item->quantity > 1) {
                             DrawText(std::to_string(item->quantity).c_str(), sx + 2, sy + 20, 10, WHITE);
                        }
                    }
                }
            }
        }

    } else {
        DrawText(("Selected Units: " + std::to_string(selectedSettlers.size())).c_str(), x + 10, contentY, 10, WHITE);
        DrawText("Group Command Ready", x + 10, contentY + 20, 10, LIGHTGRAY);
    }
}

void UISystem::DrawSettlerStatsOverlay(const std::vector<Settler*>& settlers, Camera3D camera, int screenWidth, int screenHeight) {
    // --- DRAW HOUSE OWNER NAMES ---
    // Logic to draw owner name on top of their house
    GameEngine& engine = GameEngine::getInstance();
    auto* buildingSystem = engine.getSystem<BuildingSystem>();
    
    if (buildingSystem) {
        std::vector<BuildingInstance*> buildings = buildingSystem->getBuildingsInRange(camera.target, 500.0f);
        
        for (auto* b : buildings) {
            if (b->getBlueprintId().find("house") == 0) {
                std::string owner = b->getOwner();
                if (!owner.empty()) {
                     Vector3 housePos = b->getPosition();
                     housePos.y += 4.0f; // Above roof
                     
                     Vector2 screenPos = GetWorldToScreen(housePos, camera);
                     if (screenPos.x >= -100 && screenPos.x <= screenWidth + 100 && 
                         screenPos.y >= -100 && screenPos.y <= screenHeight + 100) {
                         
                         std::string label = owner + "'s House";
                         int width = MeasureText(label.c_str(), 20);
                         DrawText(label.c_str(), (int)screenPos.x - width/2, (int)screenPos.y, 20, YELLOW);
                     }
                }
            }
        }
    }

    for (const auto* s : settlers) {
        // Calculate position above head
        Vector3 headPos = s->getPosition();
        headPos.y += 2.5f; // Slightly above head
        
        // Project to screen
        Vector2 screenPos = GetWorldToScreen(headPos, camera);
        
        // Check if visible/on-screen (simple check)
        if (screenPos.x >= -100 && screenPos.x <= screenWidth + 100 && 
            screenPos.y >= -100 && screenPos.y <= screenHeight + 100) {
            
            int panelW = 80; // Wider for skills text
            int panelH = 45; // Taller for skills text
            int startX = (int)screenPos.x - panelW/2;
            int startY = (int)screenPos.y - panelH;
            
            // Background
            DrawRectangle(startX, startY, panelW, panelH, Color{ 0, 0, 0, 180 });
            DrawRectangleLines(startX, startY, panelW, panelH, DARKGRAY);
            
            // Get Stats
            auto stats = s->getComponent<StatsComponent>();
            float hpPct = 1.0f;
            float enPct = 1.0f;
            
            if (stats) {
                hpPct = stats->getCurrentHealth() / stats->getMaxHealth();
                enPct = stats->getCurrentEnergy() / stats->getMaxEnergy();
            }
            
            // Health Bar
            DrawRectangle(startX + 2, startY + 5, panelW - 4, 6, RED);
            DrawRectangle(startX + 2, startY + 5, (int)((panelW - 4) * hpPct), 6, GREEN);
            
            // Energy Bar
            DrawRectangle(startX + 2, startY + 14, panelW - 4, 6, DARKBLUE);
            DrawRectangle(startX + 2, startY + 14, (int)((panelW - 4) * enPct), 6, YELLOW);
            
            // Skills Summary (e.g., "W:5 B:2")
            std::string summary = "";
            const auto& skills = s->getSkills(); // Fixed access via getter
            // Assuming we want Woodcutting and Building
            if (skills.hasSkill(SkillType::WOODCUTTING)) {
                summary += "W:" + std::to_string(skills.getSkillLevel(SkillType::WOODCUTTING)) + " ";
            }
            if (skills.hasSkill(SkillType::BUILDING)) {
                summary += "B:" + std::to_string(skills.getSkillLevel(SkillType::BUILDING));
            }
            
            DrawText(summary.c_str(), startX + 4, startY + 25, 10, WHITE);
        }
    }
}

void UISystem::ShowBuildingInfo(BuildingInstance* building, int screenWidth, int screenHeight) {
    if (!building) return;
    
    int width = 200;
    int height = 150;
    int x = screenWidth - width - 10;
    int y = 100;
    
    DrawRectangle(x, y, width, height, Color{ 20, 20, 20, 230 });
    DrawRectangleLines(x, y, width, height, YELLOW);
    
    DrawText("Building Info", x + 10, y + 10, 20, WHITE);
    DrawText(("Type: " + building->getBlueprintId()).c_str(), x + 10, y + 40, 10, LIGHTGRAY);
    
    if (building->isBuilt()) { // Fixed: isConstructed() -> isBuilt()
        DrawText("Status: Completed", x + 10, y + 60, 10, GREEN);
        
        std::string owner = building->getOwner();
        if (owner.empty()) owner = "None";
        DrawText(("Owner: " + owner).c_str(), x + 10, y + 80, 10, WHITE);
    } else {
        DrawText("Status: Under Construction", x + 10, y + 60, 10, ORANGE);
    }
}