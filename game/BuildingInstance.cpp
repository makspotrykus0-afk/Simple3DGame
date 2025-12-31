#include "BuildingInstance.h"
#include "../core/GameEngine.h"
#include "../systems/StorageSystem.h"

bool BuildingInstance::addItem(std::unique_ptr<Item> item) {
    if (!item) return false;
    
    // Pobierz system magazynowania
    auto* storageSystem = GameEngine::getInstance().getSystem<StorageSystem>();
    if (!storageSystem) return false;

    // Próba dodania przedmiotu do magazynu (używa naszego nowego bool addItemToStorage)
    // UWAGA: item->getDisplayName() jest używane wewnątrz do mapowania na ResourceType
    // Ilość zakładamy jako 1 dla pojedynczego przedmiotu
    bool success = storageSystem->addItemToStorage(m_storageId, *item, 1);
    
    return success;
}
