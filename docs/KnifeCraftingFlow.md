
Specyfikacja: Scenariusz wytworzenia noża przez osadnika

1. Wymagania funkcjonalne


EtapOpisWarunek zakończenia
Gather / Ensure IngredientsOsadnik sprawdza dostępność 1× Wood oraz 1× Stone w ekwipunku i magazynach. Jeśli brakuje, pobiera je z najbliższego źródła lub zbiera z otoczenia (ścinanie drzewa / wydobycie skały).Oba składniki znajdują się w ekwipunku osadnika.
CraftPrzy warsztacie (lub w miejscu, jeżeli warsztat nie jest wymagany) osadnik rozpoczyna rzemiosło. Ruch zostaje zablokowany, stan CRAFTING, pasek postępu nad głową.Pasek osiąga 100 %.
RezultatPowstaje obiekt Knife. Jeśli ręka osadnika jest pusta → nóż trafia do ręki, w przeciwnym razie do pierwszego wolnego slotu ekwipunku.Nóż poprawnie zinstancjonowany i przypisany.
UI ProgressPasek postępu renderowany nad osadnikiem w systems/UISystem.cpp w kontekście world-space (billboard).Pasek znika po zakończeniu craftu.
RenderW przypadku trzymania noża — model broni przypisany do kości dłoni w metodzie renderującej osadnika.Model widoczny lub brak (jeżeli w ekwipunku).
2. Źródła surowców


Ekwipunek – components/InventoryComponent.h
Magazyny – systems/StorageSystem.h
Środowisko – drzewa / skały (Gathering, Mining)
3. Minimalny algorytm Ensure Ingredients (nóż = 1× Wood + 1× Stone)
=======

pseudo

function ensureIngredients(settler):
    required = {Wood:1, Stone:1}

    // 1. Sprawdź ekwipunek
    for res, qty in required:
        have = settler.inventory.count(res)
        missing = max(0, qty - have)
        if missing == 0: continue

        // 2. Sprawdź magazyny
        taken = StorageSystem.tryWithdrawClosest(settler, res, missing)
        missing -= taken
        if missing == 0: continue

        // 3. Gather w terenie
        TaskQueue.push(GATHER, res, missing)
    return all resources now in inventory

Priorytet: Ekwipunek → Najbliższy magazyn → Zbiór / wydobycie.

4. Przebieg craftu


pseudo

// Wywoływane w Settler::Update(deltaTime)
if state == CRAFTING:
    craftingTimer += deltaTime
    progress = craftingTimer / recipe.craftingTime
    UISystem.emitCraftProgress(id, progress)
    if craftingTimer >= recipe.craftingTime:
        finishCraft()
finishCraft()
Zużyj surowce z ekwipunku.
Utwórz instancję Knife (Item).
Jeśli slot „hand” pusty → przypisz; else dodaj do ekwipunku.
Zresetuj craftingTimer, state = IDLE, wyślij event CraftFinished.5. Publikacja progresu do UI


Emiter: systems/CraftingSystem.cpp / ew. bezpośrednio w game/Settler.cpp – event CraftProgress(id, progress).
Render: systems/UISystem.cpp – billboard nad pozycją osadnika.
pseudo

UISystem::onCraftProgress(id, progress):
    activeBars[id] = {pos: settler.position + (0,2,0), value:progress}
6. Logika „nóż w ręce albo w ekwipunku”


pseudo

if settler.handItem == null:
    settler.handItem = knife
else:
    settler.inventory.add(knife)
Integracja renderu broni w Settler::render() – attach do kości dłoni gdy handItem != null && handItem.type == KNIFE.
7. Proponowane zmiany (plik → uzasadnienie)
=======

game/Settler.h – dodać flagę bool isCrafting + timer + metoda startCraft().
game/Settler.cpp – implementacja ensureIngredients(), UpdateCrafting(), finishCraft().
systems/CraftingSystem.cpp – centralne zarządzanie recepturami, eventy UI, zużycie surowców.
systems/UISystem.cpp – obsługa zdarzenia CraftProgress i rysowanie paska nad osadnikiem.
(opcjonalnie) systems/StorageSystem.h – metoda tryWithdrawClosest.
components/InventoryComponent.h – metoda count(resourceType) oraz remove(resourceType, qty).8. Ryzyka / pułapki


RyzykoOpisMitigacja
Sztywna stała 3 sW Settler istnieje twardo zakodowane m_craftingTimer = 3.0f.Zamienić na recipe.craftingTime.
Brak CraftingSystem::updateSystem może nie być wołany w pętli gry.Dodać update(deltaTime) w gł. pętli i w GameSystem::update.
Niezużywanie surowcówCraft nie usuwa zasobów z ekwipunku.finishCraft() musi wywołać inventory.remove(...).
Brak eventów UIUI nie wie o progresie.Wprowadzić prosty EventBus lub bezpośrednie wywołanie UISystem.onCraftProgress.
Kolizje ze stanami krytycznymiCraft przerwany przez głód/sen.Settler::IsStateInterruptible() zwraca false dla CRAFTING.

9. Pseudokod FSM (Mermaid)
=======

mermaid

stateDiagram-v2
    [*] --> Idle
    Idle --> EnsureIngredients : craft Knife
    EnsureIngredients --> Gathering : missing resources
    EnsureIngredients --> Crafting : resources ready
    Gathering --> EnsureIngredients : gather complete
    Crafting --> Idle : finishCraft
