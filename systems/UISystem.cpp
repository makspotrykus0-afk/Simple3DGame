#include "raylib.h"

#include "raymath.h"

#include "UISystem.h"
#include "EditorSystem.h"
#include "../core/GameEngine.h"

#include "../systems/InteractionSystem.h"

#include "../game/Colony.h"
#include <iostream>



#include "../components/SkillsComponent.h"

#include "../systems/CraftingSystem.h"
#include <iostream>
#include "../core/Logger.h"

#include <algorithm>
// ==========================================

// UIElement Implementation

// ==========================================
Vector3 UISystem::UIElement::getScreenPosition() const {

return Vector3{position.x, position.y, 0.0f};

}
// ==========================================

// UIFactory Implementation

// ==========================================
UISystem::UIElement UISystem::UIFactory::createUIElement(UIElementType type, const std::string& playerId, const UIPosition& position) {

UIElement element;

element.type = type;

element.position = position;

(void)playerId;

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

(void)elements;

}
void UISystem::UILayoutManager::applyFlowLayout(std::vector<UIElement*>& elements, bool horizontal) {

float currentPos = 0.0f;

for (auto* element : elements) {

if (horizontal) {

element->position.x = currentPos;

currentPos += element->position.width + 5.0f;

} else {

element->position.y = currentPos;

currentPos += element->position.height + 5.0f;

}

}

}
void UISystem::UILayoutManager::applyGridLayout(std::vector<UIElement*>& elements, const Vector2& containerSize) {

if (elements.empty()) return;

int cols = static_cast<int>(sqrt(elements.size()));

if (cols == 0) cols = 1;

float cellWidth = containerSize.x / static_cast<float>(cols);

float cellHeight = 100.0f;

for (size_t i = 0; i < elements.size(); ++i) {

int row = static_cast<int>(i) / cols;

int col = static_cast<int>(i) % cols;

elements[i]->position.x = col * cellWidth;

elements[i]->position.y = row * cellHeight;

elements[i]->position.width = cellWidth - 10.0f;

elements[i]->position.height = cellHeight - 10.0f;

}

}
// ==========================================

// UISystem Implementation

// ==========================================
UISystem::UISystem()
: m_name("UISystem"),
m_cacheHits(0),
m_cacheMisses(0),
m_totalRenderTime(0),
m_interactionCount(0),
m_nextUIElementId(1),
m_activeSelectionTab(0),
m_selectedInventorySlot(-1),
m_selectedBuilding(nullptr),
m_colony(nullptr) {
}
UISystem::~UISystem() {

shutdown();

}
void UISystem::initialize() {
    initializeBaseUIElementTypes();
    
    // Load icons texture (Visual Parity Constitution rule)
    m_iconsTexture = LoadTexture("assets/ui/icons.png");
    if (m_iconsTexture.id != 0) {
        m_isIconsLoaded = true;
        std::cout << "[UISystem] Icons texture loaded successfully." << std::endl;
    } else {
        std::cout << "[UISystem] WARNING: Failed to load icons texture from assets/ui/icons.png!" << std::endl;
    }

    std::cout << "UISystem initialized." << std::endl;
}
void UISystem::update(float deltaTime) {
    m_uiAnimationTime += deltaTime;
    updateUIAnimations(deltaTime);
    m_cache.cleanup();
}
void UISystem::render() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // 1. Resource Bar (Command Module AAA)
    // Avoid overlap with Editor GUI (top-right)
    bool editorActive = false;
    auto editor = GameEngine::getInstance().getSystem<EditorSystem>();
    if (editor && editor->HasSelection()) {
        editorActive = true;
    }

    if (m_colony && !editorActive) {
        int w = m_colony->getWood();
        int f = m_colony->getFood();
        int s = m_colony->getStone();
        int pop = (int)m_colony->getSettlers().size();
        DrawPremiumResourceBar(w, f, s, pop, sw);
    }

    // 2. Log Console
    if (m_showMissionLog) {
        DrawReactiveLogConsole(sw, sh);
    }

    // 3. Selection Info (Settler or Building)
    if (m_colony) {
        const auto& selected = m_colony->getSelectedSettlers();
        if (!selected.empty()) {
            DrawSettlerSelectedPanel(selected[0], sw, sh);
        } else if (m_selectedBuilding) {
            ShowBuildingInfo(m_selectedBuilding, sw);
        }
    }

    // Existing render logic for player UI components
    for (const auto& playerPair : m_playerUIElements) {
        renderPlayerUI(playerPair.first);
    }

    if (m_isCraftingPanelVisible) {
        DrawCraftingPanel(sw, sh);
    }
    
    // 4. Colony Stats (Tab)
    if (m_showColonyStats) {
        DrawColonyStatsPanel(sw);
    }

    // Bottom panel (Standard but with themes)
    DrawBottomPanel(m_isBuildingMode, false, sw); // cameraMode fallback
}
void UISystem::shutdown() {
    if (m_isIconsLoaded) {
        UnloadTexture(m_iconsTexture);
        m_isIconsLoaded = false;
    }

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

createUIElement(UIElementType::PANEL, playerId, UIPosition(10, 10, 200, 100));

return true;

}
void UISystem::updatePlayerUI(const std::string& playerId) {

(void)playerId;

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

stats.totalAnimations = 0;

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

(void)deltaTime;

}
void UISystem::renderPlayerUI(const std::string& playerId) {

auto it = m_playerUIElements.find(playerId);

if (it != m_playerUIElements.end()) {

for (const auto& pair : it->second) {

const UIElement& element = pair.second;

if (element.isVisible()) {

// Rendering logic

}

}

}

}
void UISystem::lazyLoadPlayerUI(const std::string& playerId) {

(void)playerId;

}
void UISystem::notifyUIEvent(void* event) {

(void)event;

}
// Bottom command panel
void UISystem::DrawBottomPanel(bool isBuildingMode, bool cameraMode, int screenWidth) {
    int screenHeight = GetScreenHeight();
    int height = 75;
    int y = screenHeight - height;
    
    // 1. BASE MODULE (Angular Aggression)
    // Draw a trapezoidal base for the center section
    
    // Background Glow
    DrawRectangleGradientV(0, screenHeight - 100, screenWidth, 100, Fade(COLOR_NEON_TEAL, 0.0f), Fade(COLOR_NEON_TEAL, 0.15f));
    
    // Bottom Bar Frame
    DrawRectangle(0, y, screenWidth, height, { 15, 20, 25, 240 });
    DrawLineEx({0, (float)y}, {(float)screenWidth, (float)y}, 2.0f, COLOR_NEON_TEAL);
    
    // Decorative scanline on bar
    float barScanY = y + (fmodf(m_uiAnimationTime * 10.0f, (float)height));
    DrawLineEx({0, barScanY}, {(float)screenWidth, barScanY}, 1.0f, Fade(COLOR_NEON_TEAL, 0.05f));

    int startX = 40;
    int btnWidth = 180;
    int btnHeight = 45;
    int btnY = y + 15;
    int gap = 15;
    
    auto DrawCommandButton = [&](const char* label, bool active, Color col, float x, const char* shortcut) {
        Rectangle rec = { x, (float)btnY, (float)btnWidth, (float)btnHeight };
        
        // Button Glass
        DrawRectangleRec(rec, { 25, 30, 35, 200 });
        if (active) {
            DrawRectangleRec(rec, Fade(col, 0.2f));
            DrawRectangleRoundedLinesEx(rec, 0.05f, 5, 2.0f, col);
        } else {
            DrawRectangleRoundedLinesEx(rec, 0.05f, 5, 1.0f, Fade(WHITE, 0.2f));
        }

        // Bracket Details
        DrawLineEx({rec.x, rec.y}, {rec.x + 10, rec.y}, 2.0f, active ? col : Fade(WHITE, 0.3f));
        DrawLineEx({rec.x, rec.y}, {rec.x, rec.y + 10}, 2.0f, active ? col : Fade(WHITE, 0.3f));

        DrawText(label, (int)rec.x + 15, (int)rec.y + 15, 12, active ? WHITE : GRAY);
        DrawText(shortcut, (int)rec.x + btnWidth - 30, (int)rec.y + 5, 9, Fade(col, 0.5f));
    };

    DrawCommandButton("BUILD_MODE", isBuildingMode, COLOR_NEON_TEAL, (float)startX, "B");
    startX += btnWidth + gap;
    DrawCommandButton("OPTICS_VIEW", cameraMode, COLOR_NEON_GOLD, (float)startX, "C");
    startX += btnWidth + gap;
    DrawCommandButton("STATION_CRAFT", m_isCraftingPanelVisible, COLOR_NEON_ORANGE, (float)startX, "K");
    
    // Help Tooltip (Right side, technical look)
    int helpX = screenWidth - 450;
    DrawText("LMB: SELECT // RMB: ORDER // WASD: NAV // TAB: STATS", helpX, y + 25, 10, Fade(COLOR_NEON_TEAL, 0.6f));
    DrawRectangle(screenWidth - 460, y + 25, 4, 10, COLOR_NEON_TEAL);

    // CLICK HANDLER
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 m = GetMousePosition();
        if (CheckCollisionPointRec(m, { (float)40 + (btnWidth + gap) * 2, (float)btnY, (float)btnWidth, (float)btnHeight })) {
            m_isCraftingPanelVisible = !m_isCraftingPanelVisible;
        }
    }
}
void UISystem::DrawBuildingSelectionPanel(const std::vector<BuildingBlueprint*>& blueprints, const std::string& selectedId, int screenWidth) {
    int screenHeight = GetScreenHeight();
    int cardWidth = 160;
    int cardHeight = 100;
    int padding = 15;
    int panelHeight = cardHeight + 40;
    int y = screenHeight - 80 - panelHeight;

    Rectangle panelRec = { 0, (float)y, (float)screenWidth, (float)panelHeight };
    DrawPremiumPanel(panelRec, "CONSTRUCTION MENU", 0.95f);

    int totalWidth = static_cast<int>(blueprints.size()) * (cardWidth + padding) + padding;
    int startX = (screenWidth - totalWidth) / 2;
    int startY = y + 50;

    for (size_t i = 0; i < blueprints.size(); ++i) {
        BuildingBlueprint* bp = blueprints[i];
        int x = startX + padding + static_cast<int>(i) * (cardWidth + padding);
        bool isSelected = (bp->getId() == selectedId);
        
        Rectangle cardRec = { (float)x, (float)startY, (float)cardWidth, (float)cardHeight };
        
        // Card Background
        Color cardBg = isSelected ? Fade(COLOR_NEON_TEAL, 0.15f) : Color{ 35, 45, 55, 200 };
        DrawRectangleRounded(cardRec, 0.15f, 8, cardBg);
        
        // Border
        Color borderColor = isSelected ? COLOR_NEON_TEAL : Fade(WHITE, 0.2f);
        DrawRectangleRoundedLinesEx(cardRec, 0.15f, 8, isSelected ? 3.0f : 1.5f, borderColor);
        
        // Icon (Using building icon from sheet, index 3)
        Rectangle iconRec = { (float)x + 10, (float)startY + 10, 40, 40 };
        DrawRectangleRounded(iconRec, 0.2f, 5, { 25, 30, 35, 255 });
        if (m_isIconsLoaded) {
            float texW = (float)m_iconsTexture.width;
            float texH = (float)m_iconsTexture.height;
            float iW = texW / 4.0f; float iH = texH / 4.0f;
            Rectangle srcRec = { static_cast<float>(8 % 4) * iW, static_cast<float>(8 / 4) * iH, iW, iH };
            DrawTexturePro(m_iconsTexture, srcRec, { iconRec.x + 4, iconRec.y + 4, 32, 32 }, { 0, 0 }, 0.0f, WHITE);
        } else {
            DrawText("B", (int)iconRec.x + 12, (int)iconRec.y + 10, 20, COLOR_NEON_GOLD);
        }

        // Blueprint Name
        DrawText(bp->getName().c_str(), x + 60, startY + 12, 13, isSelected ? COLOR_NEON_TEAL : WHITE);
        
        // Requirements
        int resY = startY + 55;
        for(const auto& req : bp->getRequirements()) {
            std::string cost = req.resourceType + ": " + std::to_string(req.amount);
            DrawText(cost.c_str(), x + 10, resY, 10, Fade(WHITE, 0.7f));
            resY += 12;
        }
    }
}

