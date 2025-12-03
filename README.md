# Simple3DGame - System Mechanik Postaci

## Opis Projektu

Simple3DGame to zaawansowany system mechanik postaci w grze 3D, który obejmuje kompleksowe zarządzanie zasobami, budowaniem struktur, systemem żywności, umiejętnościami i wyposażeniem.

## Struktura Projektu

```
Simple3DGame/
├── components/
│   ├── EquipmentComponent.h
│   ├── ResourceComponent.h
│   ├── SkillsComponent.h
│   ├── StatsComponent.h
│   └── ...
├── core/
│   ├── GameComponent.h
│   ├── GameEngine.h
│   ├── GameSystem.h
│   ├── Logger.h
│   └── ObjectPool.h
├── entities/
│   ├── GameEntity.h
│   └── ...
├── systems/
│   ├── BuildingSystem.h
│   ├── EquipmentSystem.h
│   ├── FoodSystem.h
│   ├── ResourceSystem.h
│   ├── SkillsSystem.h
│   ├── StorageSystem.h
│   ├── TestSystem.h
│   └── UISystem.h
└── src/
    ├── BuildingTask.cpp
    ├── GatheringTask.cpp
    └── ...
```

## Funkcjonalności

### 1. System Zasobów
- Zarządzanie różnymi typami zasobów (drewno, kamień, jedzenie, złoto)
- System ekwipunku z możliwością przenoszenia i przechowywania zasobów
- Zarządzanie magazynami i transportem zasobów

### 2. System Budowy
- Budowanie różnych struktur i obiektów w grze
- Zarządzanie zasobami potrzebnymi do budowy
- System zarządzania budynkami i ich funkcjami

### 3. System Żywności i Głodu
- Zarządzanie poziomem głodu postaci
- System regeneracji zdrowia w zależności od poziomu głodu
- Zarządzanie zapasami żywności

### 4. System Umiejętności
- Drzewa rozwoju umiejętności
- System poziomowania umiejętności
- Zastosowanie umiejętności w różnych zadaniach

### 5. System Ekwipunku
- Zarządzanie wyposażeniem postaci
- System slotów ekwipunkowych
- Zarządzanie przedmiotami w ekwipunku

## Instalacja i Uruchomienie

1. Sklonuj repozytorium:
   ```
   git clone https://github.com/twoje-repozytorium/Simple3DGame.git
   ```

2. Przejdź do katalogu projektu:
   ```
   cd Simple3DGame
   ```

3. Skompiluj projekt:
   ```
   make
   ```

4. Uruchom grę:
   ```
   ./Simple3DGame
   ```

## Dokumentacja API

### Komponenty

#### EquipmentComponent
- Zarządza wyposażeniem postaci
- Metody: `addItemToInventory`, `removeItemFromInventory`, `equipItem`, `unequipItem`

#### ResourceComponent
- Zarządza zasobami postaci
- Metody: `addResource`, `removeResource`, `getResourceAmount`

#### SkillsComponent
- Zarządza umiejętnościami postaci
- Metody: `addSkill`, `removeSkill`, `levelUpSkill`

#### StatsComponent
- Zarządza statystykami postaci
- Metody: `modifyHealth`, `modifyEnergy`, `getCurrentHealth`, `getCurrentEnergy`

### Systemy

#### BuildingSystem
- Zarządza budową struktur
- Metody: `createBuilding`, `destroyBuilding`

#### EquipmentSystem
- Zarządza systemem wyposażenia
- Metody: `addPerson`, `removePerson`, `addItemToInventory`, `equipItem`

#### FoodSystem
- Zarządza systemem żywności
- Metody: `addPerson`, `removePerson`, `eatFood`

#### ResourceSystem
- Zarządza systemem zasobów
- Metody: `registerResourceType`, `getResourceInfo`

#### SkillsSystem
- Zarządza systemem umiejętności
- Metody: `addPerson`, `removePerson`, `addSkill`, `levelUpSkill`

#### StorageSystem
- Zarządza systemem magazynów
- Metody: `addStorage`, `removeStorage`, `transferResources`

#### TestSystem
- System testowania
- Metody: `addTest`, `runAllTests`

#### UISystem
- Zarządza interfejsem użytkownika
- Metody: `addUIElement`, `removeUIElement`

## Przykłady Kodu

### Tworzenie postaci z wyposażeniem

```cpp
// Tworzenie postaci
GameEntity* player = new GameEntity("player1");

// Dodawanie komponentów
player->addComponent(std::make_unique<StatsComponent>("player1", 100.0f, 100.0f));
player->addComponent(std::make_unique<EquipmentComponent>("player1"));
player->addComponent(std::make_unique<ResourceComponent>("player1", 100.0f));

// Dodawanie przedmiotów do ekwipunku
auto equipmentSystem = GameEngine::getInstance().getSystem<EquipmentSystem>("EquipmentSystem");
equipmentSystem->addItemToInventory("player1", "sword", 1);
equipmentSystem->addItemToInventory("player1", "shield", 1);

// Wyposażanie przedmiotów
equipmentSystem->equipItem("player1", "sword", EquipmentSlot::WEAPON);
equipmentSystem->equipItem("player1", "shield", EquipmentSlot::SHIELD);
```

### Budowanie budynku

```cpp
// Tworzenie budynku
auto buildingSystem = GameEngine::getInstance().getSystem<BuildingSystem>("BuildingSystem");
buildingSystem->createBuilding("house", Vector3(10.0f, 0.0f, 10.0f));

// Dodawanie zasobów do budowy
auto resourceSystem = GameEngine::getInstance().getSystem<ResourceSystem>("ResourceSystem");
resourceSystem->registerResourceType(ResourceType::WOOD, "Drewno", "Podstawowy materiał budowlany", 1.0f, 1.0f, true, 50, 1);
```

## Testowanie

Projekt zawiera system testów, który można uruchomić za pomocą:

```cpp
auto testSystem = GameEngine::getInstance().getSystem<TestSystem>("TestSystem");
testSystem->runAllTests();
```

## Optymalizacje

Projekt został zoptymalizowany pod kątem wydajności, w tym:
- Object pooling dla często tworzonych obiektów
- Caching zasobów i danych
- Optymalizacja pamięci

## Wymagania Systemowe

- System operacyjny: Windows 10 lub nowszy
- Kompilator: Visual Studio 2019 lub nowszy
- Biblioteki: raylib, glm

## Kontakt

W przypadku pytań lub problemów prosimy o kontakt pod adresem: support@simple3dgame.com