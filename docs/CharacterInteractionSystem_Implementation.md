# System Interakcji Postaci - Dokumentacja Implementacji

## Przegląd

Zaimplementowano kompletny system interakcji postaci dla Simple3DGame zgodnie z architekturą opisaną w [`docs/CharacterInteractionSystem_Architecture.md`](docs/CharacterInteractionSystem_Architecture.md:1).

## Zaimplementowane Komponenty

### 1. Klasy Wsparcia

#### [`game/Item.h`](game/Item.h:1) / [`game/Item.cpp`](game/Item.cpp:1)
- **Bazowa klasa [`Item`](game/Item.h:68)** - abstrakcyjna klasa dla wszystkich przedmiotów
- **[`ResourceItem`](game/Item.h:232)** - przedmioty zasobowe (drewno, kamień, etc.)
- **[`EquipmentItem`](game/Item.h:289)** - ekwipunek (broń, zbroja)
- **[`ConsumableItem`](game/Item.h:397)** - przedmioty zużywalne (jedzenie, mikstury)

**Funkcjonalności:**
- System rzadkości przedmiotów ([`ItemRarity`](game/Item.h:24))
- Stackowanie przedmiotów
- Klonowanie przedmiotów
- Wyświetlanie informacji o przedmiotach

#### [`game/InteractableObject.h`](game/InteractableObject.h:1) / [`game/InteractableObject.cpp`](game/InteractableObject.cpp:1)
- **Interfejs [`InteractableObject`](game/InteractableObject.h:56)** - bazowy interfejs dla obiektów interaktywnych
- **[`BaseInteractableObject`](game/InteractableObject.h:117)** - bazowa implementacja
- **[`CollectableObject`](game/InteractableObject.h:227)** - obiekty zbieralne
- **[`ContainerObject`](game/InteractableObject.h:263)** - kontenery
- **[`SwitchObject`](game/InteractableObject.h:303)** - przełączniki

**Funkcjonalności:**
- Typy interakcji ([`InteractionType`](game/InteractableObject.h:13))
- Zasięg interakcji
- Walidacja możliwości interakcji
- Renderowanie podpowiedzi

#### [`game/BuildingBlueprint.h`](game/BuildingBlueprint.h:1) / [`game/BuildingBlueprint.cpp`](game/BuildingBlueprint.cpp:1)
- **Klasa [`BuildingBlueprint`](game/BuildingBlueprint.h:37)** - plany budynków
- **Struktura [`ResourceRequirement`](game/BuildingBlueprint.h:11)** - wymagania zasobów
- **Struktura [`BoxCollider`](game/BuildingBlueprint.h:24)** - kolizje budynków

**Funkcjonalności:**
- Zarządzanie wymaganiami zasobów
- Walidacja dostępności zasobów
- Konfiguracja parametrów budynku

### 2. Komponenty ECS

#### [`components/InteractionComponent.h`](components/InteractionComponent.h:1) / [`components/InteractionComponent.cpp`](components/InteractionComponent.cpp:1)
Komponent zarządzający możliwością interakcji encji z obiektami.

**Kluczowe funkcje:**
- [`setInteractionRange()`](components/InteractionComponent.h:42) - ustawienie zasięgu
- [`canInteractWith()`](components/InteractionComponent.h:68) - sprawdzanie możliwości interakcji
- [`performInteraction()`](components/InteractionComponent.h:96) - wykonanie interakcji
- System cooldownu interakcji

#### [`components/InventoryComponent.h`](components/InventoryComponent.h:1) / [`components/InventoryComponent.cpp`](components/InventoryComponent.cpp:1)
Komponent zarządzający ekwipunkiem encji.

**Kluczowe funkcje:**
- [`addItem()`](components/InventoryComponent.h:89) - dodawanie przedmiotów
- [`removeItem()`](components/InventoryComponent.h:97) - usuwanie przedmiotów
- [`sortItems()`](components/InventoryComponent.h:157) - sortowanie
- [`compressItems()`](components/InventoryComponent.h:162) - kompresja stacków
- System zarządzania wagą i pojemnością

### 3. Systemy

#### [`systems/InteractionSystem.h`](systems/InteractionSystem.h:1) / [`systems/InteractionSystem.cpp`](systems/InteractionSystem.cpp:1)
System wykrywania i wykonywania interakcji z obiektami.

**Kluczowe funkcje:**
- [`raycastFromCamera()`](systems/InteractionSystem.h:93) - raycast z kamery gracza
- [`processPlayerInput()`](systems/InteractionSystem.h:106) - przetwarzanie inputu
- [`registerInteractableObject()`](systems/InteractionSystem.h:123) - rejestracja obiektów
- Automatyczne wykrywanie obiektów w zasięgu