std::string UISystem::HandleBuildingSelectionPanelClick(const std::vector<BuildingBlueprint*>& blueprints, int screenWidth, int screenHeight) {

if (blueprints.empty()) return "";

int cardWidth = 140;

int cardHeight = 80;

int padding = 10;

int panelHeight = cardHeight + 20;

int startY = screenHeight - 60 - panelHeight;

int totalWidth = static_cast<int>(blueprints.size()) * (cardWidth + padding) + padding;

int startX = (screenWidth - totalWidth) / 2;

Vector2 mousePos = GetMousePosition();

if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

    if (mousePos.y >= startY && mousePos.y <= startY + panelHeight &&
        mousePos.x >= startX && mousePos.x <= startX + totalWidth) {

        for (size_t i = 0; i < blueprints.size(); ++i) {

            int x = startX + padding + static_cast<int>(i) * (cardWidth + padding);

            int y = startY + 10;

            if (mousePos.x >= x && mousePos.x <= x + cardWidth &&
                mousePos.y >= y && mousePos.y <= y + cardHeight) {

                return blueprints[i]->getId();

            }

        }

    }

}

return "";


}
bool UISystem::HandleSelectionPanelClick(int screenWidth, int screenHeight) {
    (void)screenHeight;
    if (!m_colony) return false;
    const auto& settlers = m_colony->getSelectedSettlers();
    if (settlers.empty()) return false;
    Settler* settler = settlers[0];

    // Reconstruct Layout Metrics (Must match DrawSettlerSelectedPanel)
    int width = 280;
    int height = 500;
    int startX = screenWidth - 300; 
    int startY = 100;
    
    Vector2 mouse = GetMousePosition();
    
    // Check Panel Bounds
    if (mouse.x < startX || mouse.x > startX + width || mouse.y < startY || mouse.y > startY + height) {
        return false;
    }
    // --- TAB CLICK HANDLING ---
    // Check Close Button (X)
    Rectangle closeBtn = { (float)startX + width - 35, (float)startY + 15, 20, 20 };
    if (CheckCollisionPointRec(mouse, closeBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (m_colony) m_colony->clearSelection();
        return true;
    }

    // --- TAB CLICK HANDLING ---
    int tabCount = 4;
    int tabW = (width - 20) / tabCount;
    int tabH = 25;
    // Calculation must match DrawSettlerSelectedPanel exactly:
    // rect.y(100) + 65(portrait offset) + 80(portrait height) + 15(margin)
    int tabY = startY + 65 + 80 + 15; 
    
    if (mouse.y >= tabY && mouse.y <= tabY + tabH) {
        for(int i=0; i<tabCount; ++i) {
             int tX = startX + 10 + i * tabW;
             if (mouse.x >= tX && mouse.x < tX + tabW) {
                 if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                     m_activeSelectionTab = i;
                     return true;
                 }
             }
        }
    }
    
    // --- CONTENT CLICK HANDLING ---
    int contentY = tabY + tabH + 10;
    
    if (m_activeSelectionTab == 1) { // NEURO (Skills)
         int rowY = contentY + 5;
         const auto& skills = settler->getSkills().getAllSkills();
         // Needed to iterate in same order as Draw: map is sorted by key
         for(auto& pair : skills) { 
              // Left Arrow "<"
              Rectangle leftArr = { (float)startX + 135, (float)rowY - 2, 15, 15 };
              // Right Arrow ">"
              Rectangle rightArr = { (float)startX + 165, (float)rowY - 2, 15, 15 };
              
              if (CheckCollisionPointRec(mouse, leftArr) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                  int newP = pair.second.priority - 1;
                  settler->getSkills().setSkillPriority(pair.second.type, newP);
                  return true;
              }
              if (CheckCollisionPointRec(mouse, rightArr) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                  int newP = pair.second.priority + 1;
                  settler->getSkills().setSkillPriority(pair.second.type, newP);
                  return true;
              }
              rowY += 25;
              if (rowY > startY + height - 20) break;
         }
    }
    else if (m_activeSelectionTab == 3) { // DUTY (Jobs)
        int rowY = contentY + 5;
        // Mapping must match Draw loop order!
        bool* toggles[] = {
            &settler->gatherFood,
            &settler->gatherStone,
            &settler->gatherWood,
            &settler->performBuilding,
            &settler->huntAnimals,
            &settler->haulToStorage
        };
        
        for(bool* val : toggles) {
             Rectangle toggleRec = { (float)startX + 180, (float)rowY, 40, 16 };
             if (CheckCollisionPointRec(mouse, toggleRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                 *val = !(*val);
                 settler->OnJobConfigurationChanged();
                 return true;
             }
             rowY += 22;
        }
    }

    return true; // Consumed click within panel
}

bool UISystem::IsMouseOverUI() {
    Vector2 mouse = GetMousePosition();
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // 1. Bottom Panel
    int bottomHeight = 60;
    if (mouse.y >= screenHeight - bottomHeight) return true;

    // 2. Building mode / Build tab
    if (m_isBuildingMode) {
        int cardHeight = 80;
        int panelHeight = cardHeight + 20;
        int panelY = screenHeight - bottomHeight - panelHeight;
        if (mouse.y >= panelY && mouse.y < screenHeight - bottomHeight) return true;
    }

    // 3. Settler Selected Panel (Premium)
    if (m_colony && !m_colony->getSelectedSettlers().empty()) {
        int width = 280;
        int height = 500;
        int x = screenWidth - 300;
        int y = 100;
        if (mouse.x >= x && mouse.x <= x + width && mouse.y >= y && mouse.y <= y + height) return true;
    }

    // 4. Building Info Panel
    if (m_selectedBuilding) {
        int width = 200;
        int height = 120;
        int x = screenWidth - width - 10;
        int y = 50;
        if (mouse.x >= x && mouse.x <= x + width && mouse.y >= y && mouse.y <= y + height) return true;
    }

    // 5. Crafting Panel
    if (m_isCraftingPanelVisible) {
        int width = 300;
        int height = 400;
        int x = (screenWidth - width) / 2;
        int y = (screenHeight - height) / 2;
        if (mouse.x >= x && mouse.x <= x + width && mouse.y >= y && mouse.y <= y + height) return true;
    }

    // 6. Colony Stats Panel
    if (m_showColonyStats) {
        Rectangle rec = { 50, 80, (float)screenWidth - 100, 500 };
        if (CheckCollisionPointRec(mouse, rec)) return true;
    }

    return false;
}
void UISystem::DrawSelectionInfo(const std::vector<Settler*>& selectedSettlers, int screenWidth, int screenHeight) {

if (selectedSettlers.empty()) return;

int width = 250;

int height = 280;

int x = screenWidth - width - 10;

int y = screenHeight - height - 70;
DrawRectangle(x, y, width, height, Color{ 20, 30, 40, 230 });

DrawRectangleLines(x, y, width, height, SKYBLUE);

int tabCount = 4;

int tabWidth = width / tabCount;

int tabHeight = 25;
const char* tabs[] = { "Stats", "Skills", "Inv", "Jobs" };

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
if (selectedSettlers.size() == 1) {

    Settler* s = selectedSettlers[0];

    if (m_activeSelectionTab == 0) { // Stats

        std::string name = s->getName();

        if (name.empty()) name = "Settler";

        DrawText(("Unit: " + name).c_str(), x + 10, contentY, 10, WHITE);

        DrawText(("State: " + s->actionState).c_str(), x + 10, contentY + 15, 10, LIGHTGRAY);

        std::string housingText = "House: " + std::string(s->hasHouse ? "Yes" : "No");

        housingText += " (Pref: " + std::to_string((int)s->preferredHouseSize) + ")";

        DrawText(housingText.c_str(), x + 10, contentY + 30, 10, s->hasHouse ? GREEN : RED);

        DrawText("Health:", x + 10, contentY + 45, 10, WHITE);
            const StatsComponent* stats = &s->getStats();
            float hpPct = 0.0f;
            float enPct = 0.0f;
            float fullnessPct = 0.0f;
            if (stats) {
                hpPct = stats->getCurrentHealth() / stats->getMaxHealth();
                fullnessPct = stats->getCurrentHunger() / stats->getMaxHunger();
                if (stats->getMaxEnergy() > 0.0f) {
                    enPct = stats->getCurrentEnergy() / stats->getMaxEnergy();
                    enPct = std::clamp(enPct, 0.0f, 1.0f);
                }
            }
        DrawRectangle(x + 70, contentY + 45, (int)(100 * hpPct), 10, GREEN);
        DrawText("Energy:", x + 10, contentY + 65, 10, WHITE);
        
        int energyMaxWidth = 100;
        int energyCurrentWidth = (int)(energyMaxWidth * enPct);
        DrawRectangle(x + 70, contentY + 65, energyMaxWidth, 10, DARKBLUE);
        DrawRectangle(x + 70, contentY + 65, energyCurrentWidth, 10, SKYBLUE);
        DrawText("Hunger:", x + 10, contentY + 85, 10, WHITE);
        
        int hungerMaxWidth = 100;
        int hungerCurrentWidth = (int)(hungerMaxWidth * fullnessPct);
        Color hungerColor = ORANGE;
        if(fullnessPct < 0.25f) hungerColor = RED;
        else if (fullnessPct > 0.75f) hungerColor = GREEN;
        DrawRectangle(x + 70, contentY + 85, hungerMaxWidth, 10, DARKGRAY);
        DrawRectangle(x + 70, contentY + 85, hungerCurrentWidth, 10, hungerColor);
        
    } else if (m_activeSelectionTab == 1) { // Skills
        DrawText("Skills:", x + 10, contentY, 10, YELLOW);
        int skillY = contentY + 20;
        const auto& skills = s->getSkills().getAllSkills();
        for (const auto& pair : skills) {
            std::string skillText = pair.second.name + ": Lvl " + std::to_string(pair.second.level);
            DrawText(skillText.c_str(), x + 10, skillY, 10, WHITE);
            float xpPct = pair.second.currentXP / pair.second.xpToNextLevel;
            DrawRectangle(x + 100, skillY + 2, 80, 6, DARKGRAY);
            DrawRectangle(x + 100, skillY + 2, (int)(80 * xpPct), 6, SKYBLUE);
            skillY += 15;
            if (skillY > y + height - 10) break;
        }
        int autoEquipY = skillY + 5;
        DrawText("Auto Equip Best:", x + 10, autoEquipY, 10, LIGHTGRAY);
        int checkSize = 12;
        Color aeColor = s->getSkills().autoEquipBestItems ? GREEN : DARKGRAY;
        DrawRectangle(x + 100, autoEquipY, checkSize, checkSize, aeColor);
        DrawRectangleLines(x + 100, autoEquipY, checkSize, checkSize, LIGHTGRAY);
        
    } else if (m_activeSelectionTab == 2) { // Inventory
        DrawText("Equipment:", x + 10, contentY, 10, YELLOW);
        int mainHandSize = 40;
        int mhX = x + 80;
        int mhY = contentY + 25;
        DrawText("Main Hand:", x + 10, mhY + 10, 10, WHITE);
        Color mhBorderColor = GRAY;
        if (m_selectedInventorySlot == 0) {
            mhBorderColor = GREEN;
            DrawRectangleLines(mhX - 2, mhY - 2, mainHandSize + 4, mainHandSize + 4, GREEN);
        }
        DrawRectangleLines(mhX, mhY, mainHandSize, mainHandSize, mhBorderColor);
        
        // CRITICAL: Check held item FIRST (e.g. crafted knife), then inventory slot 0
        const Item* heldItem = s->getHeldItem();
        auto* inv = &s->getInventory();
        
        if (heldItem) {
            // Draw held item (e.g. knife)
            DrawRectangle(mhX + 4, mhY + 4, mainHandSize - 8, mainHandSize - 8, DARKGRAY);
            // Draw first letter of item name
            std::string itemName = heldItem->getDisplayName();
            if (!itemName.empty()) {
                DrawText(TextFormat("%c", itemName[0]), mhX + 12, mhY + 12, 20, YELLOW);
            }
        } else if (inv) {
            // Fall back to inventory slot 0
            InventoryItem* item = inv->getItemAt(0);
            if (item) {
                DrawRectangle(mhX + 4, mhY + 4, mainHandSize - 8, mainHandSize - 8, BROWN);
                DrawText("I", mhX + 12, mhY + 12, 20, WHITE);
            } else {
                DrawText("Empty", mhX + 5, mhY + 15, 8, DARKGRAY);
            }
        } else {
            DrawText("Empty", mhX + 5, mhY + 15, 8, DARKGRAY);
        }
        DrawText("Backpack:", x + 10, contentY + 70, 10, WHITE);
        int slotSize = 30;
        int slotGap = 5;
        int slotsPerRow = 5;
        for(int i = 0; i < 10; ++i) {
            int col = i % slotsPerRow;
            int row = i / slotsPerRow;
            int sx = x + 20 + col * (slotSize + slotGap);
            int sy = contentY + 80 + row * (slotSize + slotGap);
            Color borderColor = GRAY;
            int actualSlotIndex = i; // FIXED: Inventory is 0-indexed
            if (m_selectedInventorySlot == actualSlotIndex) {
                borderColor = GREEN;
                DrawRectangleLines(sx - 1, sy - 1, slotSize + 2, slotSize + 2, GREEN);
            }
            DrawRectangleLines(sx, sy, slotSize, slotSize, borderColor);
            if (inv) {
                InventoryItem* item = inv->getItemAt(actualSlotIndex);
                if (item && item->item) {
                    // DEBUG: Log first slot content occassionally
                    if (actualSlotIndex == 0) {
                         // static float logTimer = 0.0f;
                         // logTimer += GetFrameTime();
                         // if (logTimer > 2.0f) {
                         //     std::cout << "[UI] Slot 0: " << item->item->getDisplayName() << " (Type: " << (int)item->item->getItemType() << ")" << std::endl;
                         //     logTimer = 0.0f;
                         // }
                    }

                    Color itemColor = BROWN;
                    std::string label = "";
                    if (item->item->getItemType() == ItemType::RESOURCE) {
                        ResourceItem* ri = dynamic_cast<ResourceItem*>(item->item.get());
                        if (ri) {
                            if (ri->getResourceType() == "Wood") { itemColor = DARKBROWN; label = "LOG"; }
                            else if (ri->getResourceType() == "Stone") { itemColor = GRAY; label = "ROCK"; }
                            else if (ri->getResourceType() == "Food") { itemColor = RED; label = "FOOD"; }
                            else if (ri->getResourceType() == "Metal") { itemColor = DARKGRAY; label = "MET"; }
                            else if (ri->getResourceType() == "Gold") { itemColor = GOLD; label = "GLD"; }
                        }
                    } else if (item->item->getItemType() == ItemType::CONSUMABLE) {
                        itemColor = GREEN;
                        label = "FOOD";
                        if (item->item->getDisplayName() == "Raw Meat") {
                             itemColor = MAROON;
                             label = "MEAT";
                        }
                    } else if (item->item->getItemType() == ItemType::TOOL) {
                        itemColor = DARKGRAY; label = "TOOL";
                    }
                    
                    // Draw item box
                    DrawRectangle(sx + 2, sy + 2, slotSize - 4, slotSize - 4, itemColor);
                    if (!label.empty()) DrawText(label.c_str(), sx + 4, sy + 4, 8, WHITE);
                    if (item->quantity > 1) DrawText(std::to_string(item->quantity).c_str(), sx + 2, sy + 20, 10, WHITE);
                }
            }
        }
        
    } else if (m_activeSelectionTab == 3) { // Jobs
        DrawText("Activity Preferences:", x + 10, contentY, 10, YELLOW);
        int checkSize = 14;
        int rowHeight = 22;
        int colWidth = 110;
        struct JobDisplay { const char* label; bool value; };
        JobDisplay jobs[] = {
            {"Wood", s->gatherWood},
            {"Stone", s->gatherStone},
            {"Food", s->gatherFood},
            {"Build", s->performBuilding},
            {"Hunt", s->huntAnimals},
            {"Craft", s->craftItems},
            {"Haul", s->haulToStorage},
            {"Crops", s->tendCrops}
        };
        for (int i = 0; i < 8; ++i) {
            int col = i % 2;
            int row = i / 2;
            int checkX = x + 15 + col * colWidth;
            int checkY = contentY + 20 + row * rowHeight;
            Color textColor = jobs[i].value ? WHITE : GRAY;
            Color boxColor = jobs[i].value ? GREEN : DARKGRAY;
            DrawText(jobs[i].label, checkX + checkSize + 5, checkY, 10, textColor);
            DrawRectangle(checkX, checkY, checkSize, checkSize, boxColor);
            DrawRectangleLines(checkX, checkY, checkSize, checkSize, LIGHTGRAY);
        }
    }

} else {

    DrawText(("Selected Units: " + std::to_string(selectedSettlers.size())).c_str(), x + 10, contentY, 10, WHITE);

    DrawText("Group Command Ready", x + 10, contentY + 20, 10, LIGHTGRAY);

}


}
void UISystem::DrawSettlerOverheadUI(const Settler& settler, const Camera& camera) {

Vector3 headPos = settler.getPosition();

headPos.y += 2.5f;

Vector3 toPoint = Vector3Subtract(headPos, camera.position);

Vector3 camForward = Vector3Subtract(camera.target, camera.position);

if (Vector3DotProduct(toPoint, camForward) > 0.0f) {

    Vector2 screenPos = GetWorldToScreen(headPos, camera);

    int screenWidth = GetScreenWidth();

    int screenHeight = GetScreenHeight();

    if (screenPos.x >= -100 && screenPos.x <= screenWidth + 100 &&
        screenPos.y >= -100 && screenPos.y <= screenHeight + 100) {
        
        int panelW = 100;
        int panelH = 50;
        int startX = (int)screenPos.x - panelW / 2;
        int startY = (int)screenPos.y - panelH;
        
        DrawRectangle(startX, startY, panelW, panelH, Color{ 0, 0, 0, 180 });
        DrawRectangleLines(startX, startY, panelW, panelH, GRAY);
        
        const auto& stats = settler.getStats();
        float hpPct = 0.0f;
        if (stats.getMaxHealth() > 0) hpPct = stats.getCurrentHealth() / stats.getMaxHealth();
        float enPct = 0.0f;
        if (stats.getMaxEnergy() > 0) enPct = stats.getCurrentEnergy() / stats.getMaxEnergy();
        
        DrawRectangle(startX + 4, startY + 4, panelW - 8, 6, RED);
        DrawRectangle(startX + 4, startY + 4, (int)((panelW - 8) * hpPct), 6, GREEN);
        
   DrawRectangle(startX + 4, startY + 12, panelW - 8, 4, DARKGRAY);
   DrawRectangle(startX + 4, startY + 12, (int)((panelW - 8) * enPct), 4, BLUE);
        // Pasek postępu craftingu
        if (settler.getState() == SettlerState::CRAFTING) {
            float craftPct = settler.GetCraftingProgress01();
            DrawRectangle(startX + 4, startY + 20, panelW - 8, 4, DARKGRAY);
            DrawRectangle(startX + 4, startY + 20, (int)((panelW - 8) * craftPct), 4, MAGENTA);
        }
        std::string topSkillName = "Settler";
        int maxLevel = 0;
        const auto& skillsComp = settler.getSkills();
        const auto& skillsMap = skillsComp.getAllSkills();
        for (const auto& pair : skillsMap) {
            if (pair.second.level > maxLevel) {
                maxLevel = pair.second.level;
                topSkillName = pair.second.name;
            }
        }
        
        std::string displayText = topSkillName;
        if (maxLevel > 1) {
            displayText += " (Lvl " + std::to_string(maxLevel) + ")";
        }
        int textW = MeasureText(displayText.c_str(), 10);
        DrawText(displayText.c_str(), startX + (panelW - textW) / 2, startY + 22, 10, WHITE);
    }

}


}
void UISystem::DrawSettlerStatsOverlay(const std::vector<Settler*>& settlers, Camera3D camera, int screenWidth, int screenHeight) {

(void)screenWidth;

(void)screenHeight;

for (const auto& settler : settlers) {

if (settler) {

DrawSettlerOverheadUI(*settler, camera);

}

}

}
void UISystem::DrawSettlersStatsPanel(const std::vector<Settler*>& settlers, int screenWidth, int screenHeight) {

if (settlers.empty()) return;

int panelWidth = 200;

int panelHeight = 30 + static_cast<int>(settlers.size()) * 25;

int x = 10;

int y = 50;

(void)screenWidth;

(void)screenHeight;
DrawRectangle(x, y, panelWidth, panelHeight, Color{ 20, 20, 20, 200 });

DrawRectangleLines(x, y, panelWidth, panelHeight, GRAY);

DrawText("Settlers:", x + 10, y + 5, 12, WHITE);
int rowY = y + 25;

for (const auto& settler : settlers) {

    if (settler) {

        std::string name = settler->getName();

        if (name.empty()) name = "Settler";

        std::string state = settler->GetStateString();

        std::string text = name + ": " + state;

        Color textColor = settler->isSelected() ? GREEN : WHITE;

        DrawText(text.c_str(), x + 10, rowY, 10, textColor);

        rowY += 20;

    }

}


}
void UISystem::ShowBuildingInfo(BuildingInstance* building, int screenWidth) {
if (!building) return;

int width = 200;

int height = 120;

int x = screenWidth - width - 10;

int y = 50;
DrawRectangle(x, y, width, height, Color{ 30, 30, 50, 230 });

DrawRectangleLines(x, y, width, height, GOLD);
std::string title = building->getBlueprintId();

DrawText(title.c_str(), x + 10, y + 10, 14, GOLD);
std::string owner = "Owner: " + building->getOwner();

if (building->getOwner().empty()) owner = "Owner: None";

DrawText(owner.c_str(), x + 10, y + 30, 10, WHITE);


}
void UISystem::DrawCraftingPanel(int screenWidth, int screenHeight) {
    // Draw Bottom Panel Base
    int panelHeight = 80;
    int panelY = screenHeight - panelHeight - 10;
    DrawRectangle(10, panelY, screenWidth - 20, panelHeight, Color{ 30, 30, 50, 200 });

int width = 300;

int height = 400;

int x = static_cast<int>((screenWidth - width) / 2.0f);

int y = static_cast<int>((screenHeight - height) / 2.0f);

DrawRectangle(x, y, width, height, Color{ 30, 30, 40, 240 });
DrawRectangleLines(x, y, width, height, MAGENTA);

DrawText("CRAFTING STATION", x + 10, y + 10, 20, WHITE);

auto* craftingSys = GameEngine::getInstance().getSystem<CraftingSystem>();
if (!craftingSys) return;

const auto& recipes = craftingSys->getRecipes();

int startY = y + 50;
int itemHeight = 60;
int itemGap = 5;

for (const auto& recipe : recipes) {
    if (startY + itemHeight > y + height) break;

    DrawRectangle(x + 10, startY, width - 20, itemHeight, Color{ 50, 50, 60, 255 });
    DrawRectangleLines(x + 10, startY, width - 20, itemHeight, GRAY);

    DrawText(recipe.name.c_str(), x + 20, startY + 5, 10, WHITE);

    std::string costStr = "Cost: ";
    for (const auto& ing : recipe.ingredients) {
        costStr += ing.resourceType + " x" + std::to_string(ing.amount) + " ";
    }
    DrawText(costStr.c_str(), x + 20, startY + 20, 10, LIGHTGRAY);

    // Craft Button
    DrawRectangle(x + width - 70, startY + 15, 50, 30, GREEN);
    DrawText("CRAFT", x + width - 65, startY + 25, 10, BLACK);

    startY += itemHeight + itemGap;
}

// Close button
DrawRectangle(x + width - 25, y + 5, 20, 20, RED);
DrawText("X", x + width - 20, y + 8, 15, WHITE);


}
bool UISystem::HandleCraftingPanelClick(int screenWidth, int screenHeight) {

if (!m_isCraftingPanelVisible) return false;

int width = 300;

int height = 400;

int x = (screenWidth - width) / 2;

int y = (screenHeight - height) / 2;
Vector2 mouse = GetMousePosition();
if (mouse.x < x || mouse.x > x + width || mouse.y < y || mouse.y > y + height) {

    return false;

}
if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

    // Close button

    if (mouse.x >= x + width - 25 && mouse.x <= x + width - 5 &&
        mouse.y >= y + 5 && mouse.y <= y + 25) {

        m_isCraftingPanelVisible = false;

        return true;

    }

    auto* craftingSys = GameEngine::getInstance().getSystem<CraftingSystem>();
    if (!craftingSys) return true;

    const auto& recipes = craftingSys->getRecipes();
    int startY = y + 50;
    int itemHeight = 60;

    for (const auto& recipe : recipes) {
        if (startY + itemHeight > y + height) break;
        
        // Craft Button Check
        int btnX = x + width - 70;
        int btnY = startY + 15;
        if (mouse.x >= btnX && mouse.x <= btnX + 50 &&
            mouse.y >= btnY && mouse.y <= btnY + 30) {
            
            
                     std::cout << "UISystem: Crafting button clicked for recipe '" << recipe.id << "'" << std::endl;
                     std::string targetSettlerId = "";
                     if (m_colony) {
                         const auto& selected = m_colony->getSelectedSettlers();
                         if (!selected.empty()) {
                             targetSettlerId = selected[0]->getName();
                         }
                     }
                     craftingSys->queueTask(recipe.id, targetSettlerId);
                     // Minimal feedback: log or std::cout as requested
                     std::cout << "UISystem: Added task for '" << recipe.id << "' to queue";
                     if (!targetSettlerId.empty()) {
                         std::cout << " (Target: " << targetSettlerId << ")";
                     }
                     std::cout << "." << std::endl;
                     return true;
        }
        startY += itemHeight + 5;
    }

}

    return true;
}

