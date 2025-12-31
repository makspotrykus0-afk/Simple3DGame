
Plan Refaktoryzacji Architektury Simple3DGame

Cel

Poprawa struktury architektonicznej projektu poprzez:

Ujednolicenie systemów (wszystkie dziedziczące po GameSystem)

Wprowadzenie mechanizmu Dependency Injection

Wykorzystanie EventSystem do luźnego sprzężenia

Uporządkowanie rejestracji systemów w GameEngine

Integracja SkillsSystem z resztą systemów

Analiza Obecnego Stanu

Istniejące Systemy

BuildingSystem (GameSystem ✓)

EquipmentSystem (GameSystem ✓)

FoodSystem (GameSystem ✓)

InteractionSystem (GameSystem ✓)

InventorySystem (GameSystem ✓)

NeedsSystem (GameSystem ✓)

ResourceSystem (GameSystem ✓)

SkillsSystem (✗ NIE dziedziczy po GameSystem)

StorageSystem (GameSystem ✓)

TimeCycleSystem (GameSystem ✓)

UISystem (GameSystem ✓)

Kluczowe Problemy

**SkillsSystem jako osobnaSkillsSystem jako osobna klasa - nie jest zarządzany przez GameEngine

Bezpośrednie zależności - systemy mają bezpośrednie wskaźniki do innych systemów

**Chaotyczna inicjalizacjaChaotyczna inicjalizacja w main.cpp - ponad 100 linii konfiguracji zależności

**BrakBrak warstwy abstrakcji - brak interfejsów dla kluczowych komponentów

**Słaba komunikacja międzySłaba komunikacja między systemami - brak użycia EventSystem

Proponowana Architektura

1. Hierarchia Systemów

text

IGameSystem (interface)
  └── GameSystem (abstract)
        ├── BuildingSystem
        ├── EquipmentSystem
        ├── FoodSystem
        ├── InteractionSystem
        ├── InventorySystem
        ├── NeedsSystem
        ├── ResourceSystem
        ├── SkillsSystem (nowe dziedziczenie)
        ├── StorageSystem
        ├── TimeCycleSystem
        └── UISystem

2. Dependency Injection Container

C++

undefinedclass DIContainer {
public:
    template<typename T>
    void registerService(std::unique_ptr<T> service);

    template<typename T>
    T* getService() const;

    void injectDependencies();
};

3. Event-Driven Communication

text

System A → EventBus → System B
          (Event)

4. Centralized System Registration

C++

void GameEngine::registerAllSystems() {
    // Rejestracja w odpowiedniej kolejności
    registerSystem<TimeCycleSystem>();
    registerSystem<ResourceSystem>();
    registerSystem<InventorySystem>();
    registerSystem<SkillsSystem>(); // Nowy system
    registerSystem<BuildingSystem>();
    // ... itd
}

Diagram Architektury (Mermaid)

mermaid

graph TD
    A[GameEngine] --> B[DIContainer]
    A --> C[EventSystem]

    B --> D[Systems]
    D --> D1[BuildingSystem]
    D --> D2[EquipmentSystem]
    D --> D3[FoodSystem]
    D --> D4[InteractionSystem]
    D --> D5[InventorySystem]
    D --> D6[NeedsSystem]
    D --> D7[ResourceSystem]
    D --> D8[SkillsSystem]
    D --> D9[StorageSystem]
    D --> D10[TimeCycleSystem]
    D --> D11[UISystem]

    C --> E[Events]
    E --> E1[SkillLevelUpEvent]
    E --> E2[ResourceGatheredEvent]
    E --> E3[BuildingCompletedEvent]
    E --> E4[InteractionEvent]

    D8 --> F[SkillsComponent]
    D4 --> G[Uses Skills]
    D1 --> H[Uses Skills]

    D4 -->|publishes| E4
    E1 -->|subscribes| D11

Kroki Implementacji

Krok 1: Refaktoryzacja SkillsSystem

Zmienić dziedziczenie na GameSystem

Zaimplementować wymagane metody wirtualne

Zaktualizować pliki .cpp i .h

Krok 2: Stworzenie DIContainer

Plik core/DIContainer.h

Rejestracja usług (systemów)

Automatyczne wstrzykiwanie zależności

Krok 3: Integracja z GameEngine

Zmodyfikować GameEngine::registerSystem do użycia DIContainer

Uporządkować kolejność rejestracji

Dodać metodę injectDependencies()

Krok 4: Event System Integration

Zdefiniować eventy związane z umiejętnościami

Dodać subskrypcje w odpowiednich systemach

Zastąpić bezpośrednie wywołania przez eventy

Krok 5: Refaktoryzacja main.cpp

Przenieść inicjalizację do GameEngine::initializeAllSystems

Użyć DIContainer do zarządzania zależnościami

Uprościć konfigurację kolonii i terenu