**Struktura [`RaycastHit`](systems/InteractionSystem.h:14):**
- Informacje o trafionym obiekcie
- Pozycja i odległość trafienia
- Normalna powierzchni

#### [`systems/InventorySystem.h`](systems/InventorySystem.h:1) / [`systems/InventorySystem.cpp`](systems/InventorySystem.cpp:1)
System zarządzania ekwipunkiem dla wszystkich encji.

**Kluczowe funkcje:**
- [`transferItem()`](systems/InventorySystem.h:47) - transfer między ekwipunkami
- [`moveItem()`](systems/InventorySystem.h:55) - przenoszenie w ekwipunku
- [`splitStack()`](systems/InventorySystem.h:62) - dzielenie stacków
- [`autoLoot()`](systems/InventorySystem.h:76) - automatyczne zbieranie
- [`sortAllInventories()`](systems/InventorySystem.h:95) - sortowanie wszystkich ekwipunków

#### [`systems/BuildingSystem.h`](systems/BuildingSystem.h:1)
Rozszerzony system budowania z walidacją i placement.

**Nowe klasy:**
- [`BuildingInstance`](systems/BuildingSystem.h:47) - instancja budynku
- [`BuildTask`](systems/BuildingSystem.h:137) - zadanie budowy
- [`ValidationResult`](systems/BuildingSystem.h:33) - wynik walidacji

**Kluczowe funkcje:**
- [`validatePlacement()`](systems/BuildingSystem.h:283) - walidacja umiejscowienia
- [`startBuilding()`](systems/BuildingSystem.h:275) - rozpoczęcie budowy
- [`consumeResources()`](systems/BuildingSystem.h:313) - konsumpcja zasobów
- [`snapToGrid()`](systems/BuildingSystem.h:362) - przyciąganie do siatki

### 4. System Zdarzeń

#### [`events/InteractionEvents.h`](events/InteractionEvents.h:1)
Kompletny zestaw zdarzeń dla systemu interakcji.

**Zaimplementowane zdarzenia:**
- [`InteractionStartedEvent`](events/InteractionEvents.h:14) - rozpoczęcie interakcji
- [`InteractionCompletedEvent`](events/InteractionEvents.h:30) - zakończenie interakcji
- [`ItemPickedUpEvent`](events/InteractionEvents.h:48) - podniesienie przedmiotu
- [`ItemDroppedEvent`](events/InteractionEvents.h:61) - upuszczenie przedmiotu
- [`ItemAddedToInventoryEvent`](events/InteractionEvents.h:73) - dodanie do ekwipunku
- [`ItemRemovedFromInventoryEvent`](events/InteractionEvents.h:85) - usunięcie z ekwipunku
- [`ItemUsedEvent`](events/InteractionEvents.h:97) - użycie przedmiotu
- [`BuildingStartedEvent`](events/InteractionEvents.h:109) - rozpoczęcie budowy
- [`BuildingCompletedEvent`](events/InteractionEvents.h:121) - zakończenie budowy
- [`BuildingCancelledEvent`](events/InteractionEvents.h:132) - anulowanie budowy
- [`BuildingDestroyedEvent`](events/InteractionEvents.h:146) - zniszczenie budynku
- [`InventoryCapacityExceededEvent`](events/InteractionEvents.h:158) - przekroczenie pojemności
- [`ContainerOpenedEvent`](events/InteractionEvents.h:169) - otwarcie kontenera
- [`ContainerClosedEvent`](events/InteractionEvents.h:180) - zamknięcie kontenera
- [`ResourceGatheredEvent`](events/InteractionEvents.h:191) - zebranie zasobu

## Integracja z Istniejącym Kodem

### Aktualizacja CMakeLists.txt
Plik [`CMakeLists.txt`](CMakeLists.txt:1) został zaktualizowany o nowe pliki źródłowe:

**Nowe systemy:**
- [`systems/InteractionSystem.cpp`](systems/InteractionSystem.cpp:1)
- [`systems/InventorySystem.cpp`](systems/InventorySystem.cpp:1)

**Nowe komponenty:**
- [`components/InteractionComponent.cpp`](components/InteractionComponent.cpp:1)
- [`components/InventoryComponent.cpp`](components/InventoryComponent.cpp:1)

**Nowa logika gry:**
- [`game/Item.cpp`](game/Item.cpp:1)
- [`game/InteractableObject.cpp`](game/InteractableObject.cpp:1)
- [`game/BuildingBlueprint.cpp`](game/BuildingBlueprint.cpp:1)

