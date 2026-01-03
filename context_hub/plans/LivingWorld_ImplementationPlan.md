# 🌐 LIVING WORLD SANDBOX - MASTERPIECE VISION DOCUMENT

**Data utworzenia**: 2026-01-03  
**Rola**: [ARCHITEKT] + [REŻYSER] + [STRATEG]  
**Cel**: Transformacja Simple3DGame → Żyjący Symulator Społeczeństwa (Dwarf Fortress × Rimworld × Cyberpunk Fantasy)

---

## 📋 EXECUTIVE SUMMARY

### Wizja Techniczna
Przekształcenie obecnej gry survival-colonization w **pełnoprawny sandbox symulator życia społecznego**, gdzie:
- **Świat żyje niezależnie od gracza** - AI miasta rozwijają się w tle, wojny toczą się automatycznie
- **Przełączalne tryby gry** - FPS Adventure → Colony Management → World Strategy
- **Emergent Storytelling** - Historie powstają organicznie z interakcji systemów (jak w Dwarf Fortress)
- **Cyberpunk-Fantasy Fusion** - Estetyka neonowych osad obok magicznych ruin

### Fundamenty Techniczne (Już Masz!)
✅ **Solidny ECS Architecture** - Łatwa rozbudowa o nowe komponenty  
✅ **Colony + Settler AI** - Baza do rozbudowy o frakcje  
✅ **NavigationGrid + Pathfinding** - Gotowy na multiregion  
✅ **BuildingSystem + Crafting** - Szkielet tech progression  
✅ **Resource Management** - Baza dla ekonomii handlu  

---

## 🎯 ROADMAP ROZWOJU (5 FAZ)

```mermaid
flowchart TD
    START[Obecny Stan<br/>Colony Sim + Basic AI] --> PHASE1
    
    PHASE1[FAZA 1: FOUNDATIONS<br/>3-4 miesiące]
    PHASE1 --> |World State Manager| P1A[Podział na Regiony]
    PHASE1 --> |Player Character Deep| P1B[FPS/TPS Controls]
    PHASE1 --> |Background Simulation| P1C[Passive World Ticking]
    
    P1A & P1B & P1C --> PHASE2[FAZA 2: LIVING WORLD<br/>4-5 miesięcy]
    
    PHASE2 --> |AI Factions| P2A[Autonomiczne Miasta NPC]
    PHASE2 --> |Diplomacy System| P2B[Wojny i Sojusze]
    PHASE2 --> |Dynamic Economy| P2C[Handel Między Frakcjami]
    
    P2A & P2B & P2C --> PHASE3[FAZA 3: ADVENTURE MODE<br/>3-4 miesiące]
    
    PHASE3 --> |Perspective Switch| P3A[FPS ↔ TPS ↔ RTS]
    PHASE3 --> |Quest System| P3B[Procedural Adventures]
    PHASE3 --> |Party Management| P3C[Rekrutacja Drużyny]
    
    P3A & P3B & P3C --> PHASE4[FAZA 4: CYBERPUNK FUSION<br/>5-6 miesięcy]
    
    PHASE4 --> |Tech Tree| P4A[Electronics + Magic]
    PHASE4 --> |Aesthetic Overhaul| P4B[Neon-Lit World]
    PHASE4 --> |Advanced Crafting| P4C[Implants + Enchantments]
    
    P4A & P4B & P4C --> PHASE5[FAZA 5: MASTERPIECE POLISH<br/>2-3 miesiące]
    
    PHASE5 --> END[Living World Sandbox<br/>MASTERPIECE 9.5]
    
    style START fill:#2a2a2a,stroke:#00ff88,stroke-width:2px,color:#fff
    style END fill:#ff00ff,stroke:#00ffff,stroke-width:4px,color:#fff
    style PHASE1 fill:#1a1a3a,stroke:#4488ff,stroke-width:2px,color:#fff
    style PHASE2 fill:#1a3a1a,stroke:#88ff44,stroke-width:2px,color:#fff
    style PHASE3 fill:#3a1a1a,stroke:#ff8844,stroke-width:2px,color:#fff
    style PHASE4 fill:#3a1a3a,stroke:#ff44ff,stroke-width:2px,color:#fff
    style PHASE5 fill:#3a3a1a,stroke:#ffff44,stroke-width:2px,color:#fff
```

---

## 🏗️ ARCHITEKTURA SYSTEMOWA - CURRENT vs TARGET

