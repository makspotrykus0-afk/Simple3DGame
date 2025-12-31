#include "PlayerCharacter.h"
#include "../core/Logger.h" 
#include "../components/StatsComponent.h" 
#include "../components/SkillsComponent.h" 
#include "../components/EquipmentComponent.h" 
#include "../components/InventoryComponent.h" 
#include "../components/PositionComponent.h" 
#include "../components/InteractionComponent.h" 
#include "../components/BuildingComponent.h" 
#include "../core/GameEngine.h" 
#include "../systems/SkillsSystem.h" 
#include "../systems/ResourceSystem.h" 
#include "../systems/SkillTypes.h" 

// --- AttributeLevel Implementation ---
// AttributeLevel::addExperience is defined inline in PlayerCharacter.h

// --- PlayerCharacter::CharacterStats Implementation ---
int PlayerCharacter::CharacterStats::addExperience(float exp, float expMultiplier) {
    experience += exp * expMultiplier;
    int levelsGained = 0;
    while (canLevelUp()) {
        levelsGained++;
        level++;
        experienceToNext *= 1.5f; 
    }
    return levelsGained;
}

bool PlayerCharacter::CharacterStats::levelUp() {
    if (!canLevelUp()) return false;

    int levelsGained = addExperience(experienceToNext); 
    if (levelsGained > 0) {
        experience = 0.0f; 
        return true;
    }
    return false;
}

int PlayerCharacter::CharacterStats::getAttribute(PrimaryAttribute attribute) const {
    auto it = attributes.find(attribute);
    if (it != attributes.end()) {
        return it->second.currentValue;
    }
    return 0; 
}

float PlayerCharacter::CharacterStats::getDerivedStat(DerivedStat stat) const {
    auto it = derivedStats.find(stat);
    if (it != derivedStats.end()) {
        return it->second;
    }
    return 0.0f; 
}

// --- PlayerCharacter::ClassFactory Implementation ---
std::unordered_map<CharacterClass, PlayerCharacter::ClassConfig> PlayerCharacter::ClassFactory::createBaseClasses() {
    std::unordered_map<CharacterClass, ClassConfig> baseClasses;

    ClassConfig survivor(CharacterClass::SURVIVOR, "Survivor", "A versatile character skilled in multiple areas.");
    survivor.startingAttributes[PrimaryAttribute::STRENGTH] = 10;
    survivor.startingAttributes[PrimaryAttribute::DEXTERITY] = 10;
    survivor.startingAttributes[PrimaryAttribute::CONSTITUTION] = 10;
    survivor.startingStats[DerivedStat::HEALTH] = 100.0f;
    survivor.startingStats[DerivedStat::STAMINA] = 100.0f;
    survivor.bonusSkills.push_back(SkillType::SURVIVAL); 
    survivor.bonusSkills.push_back(SkillType::WOODCUTTING); 
    baseClasses.emplace(CharacterClass::SURVIVOR, survivor);

    ClassConfig warrior(CharacterClass::WARRIOR, "Warrior", "Focuses on combat and physical prowess.");
    warrior.startingAttributes[PrimaryAttribute::STRENGTH] = 14;
    warrior.startingAttributes[PrimaryAttribute::CONSTITUTION] = 12;
    warrior.startingStats[DerivedStat::HEALTH] = 120.0f;
    warrior.startingStats[DerivedStat::ATTACK_POWER] = 10.0f;
    warrior.bonusSkills.push_back(SkillType::COMBAT); 
    warrior.bonusSkills.push_back(SkillType::MINING); 
    baseClasses.emplace(CharacterClass::WARRIOR, warrior);

    ClassConfig craftsman(CharacterClass::CRAFTSMAN, "Craftsman", "Excels in crafting and building.");
    craftsman.startingAttributes[PrimaryAttribute::DEXTERITY] = 12;
    craftsman.startingAttributes[PrimaryAttribute::INTELLIGENCE] = 10;
    craftsman.startingStats[DerivedStat::CARRY_WEIGHT] = 150.0f;
    craftsman.bonusSkills.push_back(SkillType::CRAFTING); 
    craftsman.bonusSkills.push_back(SkillType::SMITHING); 
    craftsman.bonusSkills.push_back(SkillType::BUILDING); 
    baseClasses.emplace(CharacterClass::CRAFTSMAN, craftsman);
    
    return baseClasses;
}

// --- PlayerCharacter Implementation ---