// ==========================================
// PREMIUM UI IMPLEMENTATION
// ==========================================

void UISystem::DrawPremiumPanel(Rectangle rec, const char* title, float opacity) {
    // 1. OUTER GLOW (Art Director Standard)
    for (int i = 1; i <= 3; i++) {
        DrawRectangleRoundedLinesEx({rec.x - i, rec.y - i, rec.width + i*2, rec.height + i*2}, 
                                    0.05f, 5, 1.0f, Fade(COLOR_NEON_TEAL, 0.15f / i));
    }

    // 2. BASE GLASS
    Color bgColor = COLOR_GLASS_BG;
    bgColor.a = (unsigned char)(opacity * 255);
    DrawRectangleRounded(rec, 0.05f, 5, bgColor);
    
    // 3. TECHNICAL NOISE (Grid)
    for (float x = rec.x; x < rec.x + rec.width; x += 40) {
        DrawLineEx({x, rec.y}, {x, rec.y + rec.height}, 1.0f, Fade(COLOR_NEON_TEAL, 0.05f));
    }
    for (float y = rec.y; y < rec.y + rec.height; y += 40) {
        DrawLineEx({rec.x, y}, {rec.x + rec.width, y}, 1.0f, Fade(COLOR_NEON_TEAL, 0.05f));
    }

    // Border
    DrawRectangleRoundedLinesEx(rec, 0.05f, 5, 2.0f, COLOR_NEON_TEAL);

    if (title != nullptr) {
        // Technical Header Bracket
        DrawRectangle((int)rec.x + 10, (int)rec.y + 10, 4, 30, COLOR_NEON_TEAL);
        
        DrawText(title, (int)rec.x + 25, (int)rec.y + 15, 22, WHITE);
        DrawText("LOG_ACTIVE", (int)rec.x + rec.width - 80, (int)rec.y + 15, 9, Fade(COLOR_NEON_TEAL, 0.6f));
        
        DrawLineEx({rec.x + 10, rec.y + 45}, {rec.x + rec.width - 10, rec.y + 45}, 1.0f, Fade(COLOR_NEON_TEAL, 0.3f));
    }
}