### Diagram Transformacji

```mermaid
graph LR
    subgraph CURRENT["🟢 OBECNY STAN (Colony Sim)"]
        direction TB
        CM[main.cpp<br/>Game Loop]
        CM --> COL[Colony.cpp<br/>Single Colony]
        COL --> SET[Settlers<br/>Basic AI]
        COL --> BUILD[BuildingSystem<br/>Static Structures]
        COL --> RES[Resources<br/>Local Storage]
        
        style CM fill:#1a4d1a,stroke:#00ff00,color:#fff
        style COL fill:#1a4d1a,stroke:#00ff00,color:#fff
    end
    
    subgraph TARGET["🔵 TARGET ARCHITECTURE (Living World)"]
        direction TB
        WM[WorldManager.cpp<br/>Global State]
        WM --> REG[RegionSystem<br/>Chunked World]
        WM --> FAC[FactionSystem<br/>AI Cities]
        WM --> PLAY[PlayerController<br/>FPS/TPS/RTS]
        
        REG --> |Active| ACT_REG[Active Regions<br/>Full Simulation]
        REG --> |Inactive| PASS_REG[Passive Regions<br/>Abstract Tick]
        
        FAC --> AI_CITY[AI Settlement<br/>Autonomous Growth]
        FAC --> DIPLO[DiplomacySystem<br/>Wars/Trade]
        
        PLAY --> FPS_MODE[FPS Adventure<br/>Direct Control]
        PLAY --> RTS_MODE[Colony Management<br/>Strategy View]
        
        style WM fill:#1a1a4d,stroke:#00ffff,color:#fff
        style TARGET fill:#0a0a2a,stroke:#00ffff,stroke-width:3px
    end
    
    CURRENT -.->|MIGRATION PATH| TARGET
```

---

## 🧩 KLUCZOWE NOWE SYSTEMY

### 1. **WorldManager** - Serce Żyjącego Świata

> [!IMPORTANT]
> To najbardziej krytyczny system - zarządza całym stanem świata i synchronizuje wszystkie regiony.

```mermaid
classDiagram
    class WorldManager {
        -vector~Region*~ regions
        -vector~Faction*~ factions
        -float globalTime
        -EconomyState globalEconomy
        +Update(deltaTime) void
        +GetActiveRegions() vector
        +TickPassiveRegions() void
        +SpawnFaction(type) Faction*
    }
    
    class Region {
        -Vector3 worldPosition
        -bool isActive
        -Colony* colony
        -vector~NPC*~ npcs
        -TerrainChunk terrain
        +ActivateRegion() void
        +DeactivateRegion() void
        +PassiveTick(deltaTime) void
    }
    
    class Faction {
        -string name
        -FactionType type
        -vector~Settlement*~ settlements
        -DiplomacyState relations
        -Resources treasury
        +ExpandTerritory() void
        +DecideAction() FactionAction
        +TradeWith(Faction*) void
    }
    
    class PassiveSimulation {
        -AbstractEconomy economy
        -PopulationGrowth growth
        +SimulateGrowth(deltaTime) void
        +GenerateEvents() vector
    }
    
    WorldManager "1" --> "*" Region
    WorldManager "1" --> "*" Faction
    Region "1" --> "1" PassiveSimulation
    Faction "1" --> "*" Region
    
    style WorldManager fill:#2a2a4a,stroke:#00ffff,stroke-width:3px,color:#fff
    style Region fill:#1a3a1a,stroke:#88ff44,color:#fff
    style Faction fill:#3a1a1a,stroke:#ff8844,color:#fff
```

**Uzasadnienie Techniczne:**
- **Region System**: Podział świata na chunki (np. 100×100m) dla optymalizacji
- **Active vs Passive**: Tylko regiony w zasięgu gracza są w pełni symulowane
- **Passive Ticking**: Miasta AI używają abstrakcyjnych kalkulacji (wzrost populacji, produkcja) zamiast pełnej symulacji jednostek

---

### 2. **AI Faction System** - Autonomiczne Społeczeństwa