PlayerCharacter::PlayerCharacter(const std::string& playerId, CharacterClass characterClass)
    : GameEntity(playerId), 
      m_playerId(playerId),
      m_stats(), 
      m_playerStats(), 
      m_lastPosition({0.0f, 0.0f, 0.0f})
{
    m_stats.characterClass = characterClass;
    auto baseClasses = ClassFactory::createBaseClasses();
    auto configIt = baseClasses.find(characterClass);
    if (configIt != baseClasses.end()) {
        const auto& config = configIt->second;
        for (const auto& pair : config.startingAttributes) {
            m_stats.attributes[pair.first] = AttributeLevel(pair.second);
        }
        for (const auto& pair : config.startingStats) {
            m_stats.derivedStats[pair.first] = pair.second;
        }
        // Assign bonusSkills correctly. CharacterStats now has bonusSkills member.
        m_stats.bonusSkills = config.bonusSkills; 
    } else {
        Logger::log(LogLevel::Warning, "Character class '" + std::to_string(static_cast<int>(characterClass)) + "' not found. Using default Survivor.");
        auto defaultIt = baseClasses.find(CharacterClass::SURVIVOR);
        if(defaultIt != baseClasses.end()){
            const auto& config = defaultIt->second;
            for (const auto& pair : config.startingAttributes) {
                m_stats.attributes[pair.first] = AttributeLevel(pair.second);
            }
            for (const auto& pair : config.startingStats) {
                m_stats.derivedStats[pair.first] = pair.second;
            }
            m_stats.bonusSkills = config.bonusSkills;
        }
    }
    
    m_playerStats.totalExperience = 0.0f;
    m_playerStats.totalLevelUps = 0;
    m_playerStats.timePlayed = 0.0f;
    m_playerStats.deaths = 0;
    m_playerStats.kills = 0;
    m_playerStats.distanceTraveled = 0.0f;
}

PlayerCharacter::~PlayerCharacter() {
}

bool PlayerCharacter::initialize() {
    // Ensure components are added correctly.
    // Check if component exists and add it if it doesn't.
    if (!getComponent<PositionComponent>()) {
        addComponent(std::make_shared<PositionComponent>());
    }
    if (!getComponent<InteractionComponent>()) {
        addComponent(std::make_shared<InteractionComponent>());
    }
    if (!getComponent<InventoryComponent>()) {
        addComponent(std::make_shared<InventoryComponent>(m_playerId, 100.0f));
    }
    if (!getComponent<BuildingComponent>()) {
        addComponent(std::make_shared<BuildingComponent>(m_playerId));
    }
    if (!getComponent<SkillsComponent>()) {
        addComponent(std::make_shared<SkillsComponent>());
    }
    
    // Use getComponent to ensure PositionComponent is available and set its initial position
    // This part is now handled by the addComponent calls above.
    // Removed erroneous 'Ternary'
    return true;
}

void PlayerCharacter::update(float deltaTime) {
    updatePlayerStats(deltaTime);
    GameEntity::update(deltaTime);

    if (auto* interactionComp = getInteractionComponent()) interactionComp->update(deltaTime);
    if (auto* inventoryComp = getInventoryComponent()) inventoryComp->update(deltaTime);
    if (auto* buildingComp = getBuildingComponent()) buildingComp->update(deltaTime);
    // SkillsComponent inherits from IComponent, which has update and render.
    auto skillsComp = getComponent<SkillsComponent>(); // Deduces std::shared_ptr<SkillsComponent>
    if (skillsComp) { // Check if the shared_ptr is valid
        skillsComp->update(deltaTime); // Call inherited update method
    }
}

void PlayerCharacter::render() {
    if (auto* interactionComp = getInteractionComponent()) interactionComp->render();
    if (auto* inventoryComp = getInventoryComponent()) inventoryComp->render();
    if (auto* buildingComp = getBuildingComponent()) buildingComp->render();
    // SkillsComponent inherits from IComponent, which has update and render.
    if (auto skillsComp = getComponent<SkillsComponent>()) { // Deduces std::shared_ptr<SkillsComponent>
        skillsComp->render(); // Call inherited render method
    }
}

