#pragma once

#include "GameEntity.h"
#include "../systems/SkillTypes.h" // Include the new SkillType definition
#include "../systems/ResourceSystem.h"
#include "../systems/FoodSystem.h"
#include "../systems/SkillsSystem.h"
#include "../systems/EquipmentSystem.h"
#include "../components/PositionComponent.h"
#include "../components/InteractionComponent.h"
#include "../components/InventoryComponent.h"
#include "../components/BuildingComponent.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>
#include <any>
#include <algorithm> // Required for std::find_if

/**
 * @brief Typy klas postaci
 */
enum class CharacterClass : uint32_t {
    SURVIVOR = 0,    // Ocalały - wszechstronny
    WARRIOR = 1,     // Wojownik - walka i siła
    CRAFTSMAN = 2,   // Rzemieślnik - wytwarzanie i rozwój
    EXPLORER = 3,    // Odkrywca - mobilność i przetrwanie
    MEDIC = 4,       // Medyk - leczenie i ochrona
    SCHOLAR = 5,     // Uczony - magia i technologia
    CUSTOM_START = 1000
};

/**
 * @brief Atrybuty podstawowe postaci
 */
enum class PrimaryAttribute : uint32_t {
    STRENGTH = 0,      // Siła - wpływa na obrażenia i noszenie
    DEXTERITY = 1,     // Zręczność - wpływa na szybkość i precyzję
    INTELLIGENCE = 2,  // Intelekt - wpływa na magię i technologię
    WISDOM = 3,        // Mądrość - wpływa na przetrwanie i decyzje
    CONSTITUTION = 4,  // Kondycja - wpływa na zdrowie i wytrzymałość
    CHARISMA = 5       // Charyzma - wpływa na społeczne interakcje
};

/**
 * @brief Statystyki pochodne postaci
 */
enum class DerivedStat : uint32_t {
    HEALTH = 0,           // Zdrowie
    MANA = 1,             // Mana/Energia
    STAMINA = 2,          // Wytrzymałość
    ARMOR = 3,            // Pancerz
    ATTACK_POWER = 4,     // Siła ataku
    DEFENSE = 5,          // Obrona
    SPEED = 6,            // Szybkość
    CRIT_CHANCE = 7,      // Szansa na cios krytyczny
    CARRY_WEIGHT = 8,     // Maksymalny ciężar noszenia
    EXP_MULTIPLIER = 9    // Mnożnik doświadczenia
};

/**
 * @brief Struktura opisująca poziom atrybutu
 */
struct AttributeLevel {
    int baseValue;      // Bazowa wartość (na poziomie 1)
    int currentValue;   // Aktualna wartość
    float experience;   // Doświadczenie do następnego poziomu
    
    AttributeLevel(int base = 10) : baseValue(base), currentValue(base), experience(0.0f) {}
    
    /**
     * @brief Dodaje doświadczenie do atrybutu
     * @param exp Ilość doświadczenia
     * @return Liczba zdobytych poziomów
     */
    int addExperience(float exp) {
        experience += exp;
        int levelsGained = 0;
        // Simplified leveling logic
        while (experience >= 100.0f) { 
            experience -= 100.0f;
            currentValue += 5; 
            levelsGained++;
        }
        return levelsGained;
    }
    
    /**
     * @brief Oblicza aktualną wartość atrybutu na podstawie bazy i poziomu
     */
    void recalculateCurrentValue() {
        // Placeholder for actual calculation logic
    }
};

/**
 * @brief Zdarzenie zmiany poziomu postaci
 */
struct LevelUpEvent {
    std::string playerId;
    int oldLevel;
    int newLevel;
    // Use SkillType enum here
    std::vector<SkillType> availableSkills; 
    std::chrono::high_resolution_clock::time_point levelUpTime;
};

/**
 * @brief Zdarzenie zmiany atrybutu
 */
struct AttributeChangedEvent {
    std::string playerId;
    PrimaryAttribute attribute;
    int oldValue;
    int newValue;
    float experienceGained;
};