```mermaid
stateDiagram-v2
    [*] --> Nomadic: Faction Created
    
    Nomadic --> Settling: Find Good Location
    Settling --> Established: Build First Structures
    
    Established --> Expanding: Population > 20
    Established --> Trading: Discover Other Faction
    Established --> Defending: Under Attack
    
    Expanding --> Established: Build New Outpost
    Expanding --> Warring: Territory Conflict
    
    Trading --> Allied: Positive Relations > 75
    Trading --> Hostile: Negative Relations < 25
    
    Defending --> Established: Threat Eliminated
    Defending --> Destroyed: All Settlers Dead
    
    Warring --> Victorious: Enemy Defeated
    Warring --> Destroyed: Defeat
    
    Victorious --> Expanding
    Allied --> Trading
    Hostile --> Warring
    
    Destroyed --> [*]
    
    note right of Established
        Core State:
        - Build infrastructure
        - Gather resources
        - Train settlers
    end note
    
    note right of Trading
        Economic Interactions:
        - Send caravans
        - Exchange goods
        - Tech sharing
    end note
```

**Kluczowe Mechaniki:**
- **Autonomous Decision Making**: AI frakcje działają niezależnie (budują, handlują, walczą)
- **Emergent Diplomacy**: Relacje kształtują się dynamicznie (sojusze, wojny, handel)
- **Cultural Identity**: Każda frakcja ma unique tech tree i estetykę

---

### 3. **Player Character Trinity** - Trzy Tryby Gry

```mermaid
flowchart LR
    subgraph MODES["🎮 Tryby Gry"]
        direction TB
        FPS[FPS Adventure Mode<br/>🎯 Direct Control]
        TPS[TPS Adventure Mode<br/>👁️ Third Person]
        RTS[RTS Management Mode<br/>🗺️ Strategy View]
    end
    
    FPS -->|Press V| TPS
    TPS -->|Press V| FPS
    FPS -->|Press TAB| RTS
    TPS -->|Press TAB| RTS
    RTS -->|Press TAB| FPS
    
    FPS --> FPS_ACT[- Combat System<br/>- Exploration<br/>- Quests<br/>- Loot Dungeons]
    TPS --> TPS_ACT[- Party View<br/>- Followers Management<br/>- Tactical Combat]
    RTS --> RTS_ACT[- Colony Orders<br/>- Building Placement<br/>- Resource Management<br/>- Diplomacy Screen]
    
    style FPS fill:#3a1a1a,stroke:#ff4444,stroke-width:3px,color:#fff
    style TPS fill:#1a3a1a,stroke:#44ff44,stroke-width:3px,color:#fff
    style RTS fill:#1a1a3a,stroke:#4444ff,stroke-width:3px,color:#fff
```

**Implementacja Techniczna:**
- **Camera System Refactor**: Jeden `CameraController` z trzema stanami
- **Input Context Switching**: Różne bindy klawiszy dla każdego trybu
- **UI Adaptation**: UI dynamicznie pokazuje relevantne informacje dla trybu

---

### 4. **Cyberpunk-Fantasy Tech Tree**

```mermaid
graph TD
    START[Początek Gry<br/>Primitive Tools]
    
    START --> BRANCH_A[🔧 TECH PATH<br/>Cyberpunk Branch]
    START --> BRANCH_B[✨ MAGIC PATH<br/>Fantasy Branch]
    
    BRANCH_A --> TECH1[Electronics Tier<br/>- Solar Panels<br/>- Radio<br/>- Turrets]
    TECH1 --> TECH2[Cybernetics Tier<br/>- Neural Implants<br/>- Prosthetics<br/>- Drones]
    TECH2 --> TECH3[Advanced Tech<br/>- AI Cores<br/>- Teleporters<br/>- Energy Weapons]
    
    BRANCH_B --> MAGIC1[Rune Tier<br/>- Fire Runes<br/>- Healing Circles<br/>- Ward Stones]
    MAGIC1 --> MAGIC2[Enchantment Tier<br/>- Weapon Imbue<br/>- Armor Runes<br/>- Portal Stones]
    MAGIC2 --> MAGIC3[High Magic<br/>- Golems<br/>- Mass Teleport<br/>- Reality Bending]
    
    TECH2 -.->|Hybrid Research| HYBRID[FUSION TECH<br/>- Mana Batteries<br/>- Rune-Powered Guns<br/>- Cyber-Wizards]
    MAGIC2 -.->|Hybrid Research| HYBRID
    
    style START fill:#2a2a2a,stroke:#ffffff,stroke-width:2px,color:#fff
    style BRANCH_A fill:#1a1a3a,stroke:#00ffff,stroke-width:2px,color:#fff
    style BRANCH_B fill:#3a1a3a,stroke:#ff00ff,stroke-width:2px,color:#fff
    style HYBRID fill:#3a3a1a,stroke:#ffff00,stroke-width:3px,color:#fff
    style TECH3 fill:#004444,stroke:#00ffff,stroke-width:2px,color:#fff
    style MAGIC3 fill:#440044,stroke:#ff00ff,stroke-width:2px,color:#fff
```

