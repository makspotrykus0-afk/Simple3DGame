
#include "raylib.h"

#include "raymath.h"

#include "UISystem.h"

#include "../core/GameEngine.h"

#include "../systems/InteractionSystem.h"

#include "../systems/InventorySystem.h"

#include "../game/Colony.h"

#include "../systems/StorageSystem.h"

#include "../systems/ResourceTypes.h"

#include "../components/StatsComponent.h"

#include "../components/EnergyComponent.h"

#include "../components/SkillsComponent.h"

#include "../systems/CraftingSystem.h"
#include <iostream>

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

float cellWidth = containerSize.x / cols;

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

m_nextUIElementId(1),

m_cacheHits(0),

m_cacheMisses(0),

m_totalRenderTime(0),

m_interactionCount(0),

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

std::cout << "UISystem initialized." << std::endl;

}
void UISystem::update(float deltaTime) {

updateUIAnimations(deltaTime);

m_cache.cleanup();

}
void UISystem::render() {

for (const auto& playerPair : m_playerUIElements) {

renderPlayerUI(playerPair.first);

}

// Draw immediate mode UI (e.g. Crafting Panel) if visible

if (m_isCraftingPanelVisible) {

DrawCraftingPanel(GetScreenWidth(), GetScreenHeight());

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
// ==========================================

// IMMEDIATE MODE GUI HELPERS (IMPLEMENTATION)

// ==========================================
void UISystem::DrawResourceBar(int wood, int food, int stone, int screenWidth) {

int height = 40;

DrawRectangle(0, 0, screenWidth, height, Color{ 30, 30, 30, 230 });

DrawRectangleLines(0, 0, screenWidth, height, Color{ 100, 100, 100, 255 });

int startX = 20;

int gap = 150;

int fontSize = 20;
DrawText("Wood:", startX, 10, fontSize, BROWN);

DrawText(std::to_string(wood).c_str(), startX + 60, 10, fontSize, WHITE);
DrawText("Food:", startX + gap, 10, fontSize, ORANGE);

DrawText(std::to_string(food).c_str(), startX + gap + 60, 10, fontSize, WHITE);
int gap2 = gap * 2;

DrawText("Stone:", startX + gap2, 10, fontSize, GRAY);

DrawText(std::to_string(stone).c_str(), startX + gap2 + 70, 10, fontSize, WHITE);


}
void UISystem::DrawBottomPanel(bool isBuildingMode, bool cameraMode, int screenWidth, int screenHeight) {

int height = 60;

int y = screenHeight - height;

DrawRectangle(0, y, screenWidth, height, Color{ 20, 20, 20, 240 });

DrawLine(0, y, screenWidth, y, GRAY);

int startX = 20;

int btnWidth = 150;

int btnHeight = 40;

int btnY = y + 10;

int gap = 20;
Color buildColor = isBuildingMode ? GREEN : LIGHTGRAY;

DrawRectangleLines(startX, btnY, btnWidth, btnHeight, buildColor);

DrawText(isBuildingMode ? "BUILD ACTIVE [B]" : "BUILD MODE [B]", startX + 10, btnY + 10, 10, buildColor);
startX += btnWidth + gap;

Color camColor = cameraMode ? YELLOW : LIGHTGRAY;

DrawRectangleLines(startX, btnY, btnWidth, btnHeight, camColor);

DrawText(cameraMode ? "CAM: SETTLER [C]" : "CAM: FREE [C]", startX + 10, btnY + 10, 10, camColor);
// CRAFTING BUTTON

startX += btnWidth + gap;

Color craftColor = m_isCraftingPanelVisible ? MAGENTA : LIGHTGRAY;

DrawRectangleLines(startX, btnY, btnWidth, btnHeight, craftColor);

DrawText("CRAFTING [K]", startX + 30, btnY + 10, 10, craftColor);
// Check click for Crafting Button

Vector2 mouse = GetMousePosition();

if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

    if (mouse.x >= startX && mouse.x <= startX + btnWidth &&
        mouse.y >= btnY && mouse.y <= btnY + btnHeight) {

        m_isCraftingPanelVisible = !m_isCraftingPanelVisible;

    }

}
startX += btnWidth + gap;

DrawText("LMB: Select/Action  RMB: Move/Building Info  WASD: Move Cam  TAB: Stats", startX + 20, btnY + 10, 10, LIGHTGRAY);


}
void UISystem::DrawBuildingSelectionPanel(const std::vector<BuildingBlueprint*>& blueprints, const std::string& selectedId, int screenWidth, int screenHeight) {

int cardWidth = 140;

int cardHeight = 80;

int padding = 10;

int panelHeight = cardHeight + 20;

int y = screenHeight - 60 - panelHeight;

DrawRectangle(0, y, screenWidth, panelHeight, Color{ 40, 40, 40, 200 });

DrawLine(0, y, screenWidth, y, GRAY);
DrawText("Select Blueprint:", 20, y + 5, 10, WHITE);
int totalWidth = static_cast<int>(blueprints.size()) * (cardWidth + padding) + padding;

int startX = (screenWidth - totalWidth) / 2;

int startY = y + 10;
for (size_t i = 0; i < blueprints.size(); ++i) {

    BuildingBlueprint* bp = blueprints[i];

    int x = startX + padding + static_cast<int>(i) * (cardWidth + padding);

    bool isSelected = (bp->getId() == selectedId);

    Color bgColor = isSelected ? Color{ 80, 100, 120, 255 } : Color{ 60, 60, 60, 255 };

    Color borderColor = isSelected ? YELLOW : LIGHTGRAY;

    DrawRectangle(x, startY, cardWidth, cardHeight, bgColor);
    DrawRectangleLines(x, startY, cardWidth, cardHeight, borderColor);

    DrawText(bp->getName().c_str(), x + 5, startY + 5, 10, WHITE);
    int resY = startY + 25;
    for(const auto& req : bp->getRequirements()) {
        std::string cost = req.resourceType + ": " + std::to_string(req.amount);
        DrawText(cost.c_str(), x + 5, resY, 10, LIGHTGRAY);
        resY += 12;
    }
    Color iconColor = BROWN;
    if (bp->getId() == "floor") iconColor = DARKBROWN;
    DrawRectangle(x + cardWidth - 30, startY + cardHeight - 30, 25, 25, iconColor);
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

if (!m_colony) return false;

int width = 250;

int height = 280;

int x = screenWidth - width - 10;

int y = screenHeight - height - 70;
Vector2 mouse = GetMousePosition();
if (mouse.x >= x && mouse.x <= x + width && mouse.y >= y && mouse.y <= y + height) {

    // Check Tabs

    int tabCount = 4;

    int tabWidth = width / tabCount;

    int tabHeight = 25;

    for (int i = 0; i < tabCount; ++i) {
        int tx = x + i * tabWidth;
        if (mouse.x >= tx && mouse.x <= tx + tabWidth && mouse.y >= y && mouse.y <= y + tabHeight) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                m_activeSelectionTab = i;
                m_selectedInventorySlot = -1;
            }
            return true;
        }
    }

    if (m_activeSelectionTab == 1) { // Skills
         const auto& settlers = m_colony->getSelectedSettlers();
         if (settlers.size() == 1) {
             Settler* s = settlers[0];
             int contentY = y + tabHeight + 10;
             int skillY = contentY + 20;
             const auto& skills = s->getSkills().getAllSkills();
             for (const auto& pair : skills) {
                 int btnY = skillY - 1;
                 int btnH = 14;
                 int btnW = 20;
                 if (mouse.x >= x + 170 && mouse.x <= x + 170 + btnW && mouse.y >= btnY && mouse.y <= btnY + btnH) {
                     if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                         int newPrio = pair.second.priority - 1;
                         s->getSkills().setSkillPriority(pair.second.type, newPrio);
                     }
                     return true;
                 }
                 if (mouse.x >= x + 220 && mouse.x <= x + 220 + btnW && mouse.y >= btnY && mouse.y <= btnY + btnH) {
                     if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                         int newPrio = pair.second.priority + 1;
                         s->getSkills().setSkillPriority(pair.second.type, newPrio);
                     }
                     return true;
                 }
                 skillY += 15;
                 if (skillY > y + height - 10) break;
             }
             int autoEquipY = skillY + 5;
             int checkSize = 12;
             int hitPadding = 4;
             int hitWidth = checkSize + 100;
             int checkX = x + 10;
             if (mouse.x >= checkX - hitPadding && mouse.x <= checkX + hitWidth + hitPadding &&
                 mouse.y >= autoEquipY - hitPadding && mouse.y <= autoEquipY + checkSize + hitPadding) {
                 if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                     s->getSkills().autoEquipBestItems = !s->getSkills().autoEquipBestItems;
                 }
                 return true;
             }
         }
    } else if (m_activeSelectionTab == 2) { // Inventory
         if (!m_colony) return true;
         const auto& settlers = m_colony->getSelectedSettlers();
         int contentY = y + tabHeight + 10;
         int mainHandSize = 40;
         if (mouse.x >= x + 80 && mouse.x <= x + 80 + mainHandSize && mouse.y >= contentY + 25 && mouse.y <= contentY + 25 + mainHandSize) {
             if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) return true;
         }
         // Backpack slots
         int slotSize = 30;
         int slotGap = 5;
         int slotsPerRow = 5;
         for(int i = 0; i < 10; ++i) {
             int col = i % slotsPerRow;
             int row = i / slotsPerRow;
             int sx = x + 20 + col * (slotSize + slotGap);
             int sy = contentY + 80 + row * (slotSize + slotGap);
             if (mouse.x >= sx && mouse.x <= sx + slotSize && mouse.y >= sy && mouse.y <= sy + slotSize) {
                 if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) return true;
             }
         }
    } else if (m_activeSelectionTab == 3) { // Jobs
         const auto& settlers = m_colony->getSelectedSettlers();
         if (settlers.size() == 1) {
             Settler* s = settlers[0];
             int contentY = y + tabHeight + 10;
             int checkSize = 14;
             int rowHeight = 22;
             int colWidth = 110;
             struct JobCheckbox { const char* label; bool* value; };
             JobCheckbox jobs[] = {
                 {"Wood", &s->gatherWood},
                 {"Stone", &s->gatherStone},
                 {"Food", &s->gatherFood},
                 {"Build", &s->performBuilding},
                 {"Hunt", &s->huntAnimals},
                 {"Craft", &s->craftItems},
                 {"Haul", &s->haulToStorage},
                 {"Crops", &s->tendCrops}
             };
             for (int i = 0; i < 8; ++i) {
                 int col = i % 2;
                 int row = i / 2;
                 int checkX = x + 15 + col * colWidth;
                 int checkY = contentY + 20 + row * rowHeight;
                 int hitPadding = 4;
                 if (mouse.x >= checkX - hitPadding && mouse.x <= checkX + colWidth - hitPadding &&
                     mouse.y >= checkY - hitPadding && mouse.y <= checkY + checkSize + hitPadding) {
                     if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                         bool newValue = !(*jobs[i].value);
                         if (newValue) {
                             // Allow multiple jobs enabled
                         }
                         *jobs[i].value = newValue;
                         s->OnJobConfigurationChanged();
                     }
                     return true;
                 }
             }
         }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return true;
    }

}
if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {

    m_selectedInventorySlot = -1;

}