void UISystem::DrawPremiumResourceBar(int wood, int food, int stone, int population, int screenWidth) {
    (void)screenWidth;
    
    // Art Director: Modular Command Ribbon (Top Left)
    float startX = 20.0f;
    float startY = 15.0f;
    float moduleW = 160.0f;
    float moduleH = 70.0f;
    float gap = 15.0f;

    auto DrawCommandModule = [&](const char* code, int val, Color col, int iconIdx, float x) {
        Rectangle rec = { x, startY, moduleW, moduleH };
        
        // 1. MODULE FRAME (Angular Aggression)
        DrawRectangleRec(rec, { 10, 15, 20, 200 }); // Glass
        DrawRectangleLinesEx(rec, 1.0f, Fade(col, 0.3f));
        // Accent Bar
        DrawRectangle((int)rec.x, (int)rec.y, 4, (int)rec.height, col);
        
        // Technical Header
        DrawText(code, (int)rec.x + 8, (int)rec.y + 6, 9, Fade(col, 0.8f));
        DrawLineEx({rec.x + 8, rec.y + 18}, {rec.x + rec.width - 8, rec.y + 18}, 1.0f, Fade(col, 0.2f));

        // Icon
        if (m_isIconsLoaded) {
            float iSize = static_cast<float>(m_iconsTexture.width) / 4.0f;
            Rectangle src = { static_cast<float>(iconIdx % 4) * iSize, static_cast<float>(iconIdx / 4) * iSize, iSize, iSize };
            DrawTexturePro(m_iconsTexture, src, { rec.x + 8, rec.y + 20, 42, 42 }, {0,0}, 0, WHITE);
            DrawTexturePro(m_iconsTexture, src, { rec.x + 8, rec.y + 20, 42, 42 }, {0,0}, 0, Fade(col, 0.5f));
        }

        // Numerical Data
        DrawText(TextFormat("%d", val), (int)rec.x + 60, (int)rec.y + 22, 32, WHITE);
        // Micro-Status
        DrawCircle((int)rec.x + moduleW - 15, (int)rec.y + 10, 3, (sinf(m_uiAnimationTime * 5.0f + x) > 0) ? col : Fade(col, 0.2f));
    };

    DrawCommandModule("MAT_WOOD", wood, COLOR_NEON_TEAL, 0, startX);
    DrawCommandModule("MAT_FOOD", food, COLOR_NEON_GOLD, 1, startX + moduleW + gap);
    DrawCommandModule("MAT_STONE", stone, GRAY, 2, startX + (moduleW + gap) * 2);
    
    // Population Module (Different shape)
    float popX = startX + (moduleW + gap) * 3 + gap * 2;
    Rectangle popRec = { popX, startY, moduleW + 40, moduleH };
    DrawRectangleRec(popRec, { 20, 25, 35, 230 });
    DrawRectangleLinesEx(popRec, 2.0f, COLOR_NEON_ORANGE);
    DrawText("COLONY_POPULATION", (int)popRec.x + 10, (int)popRec.y + 6, 9, COLOR_NEON_ORANGE);
    
    DrawText(TextFormat("%d", population), (int)popRec.x + 60, (int)popRec.y + 22, 42, WHITE);
    if (m_isIconsLoaded) {
        float iSize = (float)m_iconsTexture.width / 4.0f;
        Rectangle src = { 0, 2 * iSize, iSize, iSize }; // Population icon index 8 (col 0, row 2)
        DrawTexturePro(m_iconsTexture, src, { popRec.x + 10, popRec.y + 25, 40, 40 }, {0,0}, 0, COLOR_NEON_ORANGE);
    }
}

