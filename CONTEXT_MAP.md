# Mapa Kontekstu Projektu Simple3DGame

> [!IMPORTANT]
> **GŁÓWNE ZRÓDŁO KONTEKSTU**: Pełna wizja, DNA 9.7 i architektura znajdują się w [MASTER_CONTEXT.md](file:///f:/Simple3DGame/context_hub/MASTER_CONTEXT.md).

## 1. Opis Projektu

Simple3DGame to zaawansowana gra symulacyjna 3D z mechanikami kolonii, w której gracze zarządzają osadnikami (Settlers), budują struktury, zbierają zasoby i eksplorują świat. Gra wykorzystuje architekturę ECS (Entity-Component-System) i framework graficzny Raylib.

## 2. Struktura Katalogów

```
Simple3DGame/
├── core/           Rdzeń silnika ECS i systemu gry
├── entities/       Encje gry (PlayerCharacter, Ant)
├── components/     Komponenty ECS (InventoryComponent, SkillsComponent, etc.)
├── systems/        Systemy zarządzające (BuildingSystem, CraftingSystem, etc.)
├── game/           Klasy logiki gry (Colony, Settler, Terrain, ResourceNode, etc.)
├── src/            Główna aplikacja (main.cpp, pętla gry)
├── events/         System zdarzeń (InteractionEvents.h)
├── docs/           Dokumentacja projektu
├── tests/          Testy jednostkowe i integracyjne
├── resources/      Zasoby (tekstury, modele, dźwięki)
├── raylib/         Framework graficzny Raylib
└── vendor/         Biblioteki zewnętrzne
```

## 3. Kluczowe Klasy i Architektura

### 3.1 Core - Rdzeń Silnika

- **GameEntity** (`core/GameEntity.h`) - Klasa bazowa dla wszystkich encji, zarządza komponentami
- **IComponent** (`core/IComponent.h`) - Interfejs dla komponentów
- **GameComponent** (`core/GameComponent.h`) - Alias dla IComponent
- **GameSystem** (`core/GameSystem.h`) - Klasa bazowa dla wszystkich systemów
- **IGameSystem** (`core/IGameSystem.h`) - Interfejs systemu gry
- **EventSystem** (`core/EventSystem.h`) - System zdarzeń dla komunikacji między systemami
- **GameEngine** (`core/GameEngine.h`) - Zarządzanie silnikiem gry
- **DIContainer** (`core/DIContainer.h`) - Dependency Injection Container
- **ObjectPool** (`core/ObjectPool.h`) - Pool obiektów dla optymalizacji
- **Logger** (`core/Logger.h`) - System logowania

### 3.2 Components - Komponenty ECS

- **InventoryComponent** - Zarządzanie ekwipunkiem postaci
- **SkillsComponent** - Umiejętności postaci (drzewo umiejętności)
- **StatsComponent** - Statystyki (zdrowie, energia, głód)
- **EquipmentComponent** - Wyposażenie postaci (sloty: broń, tarcza, etc.)
- **PositionComponent** - Pozycja w świecie
- **ResourceComponent** - Zarządzanie zasobami postaci
- **BuildingComponent** - Komponent dla budynków
- **InteractionComponent** - Obsługa interakcji z obiektami
- **TreeComponent** - Komponent dla drzew
- **EnergyComponent** - Energia postaci

### 3.3 Systems - Systemy Gry

- **BuildingSystem** - System budowy struktur (budynki, ściany, drzwi)
- **CraftingSystem** - System craftingu przedmiotów
- **InteractionSystem** - Zarządzanie interakcjami (raycast, klikanie)
- **InventorySystem** - Zarządzanie ekwipunkiem
- **StorageSystem** - System magazynowania zasobów
- **NeedsSystem** - System potrzeb (głód, energia, sen)
- **FoodSystem** - System żywności i odżywiania
- **SkillsSystem** - System rozwijania umiejętności
- **EquipmentSystem** - Zarządzanie wyposażeniem
- **ResourceSystem** - Zarządzanie typami zasobów
- **UISystem** - Interfejs użytkownika
- **TimeCycleSystem** - Cykl dzień/noc
- **TestSystem** - System testowania

### 3.4 Game - Klasy Logiki Gry

#### Świat i Środowisko

- **Terrain** (`game/Terrain.h/cpp`) - Generator terenu, zarządza wysokościami, drzewami i resource nodes
- **Colony** (`game/Colony.h/cpp`) - Kolonia, zarządza osadnikami, zasobami, budynkami i AI
- **ColonyAI** (`game/ColonyAI.h/cpp`) - Sztuczna inteligencja kolonii
- **NavigationGrid** (`game/NavigationGrid.h/cpp`) - Pathfinding i nawigacja (A*)

#### Postacie i Zwierzęta

- **Settler** (`game/Settler.h/cpp`) - Osadnik z profesjami, zadaniami, stanami (IDLE, CHOPPING, MINING, BUILDING, etc.)
- **Animal** (`game/Animal.h/cpp`) - Zwierzęta (króliki, jelenie)
- **Spider** (`game/Spider.h/cpp`) - Pająki (wrogowie)

#### Zasoby i Obiekty

- **ResourceNode** (`game/ResourceNode.h/cpp`) - Węzły zasobów (kamień, ruda)
- **Tree** (`game/Tree.h/cpp`) - Drzewa do wycinania
- **Bush** (`game/Colony.h`) - Krzewy z owocami
- **WorldItem** (`game/Colony.h`) - Przedmioty upuszczone w świecie

#### Budynki i Konstrukcje

- **BuildingBlueprint** (`game/BuildingBlueprint.h/cpp`) - Szablon budynku
- **BuildingInstance** (`game/BuildingInstance.h`) - Instancja budynku w świecie
- **BuildingTask** (`game/BuildingTask.h/cpp`) - Zadanie budowy
- **Door** (`game/Door.h/cpp`) - Drzwi z animacją
- **Bed** (`game/Bed.h/cpp`) - Łóżka dla osadników

#### Pozostałe

- **Item** (`game/Item.h/cpp`) - Przedmioty (narzędzia, jedzenie, materiały)
- **Projectile** (`game/Projectile.h/cpp`) - Pociski (łuki, kamienie)
- **GatheringTask** (`game/GatheringTask.h/cpp`) - Zadania zbierania zasobów
- **InteractableObject** (`game/InteractableObject.h/cpp`) - Obiekty do interakcji
- **BoxCollider** (`game/BoxCollider.h`) - Kolizje AABB

### 3.5 Entities

- **PlayerCharacter** (`entities/PlayerCharacter.h/cpp`) - Postać gracza
- **Ant** (`entities/Ant.h/cpp`) - Mrówki
- **GameEntity** (`entities/GameEntity.h`) - Klasa bazowa encji

### 3.6 Events

- **InteractionEvents** (`events/InteractionEvents.h`) - Zdarzenia interakcji

## 4. Główne Mechaniki Gry

### 4.1 System Osadników (Settlers)

- **Profesje**: BUILDER, GATHERER, MINER, FARMER, CRAFTER
- **Stany**: IDLE, MOVING, CHOPPING, MINING, GATHERING, BUILDING, CRAFTING, EATING, SLEEPING
- **AI**: Autonomiczne szukanie zadań, pathfinding, zarządzanie potrzebami

### 4.2 System Zasobów

- **Typy**: Drewno, Kamień, Żelazo, Złoto, Jedzenie, Fibra
- **Zbieranie**: Wyklinanie drzew, kopanie kamieni, zbieranie z krzewów
- **Magazynowanie**: Ekwipunek osadników, budynki magazynowe

### 4.3 System Budowania

- **Typy budynków**: Ściany, podłogi, drzwi, magazyny, łóżka, furnace, workbench
- **Mechanika**: Planowanie → Zlecanie zadania → Gromadzenie zasobów → Budowa
- **Collision detection**: Sprawdzanie wolnej przestrzeni przed spawnem obiektów

### 4.4 System Craftingu

- **Receptury**: Definiowane przepisy (np. drewno → deski)
- **Stacje robocze**: Workbench, Furnace
- **Dependency**: Osadnicy szukają materiałów w magazynach

### 4.5 System Potrzeb (Needs)

- **Głód**: Osadnicy muszą jeść (berries, cooked meat)
- **Energia**: Regeneracja przez sen w łóżkach
- **Zdrowie**: Regeneracja przy niskim głodzie

### 4.6 System Umiejętności

- Rozwój umiejętności przez działania
- Zwiększone efektywności (szybsze zbieranie, budowanie)

### 4.7 System Interakcji

- **Raycast**: Wykrywanie kliknięć na obiekty
- **Selekcja**: Selekcja osadników (single, box)
- **Polecenia**: Ruchy, budowa, wycinanie, attack

## 5. Ważne Zależności

```
main.cpp → inicjalizuje → [Colony, Terrain, wszystkie systemy]
Colony → zarządza → [Settlers, Animals, ResourceNodes, Buildings]
Settler → korzysta z → [NavigationGrid, Colony.resourceNodes, BuildingSystem]
BuildingSystem → korzysta z → [BuildingBlueprint, StorageSystem, InteractionSystem]
CraftingSystem → korzysta z → [InventoryComponent, StorageSystem]
Terrain → generuje → [Trees, ResourceNodes]
ColonyAI → przydziela → [GatheringTasks, BuildingTasks]
```

## 6. Przepływ Danych

### Inicjalizacja (main.cpp)

1. Inicjalizacja Raylib
2. Tworzenie `Terrain` i generowanie świata
3. Tworzenie `Colony` i dodanie osadników
4. Inicjalizacja systemów (BuildingSystem, CraftingSystem, UISystem, etc.)
5. Rejestracja blueprintów budynków

### Pętla Gry (main loop)

1. **Input**: `processInput()` - obsługa klawiszy, myszy, selekcji
2. **Update**:
   - `colony.update(deltaTime, trees, buildings)`
   - `BuildingSystem.update()`
   - `CraftingSystem.update()`
   - `UISystem.update()`
3. **Render**:
   - `terrain.render()`
   - `colony.render()`
   - Renderowanie ścieżek, UI, debugów

### AI Osadnika (Settler.Update)

1. Sprawdzenie potrzeb (głód, energia)
2. Jeśli brak zadań → szukanie nowych (przez ColonyAI)
3. Wykonywanie obecnego zadania (MOVE → GATHER → DEPOSIT)
4. Pathfinding do celu
5. Animacje i akcje

## 7. Wzorce Projektowe

- **ECS (Entity-Component-System)**: Architektura całego projektu
- **Observer/Event System**: Komunikacja między systemami przez EventSystem
- **Factory Method**: Tworzenie budynków (BuildingBlueprint)
- **State Machine**: Stany osadników (SettlerState enum)
- **Singleton**: GameEngine, colony, systemy
- **Object Pool**: ObjectPool dla często tworzonych obiektów
- **Dependency Injection**: DIContainer

## 8. Zależności Zewnętrzne

- **Raylib** - Framework graficzny 3D/2D i dźwiękowy
- **CMake** - System budowania
- **vcpkg** - Manager pakietów dla C++
- **C++ STL** - vector, unordered_map, unique_ptr, shared_ptr

## 9. Architektura Kodu

### Smart Pointers

- `std::unique_ptr` - dla własności obiektów (Trees, ResourceNodes, Buildings)
- `std::shared_ptr` - w przypadkach współdzielonej własności
- Raw pointers - dla odniesień nieposiadających (Colony.settlers)

### Pamięć

- RAII - automatyczne zarządzanie zasobami
- Move semantics - transferowanie własności (std::move)

## 10. Ostatnie Zmiany (z historii konwersacji)

### Collision Checks dla Resources (31.12.2024)

- Zmodyfikowano `Terrain.cpp`: spawning drzew i kamieni sprawdza kolizje z budynkami
- Zapobiega spawowaniu resources wewnątrz budynków

### Stone Search Fix (31.12.2024)

- Naprawiono bug: osadnicy nie mogli znaleźć kamieni
- Fix: użycie prawidłowego parametru `resourceNodes` w `Settler::FindNearestResourceNode()`
- Dodano logi debugujące

### Logistyka Kolonii i Budynki Funkcyjne (03.01.2026)

- Wprowadzono system **Logistyki Kolonii** w `Colony.cpp`.
- Dodano nowe budynki w `BuildingSystem.cpp`:
    - **Tartak (Sawmill)**: +50% do szybkości rąbania drewna (zasięg 40m).
    - **Kuźnia (Blacksmith)**: Globalny bonus +20% do wydajności narzędzi.
    - **Studnia (Well)**: Pasywna regeneracja energii dla osadników w promieniu 25m.
- Metoda `Colony::getEfficiencyModifier` zarządza kalkulacją bonusów na podstawie pozycji i stanu osadnika.
- Zintegrowano bonusy w `Settler::Update` (regeneracja energii) oraz `UpdateChopping` i `UpdateMining`.

## 11. TODO / Planowane Rozszerzenia

- [ ] **Serializacja**: Zapisywanie/ładowanie stanu gry do pliku
- [ ] **Advanced AI**: Bardziej zaawansowane zachowania osadników, priorytety
- [ ] **Combat System**: Walka z wrogami (Spiders), defence
- [ ] **Weather System**: Deszcz, śnieg, wpływ na mechanikę gry
- [ ] **Day/Night Cycle**: Rozszerzenie TimeCycleSystem
- [ ] **Multiplayer**: Tryb wieloosobowy (Network System)
- [ ] **Optimization**: Spatial partitioning, frustum culling

## 12. Punkty Wejścia do Kodu

- **Start aplikacji**: `src/main.cpp::main()`
- **Pętla gry**: `main.cpp::338-459` (main loop)
- **Logika kolonii**: `game/Colony.cpp::update()`
- **AI osadnika**: `game/Settler.cpp::Update()`
- **System budowania**: `systems/BuildingSystem.cpp`
- **Rendering**: `main.cpp::renderScene()`

## 13. Kluczowe Pliki do Edycji

Dla konkretnych zadań:

- **Terrain/World**: `game/Terrain.cpp`, `game/NavigationGrid.cpp`
- **AI Osadników**: `game/Settler.cpp`, `game/ColonyAI.cpp`
- **Budowanie**: `systems/BuildingSystem.cpp`, `game/BuildingBlueprint.cpp`
- **Crafting**: `systems/CraftingSystem.cpp`, `game/Item.cpp`
- **UI**: `systems/UISystem.cpp`
- **Potrzeby**: `systems/NeedsSystem.cpp`, `systems/FoodSystem.cpp`

## 14. Standardy Pracy AI (Antigravity DNA 6.0 - Simple3DGame)

Projekt wykorzystuje najwyższy stopień autonomii AI (Partner Ekspert - KONSTYTUCJA 6.0):
- **Role i Tożsamość:** Agent działa jako **[ARCHITEKT]** / **[PROJEKTANT]** / **[DEBUGGER]** z pełną odpowiedzialnością za projekt.
- **Źródło Prawdy:** Reguły zdefiniowane w `.agent/rules/` (`00identity.md`, `01automation.md`, `02visuals.md`, `03cppengineering.md`).
- **Protokół "Red Team":** Automatyczna autokrytyka i symulacja błędów przed wdrożeniem zmian.
- **Protokół "Visual Parity 110%":** Implementacja musi przewyższać koncept estetycznie.
- **Protokół "R&D Sandbox":** Samodzielne prototypowanie nowych mechanik.
- **Decyzyjność:** Obowiązkowa analiza **Pros & Cons** dla każdej istotnej zmiany architektonicznej.
- **Wizualizacja:** Obowiązkowe schematy (`generate_image`) dla zadań o PredictedTaskSize > 5.
- **Turbo:** Pełna autonomia terminalowa i protokół Silent Fix.

- **Pamięć:** AI ma obowiązek aktualizować `CONTEXT_MAP.md` i `task.md` po każdej sesji.