---

## 🎨 WIZUALIZACJA KONCEPTOWA (Creative Juice)

### Klimat: Neon-Fantasy Settlements

**Przykładowe Scene:**
1. **Osada gracza o zmierzchu**: Drewniane palisady z neonowymi paskami LED, ognisko pośrodku, hologramy pokazują mapę okolicy
2. **AI City - Cyberpunk Quarter**: Wieżowce z blachy i drewna, neony reklamowe, parujące kratki, NPC handlują na bazarze
3. **Magiczny Dungeon**: Starożytne ruiny świecące runami, levitujące platformy, krystaliczne formacje

*(Wygeneruję te wizualizacje za chwilę)*

---

## 🔀 ŚCIEŻKI IMPLEMENTACJI - BEZPIECZNA vs MASTERPIECE

### Opcja A: **BEZPIECZNA** (Ewolucyjna) - 12-18 miesięcy

> [!NOTE]
> Minimalizuje ryzyko techniczne, zachowuje backward compatibility.

**Strategia:**
1. Dodawaj systemy incremental (najpierw RegionSystem, potem Factions)
2. Utrzymuj obecny Colony.cpp jako legacy fallback
3. Każda faza ma working prototype

**Pros:**
✅ Niskie ryzyko broke existing features  
✅ Możliwość publikowania wczesnych wersji  
✅ Łatwiejsze debugowanie (jedna zmiana na raz)  

**Cons:**
❌ Wolniejsze tempo  
❌ Technicznym debt (legacy code pozostaje)  
❌ Mniej spójny design (łatki zamiast redesign)

---

### Opcja B: **MASTERPIECE AAA** (Rewolucyjna) - 18-24 miesiące

> [!WARNING]
> Wymaga pełnego refactoru core architecture. Wysokie ryzyko, ale maksymalna jakość.

**Strategia:**
1. **Krok 1-3 miesiące**: Prototyp WorldManager + RegionSystem (nowy branch)
2. **Krok 2-4 miesiące**: Migracja Colony → Region + Faction systems
3. **Krok 3-6 miesięcy**: Player Character trinity + FPS combat
4. **Krok 4-5 miesięcy**: Cyberpunk-Fantasy aesthetic overhaul
5. **Krok 5-3 miesiące**: Polish + balancing

**Pros:**
✅ Czysta architektura od podstaw  
✅ Lepsze fundamenty dla przyszłych feature'ów  
✅ Prawdziwe "Living World" (nie symulacja)  

**Cons:**
❌ Długi okres bez playable build  
❌ Wysokie ryzyko scope creep  
❌ Trudniejszy rollback jeśli coś pójdzie nie tak  

---

## 🏆 REKOMENDACJA [ARCHITEKT]

### Hybrydowa Ścieżka: **"Modular Masterpiece"**

> [!IMPORTANT]
> Łączę bezpieczeństwo Opcji A z ambicją Opcji B.

**Strategia 3-torowa:**

```mermaid
gantt
    title Hybrydowy Roadmap (24 miesiące)
    dateFormat YYYY-MM
    section Foundations
    WorldManager Prototype    :a1, 2026-01, 3M
    Region System Core        :a2, after a1, 2M
    section Living World
    AI Factions v1 (Basic)    :b1, 2026-06, 3M
    Passive Simulation        :b2, after b1, 2M
    Diplomacy System          :b3, after b2, 2M
    section Player Mode
    FPS Controls              :c1, 2026-06, 2M
    TPS + Camera Switch       :c2, after c1, 2M
    Quest System Prototype    :c3, after c2, 3M
    section Cyberpunk
    Tech Tree Design          :d1, 2026-11, 2M
    Aesthetic Overhaul        :d2, after d1, 4M
    Advanced Crafting         :d3, after d2, 2M
    section Polish
    Balancing & Events        :e1, 2027-07, 2M
    Performance Optimization  :e2, after e1, 2M
    Final Masterpiece Release :milestone, 2027-11, 0d
```