Krok 6: Integracja Skills z innymi systemami

InteractionSystem - sprawdzanie umiejętności przed interakcją

BuildingSystem - premie za umiejętności budowlane

ResourceSystem - zwiększone zbiory z wyższym poziomem umiejętności

UISystem - wyświetlanie poziomów umiejętności

Szczegóły Techniczne

SkillsSystem jako GameSystem

C++

// systems/SkillsSystem.h
class SkillsSystem : public GameSystem {
public:
    SkillsSystem();
    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;
    std::string getName() const override { return "SkillsSystem"; }
    int getPriority() const override { return 10; }

    // Istniejące metody API
    void addSkillToEntity(GameEntity* entity, SkillType skillType, int level = 1, float xp = 0.0f);
    int getSkillLevelForEntity(GameEntity* entity, SkillType skillType) const;
    // ...
};

Nowe Eventy

C++

// events/SkillEvents.h
struct SkillLevelUpEvent {
    GameEntity* entity;
    SkillType skillType;
    int newLevel;
    float timestamp;
};

struct SkillXPAddedEvent {
    GameEntity* entity;
    SkillType skillType;
    float xpAdded;
    float totalXP;
};

Przykładowa Integracja

C++

undefined// W InteractionSystem::processChopping
void InteractionSystem::processChopping(Tree* tree, Settler* settler) {
    // Pobierz poziom umiejętności ścinania
    auto skillsSystem = GameEngine::getInstance().getSystem<SkillsSystem>();
    int woodcuttingLevel = skillsSystem->getSkillLevelForEntity(settler, SkillType::WOODCUTTING);

    // Oblicz efektywność na podstawie umiejętności
    float efficiency = 1.0f + (woodcuttingLevel * 0.1f);
    float chopTime = m_timeToChop / efficiency;

    // Dodaj XP za ścinanie
    if (m_chopTimer >= chopTime) {
        skillsSystem->addXPToSkillForEntity(settler, SkillType::WOODCUTTING, 10.0f);

        // Opublikuj event
        EventSystem::publish(SkillXPAddedEvent{
            settler, SkillType::WOODCUTTING, 10.0f
        });
    }
}

Harmonogram

**FazaFaza 1 (Podstawy) - Refaktoryzacja SkillsSystem + DIContainer (1-2 godziny)

**Faza 2Faza 2 (Integracja) - Event System + aktualizacja GameEngine (2-3 godziny)

**Faza 3Faza 3 (Main.cpp) - Uporządkowanie main.cpp (1 godzina)

Faza 4 (Integracja systemów) - Powiązanie Skills z innymi systemami (2-3 godziny)

**FazaFaza 5 (Testy) - Walidacja i testowanie (1-2 godziny)

Oczekiwane Korzyści

Lepsza modularność - Systemy są niezależne i wymienialne

Łatwiejsze testowanie - Możliwość mockowania zależności

Skalowalność - Łatwe dodawanie nowych systemów

**CzystszyCzystszy kod - Usunięcie bezpośrednich zależności

Elastyczność - Dynamiczne zmiany w runtime przez eventy

Ryzyka i Środki Zaradcze

| Ryzyko | Prawdopodobieństwo | Wpływ | Środek zaradczy |RyzykoPrawdopodobieństwoWpływŚrodek zaradczy

Zakłócenie istniejącej funkcjonalnościŚrednieWysokieTesty regresyjne, stopniowe wdrażanie

Problemy z kompilacjąWysokieŚrednieKompilacja przyrostowa, częste commity

Spadek wydajnościNiskieNiskieProfilowanie, optymalizacja EventSystem

Trudność w debugowaniuŚrednieŚrednieLogowanie szczegółowe, narzędzia debugowania

Wymagane Zmiany w Plikach

Nowe pliki:

core/DIContainer.h/cpp

events/SkillEvents.h

events/CommonEvents.h

Modyfikowane pliki:

systems/SkillsSystem.h/cpp

core/GameEngine.h/cpp

systems/InteractionSystem.h/cpp

systems/BuildingSystem.h/cpp

systems/ResourceSystem.h/cpp

src/main.cpp

core/EventSystem.h/cpp

Pliki testowe:

tests/TestSkillsSystem.cpp

tests/TestDIContainer.cpp

Metryki Sukcesu

Wszystkie systemy dziedziczą po GameSystem ✓

SkillsSystem zintegrowany z GameEngine ✓

Main.cpp zmniejszony o 50% linii kodu

Brak bezpośrednich zależności między systemami

Eventy wykorzystywane w co najmniej 3 przypadkach

Kompilacja bez błędów i ostrzeżeń

Testy jednostkowe pokrywają 80% nowego kodu

---

*Dokument przygotowany przez tryb Architect. Gotowy do implementacjiDokument przygotowany przez tryb Architect. Gotowy do implementacji w trybie Code.