return false;


}
bool UISystem::IsMouseOverUI() {

Vector2 mouse = GetMousePosition();

int screenWidth = GetScreenWidth();

int screenHeight = GetScreenHeight();

int bottomHeight = 60;

if (mouse.y >= screenHeight - bottomHeight) return true;

if (m_isBuildingMode) {

    int cardHeight = 80;

    int panelHeight = cardHeight + 20;

    int panelY = screenHeight - bottomHeight - panelHeight;

    if (mouse.y >= panelY && mouse.y < screenHeight - bottomHeight) {

        return true;

    }

}
if (m_colony && !m_colony->getSelectedSettlers().empty()) {

    int width = 250;

    int height = 280;

    int x = screenWidth - width - 10;

    int y = screenHeight - height - 70;

    if (mouse.x >= x && mouse.x <= x + width && mouse.y >= y && mouse.y <= y + height) {

        return true;

    }

}
// Check Crafting Panel

if (m_isCraftingPanelVisible) {

    int width = 300;

    int height = 400;

    int x = (screenWidth - width) / 2;

    int y = (screenHeight - height) / 2;

    if (mouse.x >= x && mouse.x <= x + width && mouse.y >= y && mouse.y <= y + height) {

        return true;

    }

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
void UISystem::ShowBuildingInfo(BuildingInstance* building, int screenWidth, int screenHeight) {

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

int width = 300;

int height = 400;

int x = (screenWidth - width) / 2;

int y = (screenHeight - height) / 2;

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