void UISystem::DrawSettlerSelectedPanel(Settler* settler, int screenWidth, int screenHeight) {
    (void)screenHeight;
    if (!settler) return;
    
    // 1. RE-DESIGNED PREMIUM PANEL
    Rectangle rec = { (float)screenWidth - 300, 100, 280, 500 }; 
    DrawPremiumPanel(rec, settler->getName().c_str(), 0.92f);
    
    // 1.1 Close Button (X)
    Vector2 mouse = GetMousePosition();
    Rectangle closeBtn = { rec.x + rec.width - 35, rec.y + 15, 20, 20 };
    bool isHover = CheckCollisionPointRec(mouse, closeBtn);
    DrawText("X", (int)closeBtn.x + 5, (int)closeBtn.y + 2, 16, isHover ? RED : COLOR_NEON_TEAL);
    if (isHover) DrawRectangleLinesEx(closeBtn, 1.0f, RED);
    
    // 2. PORTRAIT & BIO-INFO
    Rectangle portRec = { rec.x + 25, rec.y + 65, 80, 80 }; // Smaller portrait to fit tabs
    DrawRectangleRec(portRec, { 10, 15, 20, 255 });
    // Scanning Line (Portrait)
    float scanPos = portRec.y + (fmodf(m_uiAnimationTime * 45.0f, portRec.height));
    DrawLineEx({portRec.x, scanPos}, {portRec.x + portRec.width, scanPos}, 2.0f, Fade(COLOR_NEON_TEAL, 0.8f));
    DrawRectangleGradientV((int)portRec.x, (int)scanPos - 20, (int)portRec.width, 20, Fade(COLOR_NEON_TEAL, 0.0f), Fade(COLOR_NEON_TEAL, 0.3f));
    DrawLineEx({portRec.x - 5, portRec.y - 5}, {portRec.x + 15, portRec.y - 5}, 2.0f, COLOR_NEON_TEAL); // Bracket
    
    float infoX = portRec.x + portRec.width + 15;
    DrawText("BIO_SIGNATURE_CONFIRMED", (int)infoX, (int)portRec.y, 9, COLOR_NEON_TEAL);
    DrawText(TextFormat("ID: 0x%X", (unsigned int)((size_t)settler % 0xFFFF)), (int)infoX, (int)portRec.y + 15, 9, GRAY);
    DrawText("RANK: OPERATIVE_V1", (int)infoX, (int)portRec.y + 30, 11, COLOR_NEON_GOLD);

    // 3. TABS SYSTEM (Command Deck)
    const char* tabs[] = { "STATUS", "NEURO", "GEAR", "DUTY" };
    int tabCount = 4;
    int tabW = ((int)rec.width - 20) / tabCount;
    int tabH = 25;
    int tabY = (int)portRec.y + (int)portRec.height + 15;
    int startX = (int)rec.x;

    for (int i = 0; i < tabCount; ++i) {
        Rectangle tabRec = { (float)(startX + 10 + i * tabW), (float)tabY, (float)tabW - 2, (float)tabH };
        bool isActive = (m_activeSelectionTab == i);
        
        // Tab Style
        DrawRectangleRec(tabRec, isActive ? Fade(COLOR_NEON_TEAL, 0.3f) : Fade(BLACK, 0.5f));
        DrawRectangleLinesEx(tabRec, 1.0f, isActive ? COLOR_NEON_TEAL : DARKGRAY);
        
        // Tab Text
        int textW = MeasureText(tabs[i], 10);
        DrawText(tabs[i], (int)(tabRec.x + (tabRec.width - textW)/2), (int)(tabRec.y + 7), 10, isActive ? WHITE : GRAY);
        
        if (isActive) {
            DrawLineEx({tabRec.x, tabRec.y + tabH}, {tabRec.x + tabRec.width, tabRec.y + tabH}, 2.0f, COLOR_NEON_TEAL);
        }
    }
    
    // 4. CONTENT AREA
    int contentY = tabY + tabH + 10;
    
    if (m_activeSelectionTab == 0) { // STATUS
         int rowY = contentY + 5;
         StatsComponent& stats = settler->getStats();
         
         auto DrawStatBar = [&](const char* label, float val, Color col) {
             DrawText(label, startX + 15, rowY, 10, Fade(col, 0.8f));
             Rectangle bar = { (float)startX + 100, (float)rowY, 150.0f, 10.0f };
             DrawRectangleRec(bar, { 20, 30, 40, 255 });
             DrawRectangleRec(bar, col);
             DrawRectangle((int)bar.x + 2, (int)bar.y + 2, (int)((bar.width - 4) * val), (int)bar.height - 4, col);
             DrawText(TextFormat("%d%%", (int)(val * 100)), (int)bar.x + 160, (int)bar.y, 10, WHITE);
             rowY += 25;
         };
         
         float hp = (stats.getMaxHealth() > 0) ? stats.getCurrentHealth() / stats.getMaxHealth() : 0.0f;
         DrawStatBar("BIO_INTEGRITY", hp, hp > 0.3f ? COLOR_NEON_TEAL : RED);
         
         float energyVal = (stats.getMaxEnergy() > 0) ? stats.getCurrentEnergy() / stats.getMaxEnergy() : 0.0f;
         DrawStatBar("ENERGY_CORE", energyVal, COLOR_NEON_GOLD);
         
         float food = (stats.getMaxHunger() > 0) ? stats.getCurrentHunger() / stats.getMaxHunger() : 0.0f;
         DrawStatBar("NUTRITION_LVL", food, food > 0.2f ? COLOR_NEON_ORANGE : RED);
         
         DrawText("CURRENT_PROTOCOL:", startX + 15, rowY, 10, GRAY);
         DrawText(settler->GetStateString().c_str(), startX + 15, rowY + 12, 12, WHITE);
    }
    else if (m_activeSelectionTab == 1) { // NEURO (Skills)
        int rowY = contentY + 5;
        const auto& skills = settler->getSkills().getAllSkills();
        for(const auto& pair : skills) {
             const auto& skill = pair.second;
             DrawText(TextFormat("%s [LVL %d]", skill.name.c_str(), skill.level), startX + 15, rowY, 10, WHITE);
             
             // Priority UI
             DrawText("<", startX + 135, rowY, 10, COLOR_NEON_TEAL);
             DrawText(TextFormat("%d", skill.priority), startX + 150, rowY, 10, WHITE);
             DrawText(">", startX + 165, rowY, 10, COLOR_NEON_TEAL);
             
             // XP
             float xp = skill.currentXP / 100.0f;
             DrawRectangle(startX + 15, rowY + 14, 160, 4, DARKGRAY);
             DrawRectangle(startX + 15, rowY + 14, (int)(160 * xp), 4, COLOR_NEON_TEAL);
             
             rowY += 25;
             if (rowY > (int)rec.y + (int)rec.height - 20) break;
        }
    }
    else if (m_activeSelectionTab == 2) { // GEAR (Inventory)
        int rowY = contentY + 5;
        DrawText("EQUIPMENT_MATRIX", startX + 15, rowY, 10, COLOR_NEON_GOLD);
        
        const Item* held = settler->getHeldItem();
        Rectangle mainHandRec = { (float)startX + 15, (float)rowY + 20, 40, 40 };
        DrawRectangleRec(mainHandRec, { 20, 20, 25, 200 });
        DrawRectangleLinesEx(mainHandRec, 1.0f, COLOR_NEON_TEAL);
        
        if (held) {
             if (m_isIconsLoaded) {
                 // Draw icon (simplified placeholder)
             }
             DrawText(held->getDisplayName().substr(0,1).c_str(), (int)mainHandRec.x + 12, (int)mainHandRec.y + 8, 20, WHITE);
        } else {
             DrawText("EMPTY", (int)mainHandRec.x + 5, (int)mainHandRec.y + 15, 8, GRAY);
        }
        DrawText("MAIN_HAND", (int)mainHandRec.x + 50, (int)mainHandRec.y + 15, 10, GRAY);
        
        int slotsY = (int)mainHandRec.y + 60;
        DrawText("STORAGE_MODULE", startX + 15, slotsY - 15, 10, COLOR_NEON_GOLD);
        auto* inv = &settler->getInventory();
        if (inv) {
            for(int i=0; i<10; ++i) {
                int col = i % 5;
                int row = i / 5;
                Rectangle slot = { (float)(startX + 15 + col * 35), (float)(slotsY + row * 35), 30, 30 };
                DrawRectangleRec(slot, { 30, 30, 35, 200 });
                DrawRectangleLinesEx(slot, 1.0f, GRAY);
                
                InventoryItem* item = inv->getItemAt(i);
                if (item && item->item) {
                    DrawText(item->item->getDisplayName().substr(0,1).c_str(), (int)slot.x + 8, (int)slot.y + 5, 10, WHITE);
                    if (item->quantity > 1) DrawText(TextFormat("%d", item->quantity), (int)slot.x + 18, (int)slot.y + 18, 8, WHITE);
                }
            }
        }
    }
    else if (m_activeSelectionTab == 3) { // DUTY (Jobs)
        int rowY = contentY + 5;
        struct JobCheck { const char* label; bool val; };
        JobCheck jobs[] = {
            {"GATHER_FOOD", settler->gatherFood},
            {"GATHER_STONE", settler->gatherStone},
            {"GATHER_WOOD", settler->gatherWood},
            {"CONSTRUCT", settler->performBuilding},
            {"HUNT_HOSTILE", settler->huntAnimals},
            {"LOGISTICS", settler->haulToStorage}
        };
        for(const auto& j : jobs) {
             DrawText(j.label, startX + 15, rowY + 2, 10, WHITE);
             Rectangle toggle = { (float)startX + 180, (float)rowY, 40, 16 };
             DrawRectangleRounded(toggle, 0.5f, 4, j.val ? COLOR_NEON_TEAL : DARKGRAY);
             DrawCircle((int)(j.val ? toggle.x + 32 : toggle.x + 8), (int)(toggle.y + 8), 6.0f, WHITE);
             rowY += 22;
        }
    }
    
    // Footer
    DrawText("SYSTEM_FEED: ONLINE // ENCRYPTED", (int)rec.x + 25, (int)rec.y + (int)rec.height - 25, 9, Fade(COLOR_NEON_TEAL, 0.4f));
    DrawRectangle((int)rec.x + rec.width - 40, (int)rec.y + rec.height - 25, 15, 15, (sinf(m_uiAnimationTime * 8.0f) > 0) ? COLOR_NEON_TEAL : Fade(COLOR_NEON_TEAL, 0.2f));
}