**Kluczowe Założenia:**
1. **Równoległy Development**: Systemy WorldManager i Player Character rozwijane jednocześnie
2. **Vertical Slices**: Co 3 miesiące masz playable vertical slice (np. "1 AI faction + FPS mode działa")
3. **Fallback Points**: Jeśli Factions okaże się za trudne, fallback to "better Colony AI"

---

## ❓ PYTANIA BLOKUJĄCE DLA PARTNERA

> [!CAUTION]
> Te decyzje muszą być podjęte **przed** rozpoczęciem implementacji.

### 1. **Scope & Timeline**
- **Q1**: Jaki masz horyzont czasowy? (6M / 12M / 24M+)
- **Q2**: Czy to projekt solo, czy planujesz team expansion?
- **Q3**: Priorytet: Szybki MVP vs Długoterminowy Masterpiece?

### 2. **Techniczna Wizja**
- **Q4**: Akceptujesz pełny refactor core systems (WorldManager zamiast Colony)?
- **Q5**: Czy zgadzasz się na **porzucenie backward compatibility** z obecnym save system?
- **Q6**: Maksymalny rozmiar świata: 1km² / 10km² / unlimited procedural?

### 3. **Gameplay Focus**
- **Q7**: Co jest core loop: Adventure (FPS) czy Management (RTS)?
- **Q8**: Multiplayer w planach? (Drastycznie zmieni architekturę)
- **Q9**: Preferujesz więcej Dwarf Fortress (complexity) czy Rimworld (accessibility)?

### 4. **Estetyka**
- **Q10**: Cyberpunk:Fantasy ratio? (50:50 / 70:30 / choose at start?)
- **Q11**: 3D models: Low-poly stylized czy realistic?
- **Q12**: Voice acting / Text-only / Mixed?

---

## 📊 RISK ASSESSMENT

| Ryzyko | Prawdopodobieństwo | Impact | Mitigation |
|--------|-------------------|--------|------------|
| **Scope Creep** | 🔴 Wysokie | 🔴 Krytyczny | Locked milestones, vertical slices |
| **Performance (World Size)** | 🟡 Średnie | 🔴 Krytyczny | Spatial partitioning, LOD system |
| **AI Complexity** | 🟡 Średnie | 🟡 Średnie | Start z prostymi state machines |
| **Legacy Code Debt** | 🟢 Niskie | 🟡 Średnie | Refactor w fazie 1 |
| **Art Asset Volume** | 🔴 Wysokie | 🟡 Średnie | Procedural generation + asset packs |

---

## 🚀 NATYCHMIASTOWE NEXT STEPS (Po Aprobacie)

1. **[ARCHITEKT]** Stworzyć `WorldManager.h` prototype
2. **[PROJEKTANT]** Wygenerować concept art dla 3 scene types
3. **[STRATEG]** Zaprojektować Region serialization format
4. **[DEBUGGER]** Audit obecnego Colony.cpp (co można reuse?)
5. **[REŻYSER]** Napisać Game Design Document dla FPS combat feel

---

## 📸 VISUAL KONCEPTS - Klimat Gry

### 🌆 Scene 1: Neon Settlement at Dusk
*Osada gracza o zmierzchu - połączenie średniowiecznej konstrukcji z cyberpunk estetyką*

![Neonowa osada o zmierzchu](C:/Users/maksk/.gemini/antigravity/brain/0dd4d2c5-5b71-46e9-b5f2-6c526a1f094b/neon_settlement_dusk_1767457985121.png)

**Design Notes:**
- Drewniane palisady z wbudowanymi neonowymi paskami LED (cyan/magenta)
- Holograficzne projekcje map i danych
- Kontrast ciepłego ognia z zimnymi akcentami neonowymi
- Solar panele na prymitywnych budynkach

---

### 🏙️ Scene 2: AI City Marketplace
*Autonomiczne miasto NPC - tętniący życiem bazar cyberpunk*

![Targ w mieście AI](C:/Users/maksk/.gemini/antigravity/brain/0dd4d2c5-5b71-46e9-b5f2-6c526a1f094b/ai_city_marketplace_1767458000172.png)

**Design Notes:**
- Budynki z łączonych materiałów (metal + drewno + beton)
- NPC z cybernetic implants i fantasy robes
- Holograficzne reklamy i neony w obcych językach
- Living, breathing atmosphere - miasto żyje niezależnie od gracza

---

### ⚔️ Scene 3: Magical Dungeon Ruins
*FPS Adventure Mode - eksploracja magicznych ruin*

