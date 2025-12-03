#pragma once

#include "../core/IGameSystem.h"
#include "../components/EquipmentComponent.h"
#include "../components/StatsComponent.h"
#include "../game/Item.h"
#include <memory>
#include <string>
#include <vector>

class EquipmentSystem : public IGameSystem {
public:
    EquipmentSystem();
    virtual ~EquipmentSystem() override = default;

    // IGameSystem interface
    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;
    std::string getName() const override;
    int getPriority() const override;

    /**
     * @brief Spróbuj założyć przedmiot z ekwipunku
     * @param entity Encja, która zakłada przedmiot
     * @param item Przedmiot do założenia
     * @return true jeśli udało się założyć
     */
    bool equipItem(GameEntity* entity, std::unique_ptr<Item> item);

    /**
     * @brief Zdejmij przedmiot ze slotu
     * @param entity Encja, z której zdejmujemy przedmiot
     * @param slot Slot, z którego zdejmujemy
     * @return Zdjęty przedmiot lub nullptr
     */
    std::unique_ptr<Item> unequipItem(GameEntity* entity, EquipmentSlot slot);

    /**
     * @brief Aplikuj bonusy ze statystyk przedmiotu do statystyk encji
     * @param statsComp Komponent statystyk encji
     * @param itemStats Statystyki przedmiotu
     * @param apply true = dodaj, false = odejmij
     */
    void applyStats(StatsComponent* statsComp, const EquipmentItem::EquipmentStats& itemStats, bool apply);

private:
    std::string m_name;
};