void UISystem::DrawReactiveLogConsole(int screenWidth, int screenHeight) {
    (void)screenWidth;
    float width = 450;
    float height = 200;
    Rectangle logRec = { 30, screenHeight - height - 100, width, height };
    
    // Background with faint glow
    DrawRectangleRounded({logRec.x - 2, logRec.y - 2, logRec.width + 4, logRec.height + 4}, 0.05f, 10, Fade(COLOR_NEON_TEAL, 0.1f));
    DrawPremiumPanel(logRec, "MISSION LOG", 1.0f);
    
    // Close Button (X) for Log
    Vector2 mouse = GetMousePosition();
    Rectangle closeBtn = { logRec.x + logRec.width - 35, logRec.y + 15, 20, 20 };
    bool isHover = CheckCollisionPointRec(mouse, closeBtn);
    DrawText("X", (int)closeBtn.x + 5, (int)closeBtn.y + 2, 16, isHover ? RED : COLOR_NEON_TEAL);
    if (isHover) DrawRectangleLinesEx(closeBtn, 1.0f, RED);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && isHover) {
        m_showMissionLog = false;
    }
    
    auto logs = Logger::getRecentLogs();
    
    // Technical Noise: Fake Boot Sequence if log is empty
    if (logs.empty()) {
        DrawText("> SYSTEM_RESET: NOMINAL", (int)logRec.x + 15, (int)logRec.y + 50, 11, Fade(COLOR_NEON_TEAL, 0.4f));
        DrawText("> NETWORK_LINK: ESTABLISHED", (int)logRec.x + 15, (int)logRec.y + 65, 11, Fade(COLOR_NEON_TEAL, 0.3f));
        DrawText("> ENCRYPTION_V4: ACTIVE", (int)logRec.x + 15, (int)logRec.y + 80, 11, Fade(COLOR_NEON_TEAL, 0.2f));
        DrawText("> WAITING_FOR_MISSION_DATA...", (int)logRec.x + 15, (int)logRec.y + 95, 11, Fade(COLOR_NEON_GOLD, 0.5f));
    }

    int startY = (int)logRec.y + 50;
    int displayCount = 7;
    int startIndex = (logs.size() > (size_t)displayCount) ? (int)logs.size() - displayCount : 0;
    
    for (int i = startIndex; i < (int)logs.size(); ++i) {
        Color baseColor = WHITE;
        const char* prefix = "[INFO]";
        
        switch (logs[i].level) {
            case LogLevel::Info: 
                baseColor = COLOR_NEON_TEAL; 
                prefix = "[SYSTEM]";
                break;
            case LogLevel::Warning: 
                baseColor = COLOR_NEON_GOLD; 
                prefix = "[WARN]  ";
                break;
            case LogLevel::Error: 
                baseColor = COLOR_NEON_ORANGE; 
                prefix = "[CRIT]  ";
                break;
            default: break;
        }
        
        // Age-based fading
        float agePerc = static_cast<float>(i - startIndex + 1) / static_cast<float>(displayCount + 1);
        Color logColor = Fade(baseColor, agePerc);
        
        // Draw prefix with different color for depth
        DrawText(prefix, (int)logRec.x + 15, startY, 11, Fade(baseColor, agePerc * 0.5f));
        DrawText(logs[i].message.c_str(), (int)logRec.x + 85, startY, 13, logColor);
        
        startY += 20;
    }
}