![Magiczne ruiny dungeonu](C:/Users/maksk/.gemini/antigravity/brain/0dd4d2c5-5b71-46e9-b5f2-6c526a1f094b/magic_dungeon_ruins_1767458015219.png)

**Design Notes:**
- Pierwszoosobowa perspektywa dla immersion
- Świecące runy i krystaliczne formacje
- Levitujące platformy (puzzle elements)
- Atmospheric lighting - dramatyczne promienie światła
- Particle effects (mgła, iskry magii, pył)

---

### 🌳 Scene 4: Tech Tree Interface
*Dual-path progression system - Tech vs Magic*

![Wizualizacja tech tree](C:/Users/maksk/.gemini/antigravity/brain/0dd4d2c5-5b71-46e9-b5f2-6c526a1f094b/tech_tree_visualization_1767458032225.png)

**Design Notes:**
- Cyberpunk branch (niebieski) vs Fantasy branch (fioletowy)
- Centralny Fusion path (złoty) dla hybrydowych technologii
- Holograficzny UI z ikonami
- Czysty, czytelny design dla szybkiej nawigacji

---

### 🤝 Scene 5: Faction Diplomacy Screen
*RTS Management Mode - zarządzanie relacjami między frakcjami*

![Ekran dyplomacji frakcji](C:/Users/maksk/.gemini/antigravity/brain/0dd4d2c5-5b71-46e9-b5f2-6c526a1f094b/faction_diplomacy_ui_1767458050280.png)

**Design Notes:**
- Centralny diagram relacji (kolor = status relacji)
- Panel info frakcji z portretem i statystykami
- Akcje dyplomatyczne (Trade, War, Alliance)
- Neon accents (cyan, magenta, gold) na dark background
- Premium, AAA-quality UI design

---

## 🎯 KOŃCOWE SŁOWO [ARCHITEKT]

Partnerze, masz **solidne fundamenty** w postaci działającego ECS i nawigacji. To nie jest "start from scratch" - to **evolution**.

**Moja rekomendacja:** Zacznij od **Fazy 1 (Foundations)** z opcją **Modular Masterpiece**. W 3 miesiące masz:
- ✅ WorldManager działający
- ✅ FPS controls functional
- ✅ Pierwszy prototyp AI faction

A potem decydujemy: push dalej czy pivot.

**To będzie Arcydzieło. Ale potrzebuję Twoich odpowiedzi na pytania blokujące.**

---

*Dokument stworzony przez: Antigravity AI [ARCHITEKT MODE]*  

## 🤖 AI Faction Prototype Implementation Plan (Phase 0 - Week 2)

**Cel**: Stworzenie pierwszej autonomicznej frakcji NPC, która "żyje" w tle (simulated in passive mode).

### 1. Nowe Klasy
*   **`game/Faction.h/cpp`**:
    *   Klasa bazowa dla frakcji.
    *   Zarządza zasobami (Treasury) i listą osad.
    *   `Update(deltaTime)`: Główna pętla decyzyjna (prosty behawioryzm: zbierz surowce -> zbuduj).
*   **`game/Settlement.h/cpp`**:
    *   Reprezentuje miasto/osadę NPC.
    *   Posiada `name`, `population`, `buildings`.
    *   `PassiveTick(deltaTime)`: Symuluje wzrost i produkcję.
    *   **Auto-Expanion**: Co 30s próbuje postawić budynek (w logicznym miejscu).

### 2. Integracja z WorldManager
*   `WorldManager` będzie posiadał listę `std::vector<std::unique_ptr<Faction>>`.
*   W `WorldManager::Update`: wywołanie `faction->Update(dt)` dla wszystkich frakcji.

### 3. Wizualizacja (Debug)
*   Rozszerzenie `WorldManager::DrawDebugInfo()` o wyświetlanie statusu frakcji (np. "Faction: Empire | Gold: 100 | Settlements: 1").
*   Opcjonalnie: Rysowanie prostych ikon/markerów w miejscu osad NPC na gridzie.

### 4. Scope (Prototype Constraints)
*   **Brak fizycznych NPC**: Frakcja to tylko "liczby" i budynki (Phase 2 doda fizyczne jednostki).
*   **Brak dyplomacji**: Frakcje na razie ignorują się nawzajem.
*   **Hardcoded Behavior**: Zamiast pełnego drzewa decyzji, prosta maszyna stanów (`GATHER` -> `BUILD`).