/**
 * @brief Zdarzenie zmiany statystyki pochodnej
 */
struct DerivedStatChangedEvent {
    std::string playerId;
    DerivedStat stat;
    float oldValue;
    float newValue;
    std::string source; // Źródło zmiany (equipment, skills, etc.)
};

/**
 * @brief Postać gracza - specjalizacja GameEntity
 */
class PlayerCharacter : public GameEntity {
public:
    /**
     * @brief Konfiguracja klasy postaci
     */
    struct ClassConfig {
        CharacterClass classType;
        std::string name;
        std::string description;
        std::unordered_map<PrimaryAttribute, int> startingAttributes;
        std::unordered_map<DerivedStat, float> startingStats;
        // Use SkillType enum here - ensure SkillType is properly defined and included
        std::vector<SkillType> bonusSkills; 
        std::unordered_map<std::string, float> statMultipliers;
        std::string specializationDescription;
        
        ClassConfig(CharacterClass type, const std::string& n, const std::string& desc = "")
            : classType(type), name(n), description(desc) {}
    };

    /**
     * @brief Stan postaci gracza
     */
    struct CharacterStats {
        int level;
        float experience;
        float experienceToNext;
        std::unordered_map<PrimaryAttribute, AttributeLevel> attributes;
        std::unordered_map<DerivedStat, float> derivedStats;
        CharacterClass characterClass;
        std::vector<SkillType> bonusSkills; // Added bonusSkills here
        
        CharacterStats() : level(1), experience(0.0f), experienceToNext(100.0f),
                          characterClass(CharacterClass::SURVIVOR) {}
        
        bool canLevelUp() const { return experience >= experienceToNext; }
        int addExperience(float exp, float expMultiplier = 1.0f);
        bool levelUp();
        int getAttribute(PrimaryAttribute attribute) const;
        float getDerivedStat(DerivedStat stat) const;
    };

    /**
     * @brief Factory dla klas postaci
     */
    class ClassFactory {
    public:
        static std::unordered_map<CharacterClass, ClassConfig> createBaseClasses();
    };

public:
    PlayerCharacter(const std::string& playerId, CharacterClass characterClass = CharacterClass::SURVIVOR);
    virtual ~PlayerCharacter();

    bool initialize();
    void update(float deltaTime) override;
    void render() override;

    const std::string& getPlayerId() const { return m_playerId; }
    const CharacterStats& getStats() const { return m_stats; }

    bool setCharacterClass(CharacterClass characterClass);
    int addExperience(float experience, const std::string& source = "unknown");
    bool canLevelUp() const;
    bool levelUp();
    bool increaseAttribute(PrimaryAttribute attribute, int points = 1);

    Vector3 getPosition() const override;
    void setPosition(const Vector3& position) override;

    float getMovementSpeed() const;
    float getMaxCarryWeight() const;
    bool isAlive() const;

    int getAttribute(PrimaryAttribute attribute) const;
    float getDerivedStat(DerivedStat stat) const;

    struct PlayerStats {
        float totalExperience;
        int totalLevelUps;
        float timePlayed;
        int deaths;
        int kills;
        float distanceTraveled;
        std::unordered_map<std::string, float> experienceBySource;
    };
    const PlayerStats& getPlayerStats() const { return m_playerStats; }

    // === KOMPONENTY INTERFEJSU POSTACI ===
    InteractionComponent* getInteractionComponent() { return getComponent<InteractionComponent>().get(); }
    InventoryComponent* getInventoryComponent() { return getComponent<InventoryComponent>().get(); }
    BuildingComponent* getBuildingComponent() { return getComponent<BuildingComponent>().get(); }

private:
    bool initializeComponents();
    void calculateDerivedStats();
    void applyClassBonuses();
    template<typename T>
    void notifyCharacterEvent(const T& event);
    void updatePlayerStats(float deltaTime);

private:
    std::string m_playerId;
    CharacterStats m_stats;
    PlayerStats m_playerStats;
    std::chrono::high_resolution_clock::time_point m_startTime;
    Vector3 m_lastPosition;
};
