#pragma once

#include "../core/IComponent.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

/**
 * @brief Komponent zasobów dla encji
 * Implementuje zarządzanie zasobami w systemie ECS
 */
class ResourceComponent : public IComponent {
public:
    /**
     * @brief Struktura danych zasobu w komponencie
     */
    struct ComponentResource {
        std::string resourceType;  // Typ zasobu jako string
        int32_t currentAmount;
        int32_t reservedAmount;
        float weight;
        float volume;
        bool isStackable;
        std::chrono::high_resolution_clock::time_point lastGained;
        std::chrono::high_resolution_clock::time_point lastSpent;
        
        ComponentResource() : currentAmount(0), reservedAmount(0),
                             weight(1.0f), volume(1.0f), isStackable(true) {
            auto now = std::chrono::high_resolution_clock::now();
            lastGained = now;
            lastSpent = now;
        }
        
        /**
         * @brief Pobiera całkowitą ilość zasobu (bieżąca + zarezerwowana)
         * @return Całkowita ilość
         */
        int32_t getTotalAmount() const { return currentAmount + reservedAmount; }
    };

public:
    /**
     * @brief Konstruktor komponentu
     * @param ownerId ID właściciela encji
     */
    ResourceComponent(const std::string& ownerId);

    /**
     * @brief Destruktor
     */
    virtual ~ResourceComponent() override = default;

    // IComponent interface
    void update(float deltaTime) override;
    void render() override;
    void initialize() override;
    void shutdown() override;
    std::type_index getComponentType() const override;

    /**
     * @brief Dodaje zasób do komponentu
     * @param resourceType Typ zasobu
     * @param amount Ilość do dodania
     * @return Ilość faktycznie dodana
     */
    int32_t addResource(const std::string& resourceType, int32_t amount);

    /**
     * @brief Usuwa zasób z komponentu
     * @param resourceType Typ zasobu
     * @param amount Ilość do usunięcia
     * @return Ilość faktycznie usunięta
     */
    int32_t removeResource(const std::string& resourceType, int32_t amount);

    /**
     * @brief Pobiera ilość zasobu
     * @param resourceType Typ zasobu
     * @return Ilość zasobu
     */
    int32_t getResourceAmount(const std::string& resourceType) const;

private:
    /** ID właściciela encji */
    std::string m_ownerId;
    
    /** Mapa zasobów */
    std::unordered_map<std::string, ComponentResource> m_resources;
};