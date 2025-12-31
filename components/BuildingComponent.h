#pragma once

#include "../core/IComponent.h"
#include "../game/BuildingBlueprint.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <typeindex>

// Forward declarations
class GameEntity;

/**
 * @brief Struktura aktywnej budowy
 */
struct ActiveBuilding {
    std::string blueprintId;           // ID planu budynku
    std::unique_ptr<BuildingBlueprint> blueprint; // Plan budynku
    Vector3 position;                  // Pozycja budowy
    float progress;                    // Postęp budowy (0.0 - 1.0)
    float estimatedTime;               // Szacowany czas do zakończenia
    float startTime;                   // Czas rozpoczęcia
    bool isActive;                     // Czy budowa jest aktywna
    
    ActiveBuilding() : progress(0.0f), estimatedTime(0.0f), startTime(0.0f), isActive(false) {}
    
    ActiveBuilding(std::string id, std::unique_ptr<BuildingBlueprint> bp, Vector3 pos, float time)
        : blueprintId(std::move(id)), blueprint(std::move(bp)), position(pos),
          progress(0.0f), estimatedTime(time), startTime(0.0f), isActive(true) {}
};

/**
 * @brief Komponent budowy dla encji
 * Zarządza zdolnościami budowania encji i aktywnymi budowami
 */
class BuildingComponent : public IComponent {
public:
    /**
     * @brief Konstruktor
     * @param ownerId ID właściciela
     */
    BuildingComponent(const std::string& ownerId);

    /**
     * @brief Destruktor
     */
    virtual ~BuildingComponent() = default;

    // IComponent interface
    void update(float deltaTime) override;
    void render() override;
    void initialize() override;
    void shutdown() override;
    std::type_index getComponentType() const override;

    /**
     * @brief Sprawdza czy encja może rozpocząć budowę
     * @param blueprintId ID planu budynku
     * @return true jeśli może budować
     */
    bool canStartBuilding(const std::string& blueprintId) const;

    /**
     * @brief Rozpoczyna budowę
     * @param blueprintId ID planu budynku
     * @param position Pozycja budowy
     * @return true jeśli rozpoczęcie się powiodło
     */
    bool startBuilding(const std::string& blueprintId, const Vector3& position);

    /**
     * @brief Zatrzymuje budowę
     * @param blueprintId ID planu budynku
     * @return true jeśli zatrzymanie się powiodło
     */
    bool stopBuilding(const std::string& blueprintId);

    /**
     * @brief Pobiera aktywne budowy
     * @return Mapa aktywnych budów
     */
    const std::unordered_map<std::string, ActiveBuilding>& getActiveBuildings() const { return m_activeBuildings; }

    /**
     * @brief Sprawdza czy encja ma aktywne budowy
     * @return true jeśli ma aktywne budowy
     */
    bool hasActiveBuildings() const { return !m_activeBuildings.empty(); }

    /**
     * @brief Pobiera liczbę aktywnych budów
     * @return Liczba aktywnych budów
     */
    size_t getActiveBuildingCount() const { return m_activeBuildings.size(); }

    /**
     * @brief Ustawia maksymalną liczbę jednoczesnych budów
     * @param maxCount Maksymalna liczba
     */
    void setMaxConcurrentBuildings(size_t maxCount);

    /**
     * @brief Pobiera maksymalną liczbę jednoczesnych budów
     * @return Maksymalna liczba
     */
    size_t getMaxConcurrentBuildings() const;

    /**
     * @brief Ustawia podstawową prędkość budowania
     * @param speed Podstawowa prędkość
     */
    void setBaseBuildingSpeed(float speed);

    /**
     * @brief Pobiera podstawową prędkość budowania
     * @return Podstawowa prędkość
     */
    float getBaseBuildingSpeed() const;

    /**
     * @brief Oblicza aktualną prędkość budowania z modyfikatorami
     * @return Aktualna prędkość
     */
    float getCurrentBuildingSpeed() const;

    /**
     * @brief Dodaje przedmiot do budowy
     * @param blueprintId ID planu
     * @param quantity Ilość
     * @return true jeśli dodanie się powiodło
     */
    bool addRequiredItem(const std::string& blueprintId, const std::string& itemId, int quantity);

    /**
     * @brief Usuwa wymagany przedmiot z budowy
     * @param blueprintId ID planu
     * @param quantity Ilość
     * @return true jeśli usunięcie się powiodło
     */
    bool removeRequiredItem(const std::string& blueprintId, const std::string& itemId, int quantity);

    /**
     * @brief Sprawdza czy budowa ma wszystkie wymagane przedmioty
     * @param blueprintId ID planu
     * @return true jeśli ma wszystkie przedmioty
     */
    bool hasRequiredItems(const std::string& blueprintId) const;

    /**
     * @brief Pobiera postęp budowy
     * @param blueprintId ID planu
     * @return Postęp (0.0 - 1.0) lub -1.0 jeśli nie znaleziono
     */
    float getBuildingProgress(const std::string& blueprintId) const;

    /**
     * @brief Pobiera pozostały czas budowy
     * @param blueprintId ID planu
     * @return Pozostały czas w sekundach lub -1.0 jeśli nie znaleziono
     */
    float getRemainingTime(const std::string& blueprintId) const;

private:
    /**
     * @brief Znajduje aktywną budowę
     * @param blueprintId ID planu
     * @return Iterator do budowy lub end()
     */
    auto findActiveBuilding(const std::string& blueprintId);

private:
    /** ID właściciela */
    std::string m_ownerId;
    
    /** Aktywne budowy */
    std::unordered_map<std::string, ActiveBuilding> m_activeBuildings;
    
    /** Maksymalna liczba jednoczesnych budów */
    size_t m_maxConcurrentBuildings;
    
    /** Podstawowa prędkość budowania */
    float m_baseBuildingSpeed;
    
    /** Cache dostępnych planów budynków */
    std::vector<std::string> m_availableBlueprints;
};