bool PlayerCharacter::setCharacterClass(CharacterClass characterClass) {
    auto baseClasses = ClassFactory::createBaseClasses();
    auto configIt = baseClasses.find(characterClass);
    if (configIt != baseClasses.end()) {
        m_stats.characterClass = characterClass;
        Logger::log(LogLevel::Info, "Player " + m_playerId + " changed class to " + configIt->second.name);
        return true;
    }
    Logger::log(LogLevel::Warning, "Failed to change class for player " + m_playerId + ". Class not found.");
    return false;
}

int PlayerCharacter::addExperience(float experience, const std::string& source) {
    int levelsGained = m_stats.addExperience(experience); 
    if (levelsGained > 0) {
        m_playerStats.totalLevelUps += levelsGained;
        Logger::log(LogLevel::Info, "Player " + m_playerId + " gained " + std::to_string(levelsGained) + " level(s). Total levels: " + std::to_string(m_stats.level));
    }
    m_playerStats.totalExperience += experience;
    m_playerStats.experienceBySource[source] += experience;
    return levelsGained;
}

bool PlayerCharacter::canLevelUp() const {
    return m_stats.canLevelUp();
}

bool PlayerCharacter::levelUp() {
    if (m_stats.levelUp()) {
        Logger::log(LogLevel::Info, "Player " + m_playerId + " leveled up to level " + std::to_string(m_stats.level));
        calculateDerivedStats(); 
        return true;
    }
    return false;
}

bool PlayerCharacter::increaseAttribute(PrimaryAttribute attribute, int points) {
    auto it = m_stats.attributes.find(attribute);
    if (it != m_stats.attributes.end()) {
        it->second.currentValue += points;
        calculateDerivedStats(); 
        Logger::log(LogLevel::Info, "Player " + m_playerId + " increased attribute " + std::to_string(static_cast<int>(attribute)) + " by " + std::to_string(points));
        return true;
    }
    Logger::log(LogLevel::Warning, "Player " + m_playerId + " tried to increase non-existent attribute " + std::to_string(static_cast<int>(attribute)));
    return false;
}

Vector3 PlayerCharacter::getPosition() const {
    auto posComp = getComponent<const PositionComponent>(); // Deduces std::shared_ptr<const PositionComponent>
    if (posComp) { // Check if the shared_ptr is valid
        return posComp->getPosition();
    }
    return m_lastPosition;
}

void PlayerCharacter::setPosition(const Vector3& position) {
    auto posComp = getComponent<PositionComponent>(); // Deduces std::shared_ptr<PositionComponent>
    if (posComp) { // Check if the shared_ptr is valid
        posComp->setPosition(position);
    }
    m_lastPosition = position;
}

float PlayerCharacter::getMovementSpeed() const {
    return getDerivedStat(DerivedStat::SPEED);
}

float PlayerCharacter::getMaxCarryWeight() const {
    return getDerivedStat(DerivedStat::CARRY_WEIGHT);
}

bool PlayerCharacter::isAlive() const {
    return getDerivedStat(DerivedStat::HEALTH) > 0.0f;
}

int PlayerCharacter::getAttribute(PrimaryAttribute attribute) const {
    return m_stats.getAttribute(attribute);
}

float PlayerCharacter::getDerivedStat(DerivedStat stat) const {
    return m_stats.getDerivedStat(stat);
}

bool PlayerCharacter::initializeComponents() {
    // Components are added in PlayerCharacter::initialize().
    // Removed erroneous 'Ternary'
    return true; 
}

void PlayerCharacter::calculateDerivedStats() {
    Logger::log(LogLevel::Info, "Recalculating derived stats for player " + m_playerId);
    
    float constitutionBonus = getAttribute(PrimaryAttribute::CONSTITUTION) * 2.0f;
    m_stats.derivedStats[DerivedStat::HEALTH] = 100.0f + constitutionBonus; 

    float strengthBonus = getAttribute(PrimaryAttribute::STRENGTH) * 1.5f;
    float dexterityBonus = getAttribute(PrimaryAttribute::DEXTERITY) * 0.5f;
    m_stats.derivedStats[DerivedStat::ATTACK_POWER] = 5.0f + strengthBonus + dexterityBonus;
}

void PlayerCharacter::applyClassBonuses() {
}

template<typename T>
void PlayerCharacter::notifyCharacterEvent(const T& event) {
}

void PlayerCharacter::updatePlayerStats(float deltaTime) {
    m_playerStats.timePlayed += deltaTime;
    Vector3 currentPosition = getPosition();
    m_playerStats.distanceTraveled += Vector3Distance(m_lastPosition, currentPosition);
    m_lastPosition = currentPosition;
}
