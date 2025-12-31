#pragma once
#include "../core/IGameSystem.h"
#include "../events/InteractionEvents.h"
#include "../game/Item.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <deque>

//----------------------------------------------------------
//
// DATA STRUCTURES
//
//----------------------------------------------------------
struct CraftingIngredient {
std::string resourceType;
int amount;
};
struct CraftingRecipe {
std::string id;
std::string name;
std::string description;
float        craftingTime{1.0f};           // sekundy
std::vector<CraftingIngredient> ingredients;
// wynik
std::string resultItemId;
int         resultAmount{1};
ItemType    resultType{ItemType::MISC};
// opcjonalnie wymagana stacja robocza
std::string requiredStation;
};
// Zadanie craftingu w kolejce
struct CraftingTask {
int         taskId;
std::string recipeId;
float       progress{0.f};
std::string assignedSettlerId; // ten, który wykonuje (bo wziął zadanie)
std::string targetSettlerId;   // ten, dla którego zadanie jest przeznaczone (opcjonalne)
bool        isStarted{false};
bool        materialsRequested{false}; // Flaga: czy już zlecono pozyskanie materiałów
CraftingTask(int id, const std::string& recId)
    : taskId(id), recipeId(recId), materialsRequested(false) {}
};
// Stan oczekującego craftu (pending) per osadnik+przepis
struct PendingCraft {
std::string settlerId;
std::string recipeId;
std::vector<std::pair<std::string, int>> missingResources; // typ i ilość brakująca
bool gatherTaskIssued{false};
};
//----------------------------------------------------------
//
// CRAFTING SYSTEM
//
//----------------------------------------------------------
class CraftingSystem : public IGameSystem {
public:
CraftingSystem() = default;
~CraftingSystem() override = default;
/* IGameSystem */
void initialize() override;
void update(float deltaTime) override;
void render() override;
void shutdown() override;
std::string getName() const override { return "CraftingSystem"; }
int         getPriority() const override { return 4; }
/* API */
void registerRecipe(const CraftingRecipe& recipe);
int  queueTask(const std::string& recipeId, const std::string& targetSettlerId = "");
// Zwraca wskaźnik na zadanie (w kolejce lub aktywne) lub nullptr
CraftingTask* getAvailableTask(const std::string& settlerId);
std::unique_ptr<Item> completeTask(int taskId);
void cancelTask(int taskId);
bool canCraft(const std::string& recipeId, const std::string& settlerId = "", bool silent = false);
bool consumeIngredients(const std::string& recipeId, const std::string& settlerId = "");
const std::vector<CraftingRecipe>& getRecipes() const { return m_recipeList; }
const std::deque<CraftingTask>&    getQueue()  const { return m_taskQueue; }
private:
/* implementation helpers */
std::unique_ptr<Item> createItemFromRecipe(const CraftingRecipe& recipe);
// pending crafts management
void addPendingCraft(const std::string& settlerId, const std::string& recipeId, const std::vector<std::pair<std::string, int>>& missing);
void removePendingCraft(const std::string& settlerId, const std::string& recipeId);
PendingCraft* getPendingCraft(const std::string& settlerId, const std::string& recipeId);
void onInventoryChanged(const InventoryChangedEvent& event);
void checkAndResumeCraft(const std::string& settlerId);
/* data */
std::vector<CraftingRecipe>                   m_recipeList;
std::unordered_map<std::string, CraftingRecipe> m_recipesById;
std::deque<CraftingTask>  m_taskQueue;
std::vector<CraftingTask> m_activeTasks;
int m_nextTaskId{1};
std::unordered_map<std::string, PendingCraft> m_pendingCrafts; // key: settlerId + "|" + recipeId
float m_resumeCheckTimer{0.f};
float m_resumeCheckInterval{0.5f}; // sekundy

};