**Nowe katalogi include:**
- `game/`
- `events/`
- `ui/`

### Integracja z PlayerCharacter

Aby zintegrować system z [`PlayerCharacter`](entities/PlayerCharacter.h:109), należy:

```cpp
// W PlayerCharacter::initialize()
auto interactionComp = std::make_shared<InteractionComponent>();
addComponent(interactionComp);

auto inventoryComp = std::make_shared<InventoryComponent>(m_playerId, 100.0f);
addComponent(inventoryComp);
```

### Integracja z main.cpp

```cpp
// Inicjalizacja systemów
auto interactionSystem = std::make_shared<InteractionSystem>();
interactionSystem->setPlayerEntity(player);

auto inventorySystem = std::make_shared<InventorySystem>();
inventorySystem->registerInventory(player->getComponent<InventoryComponent>());

// W pętli gry
interactionSystem->processPlayerInput(camera);
interactionSystem->update(deltaTime);
inventorySystem->update(deltaTime);
```

## Przykłady Użycia

### 1. Tworzenie Obiektu Zbieralnego

```cpp
auto woodResource = std::make_unique<CollectableObject>(
    "Drewno",
    Vector3{10.0f, 0.0f, 10.0f},
    "WOOD",
    5
);

interactionSystem->registerInteractableObject(woodResource.get());
```

### 2. Dodawanie Przedmiotu do Ekwipunku

```cpp
auto woodItem = std::make_unique<ResourceItem>("WOOD", "Drewno", "Surowe drewno");
woodItem->setWeight(0.5f);
woodItem->setValue(1);

auto inventory = player->getComponent<InventoryComponent>();
inventory->addItem(std::move(woodItem), 10);
```

### 3. Tworzenie Planu Budynku

```cpp
auto houseBlueprint = std::make_unique<BuildingBlueprint>(
    "house_basic",
    "Podstawowy Dom",
    "Prosty drewniany dom"
);

houseBlueprint->addRequirement(ResourceRequirement("WOOD", 50, false));
houseBlueprint->addRequirement(ResourceRequirement("STONE", 20, false));
houseBlueprint->setBuildTime(30.0f);
houseBlueprint->setMaxHealth(200.0f);

buildingSystem->registerBlueprint(std::move(houseBlueprint));
```

### 4. Rozpoczęcie Budowy

```cpp
Vector3 buildPosition = {20.0f, 0.0f, 20.0f};

// Walidacja
auto validation = buildingSystem->validatePlacement("house_basic", buildPosition);
if (validation.isValid) {
    auto task = buildingSystem->startBuilding("house_basic", buildPosition, player);
}
```

### 5. Obsługa Zdarzeń

```cpp
// Rejestracja handlera dla podniesienia przedmiotu
EventBus::registerHandler<ItemPickedUpEvent>(
    [](const std::any& eventData) {
        auto event = std::any_cast<ItemPickedUpEvent>(eventData);
        std::cout << "Podniesiono: " << event.item->getDisplayName() 
                  << " x" << event.quantity << std::endl;
    },
    "ItemPickupLogger"
);

// Wysłanie zdarzenia
ItemPickedUpEvent event(player, item.get(), 5, "ground", position);
EventBus::send(event);
```

## Następne Kroki

### Do Zaimplementowania:
1. **UI/UX System** - [`UIManager`](docs/CharacterInteractionSystem_Architecture.md:604), ekrany UI
2. **BuildingSystem.cpp** - implementacja rozszerzonego systemu budowania
3. **Pełna integracja** z [`PlayerCharacter`](entities/PlayerCharacter.h:109) i istniejącymi systemami
4. **Testy jednostkowe** dla wszystkich komponentów
5. **Optymalizacja wydajności** - object pooling, LOD system

### Zalecenia:
- Przetestować raycast system z rzeczywistą kamerą
- Dodać wizualizację debug dla zasięgu interakcji
- Zaimplementować UI dla ekwipunku i budowania
- Dodać dźwięki dla interakcji
- Rozszerzyć system o więcej typów przedmiotów

## Podsumowanie

Zaimplementowano kompletny fundament systemu interakcji postaci obejmujący:
- ✅ 3 klasy wsparcia (Item, InteractableObject, BuildingBlueprint)
- ✅ 2 komponenty ECS (InteractionComponent, InventoryComponent)
- ✅ 3 systemy (InteractionSystem, InventorySystem, BuildingSystem header)
- ✅ 15 typów zdarzeń dla interakcji
- ✅ Aktualizacja CMakeLists.txt
- ✅ Dokumentacja i przykłady użycia

System jest gotowy do integracji z istniejącym kodem i dalszego rozwijania.