Color UISystem::GetThemeColor(const std::string& name, float alpha) {
    Color c = WHITE;
    if (name == "teal") c = COLOR_NEON_TEAL;
    else if (name == "gold") c = COLOR_NEON_GOLD;
    else if (name == "orange") c = COLOR_NEON_ORANGE;
    else if (name == "bg") c = COLOR_GLASS_BG;
    
    c.a = (unsigned char)(255 * alpha);
    return c;
}

void UISystem::DrawColonyStatsPanel(int screenWidth) {
    int screenHeight = GetScreenHeight();
    (void)screenWidth; // Reserved for layout
    float width = 600;
    float height = 550;
    // Position on LEFT side to avoid collision with Settler Panel (right)
    float xPos = 50; // Left margin
    float yPos = (screenHeight - height) / 2.0f; // Vertical center
    Rectangle rec = { xPos, yPos, width, height };
    
    
    // Use unified DrawPremiumPanel for consistency
    DrawPremiumPanel(rec, "COLONY OVERVIEW", 1.0f);
    
    
    int startY = (int)rec.y + 80;
    int sectionGap = 70;
    
    // === POPULATION SECTION ===
    DrawRectangle((int)rec.x + 15, startY - 5, (int)width - 30, 60, { 20, 30, 40, 180 });
    DrawText("POPULATION", (int)rec.x + 30, startY + 5, 16, LIGHTGRAY);
    int population = (int)m_colony->getSettlers().size();
    DrawText(TextFormat("%d", population), (int)rec.x + 30, startY + 28, 36, WHITE);
    DrawText("settlers", (int)rec.x + 120, startY + 38, 18, GRAY);
    
    // Population Icon
    Rectangle popIconRec = { (float)rec.x + width - 70.0f, (float)startY + 10.0f, 48.0f, 48.0f };
    DrawRectangleRounded(popIconRec, 0.3f, 8, { 30, 40, 50, 200 });
    DrawRectangleRoundedLinesEx(popIconRec, 0.3f, 8, 2.0f, COLOR_NEON_TEAL);
    if (m_isIconsLoaded) {
        float texW = static_cast<float>(m_iconsTexture.width); 
        float texH = static_cast<float>(m_iconsTexture.height);
        float iW = texW / 4.0f; float iH = texH / 4.0f;
        Rectangle srcRec = { static_cast<float>(8 % 4) * iW, static_cast<float>(8 / 4) * iH, iW, iH };
        DrawTexturePro(m_iconsTexture, srcRec, { popIconRec.x + 8, popIconRec.y + 8, 32, 32 }, { 0, 0 }, 0.0f, WHITE);
    } else {
        DrawText("@", (int)popIconRec.x + 15, (int)popIconRec.y + 10, 24, WHITE);
    }
    
    startY += sectionGap;
    
    // === RESOURCES SECTION ===
    DrawText("RESOURCES", (int)rec.x + 30, startY, 18, COLOR_NEON_GOLD);
    startY += 35;
    
    struct ResStat { const char* name; int amount; Color color; int iconIdx; };
    ResStat stats[] = {
        {"WOOD",  m_colony->getWood(),  COLOR_NEON_TEAL,   0},
        {"STONE", m_colony->getStone(), { 200, 200, 200, 255 }, 2},
        {"FOOD",  m_colony->getFood(),  COLOR_NEON_GOLD,   1}
    };
    
    for (int i = 0; i < 3; ++i) {
        int rowY = startY + (i * 55);
        DrawRectangle((int)rec.x + 25, rowY - 5, (int)width - 50, 50, { 45, 55, 65, 100 });
        
        // Icon from texture
        Rectangle iconRec = { rec.x + 35, (float)rowY + 2, 36, 36 };
        DrawRectangleRounded(iconRec, 0.2f, 5, { 25, 30, 35, 255 });
        if (m_isIconsLoaded) {
            float texW = static_cast<float>(m_iconsTexture.width); 
            float texH = static_cast<float>(m_iconsTexture.height);
            float iW = texW / 4.0f; float iH = texH / 4.0f;
            Rectangle srcRec = { static_cast<float>(stats[i].iconIdx % 4) * iW, static_cast<float>(stats[i].iconIdx / 4) * iH, iW, iH };
            DrawTexturePro(m_iconsTexture, srcRec, { iconRec.x + 4, iconRec.y + 4, 28, 28 }, { 0, 0 }, 0.0f, WHITE);
        }
        
        // Label
        DrawText(stats[i].name, (int)rec.x + 85, rowY + 2, 18, WHITE);
        
        // Amount (BIG and BOLD-looking)
        DrawText(TextFormat("%d", stats[i].amount), (int)rec.x + 200, rowY, 32, stats[i].color);
        
        // Progress Bar (visual representation, max 1000)
        float barW = 200;
        float fillRatio = (float)stats[i].amount / 1000.0f;
        if (fillRatio > 1.0f) fillRatio = 1.0f;
        
        Rectangle barBg = { rec.x + 320, (float)rowY + 10, barW, 20 };
        DrawRectangleRounded(barBg, 0.5f, 10, { 20, 20, 20, 200 });
        
        Rectangle barFill = { rec.x + 320, (float)rowY + 10, barW * fillRatio, 20 };
        DrawRectangleRounded(barFill, 0.5f, 10, stats[i].color);
        DrawRectangleRoundedLinesEx(barBg, 0.5f, 10, 1.5f, stats[i].color);
    }
    
    startY += 160;
    
    // === INFRASTRUCTURE SECTION ===
    DrawText("INFRASTRUCTURE", (int)rec.x + 30, startY, 18, COLOR_NEON_TEAL);
    startY += 35;
    
    BuildingSystem* buildingSys = GameEngine::getInstance().getSystem<BuildingSystem>();
    int buildingCount = buildingSys ? (int)buildingSys->getAllBuildings().size() : 0;
    
    DrawRectangle((int)rec.x + 25, startY - 5, (int)width - 50, 45, { 30, 40, 50, 180 });
    
    // Icon (index 3 for buildings)
    Rectangle bldIconRec = { rec.x + 35, (float)startY + 2, 36, 36 };
    DrawRectangleRounded(bldIconRec, 0.2f, 5, { 25, 30, 35, 255 });
    if (m_isIconsLoaded) {
        float texW = static_cast<float>(m_iconsTexture.width); 
        float texH = static_cast<float>(m_iconsTexture.height);
        Rectangle srcRec = { static_cast<float>(3 % 4) * (texW / 4.0f), static_cast<float>(3 / 4) * (texH / 4.0f), texW / 4.0f, texH / 4.0f };
        DrawTexturePro(m_iconsTexture, srcRec, { bldIconRec.x + 4, bldIconRec.y + 4, 28, 28 }, { 0, 0 }, 0.0f, WHITE);
    } else {
        DrawText("B", (int)bldIconRec.x + 10, (int)bldIconRec.y + 5, 20, COLOR_NEON_GOLD);
    }
    
    DrawText("BUILDINGS", (int)rec.x + 85, startY + 2, 18, WHITE);
    DrawText(TextFormat("%d", buildingCount), (int)rec.x + 230, startY, 32, COLOR_NEON_GOLD);
}
