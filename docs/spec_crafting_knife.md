
Specyfikacja: crafting „osadnik ma zrobić nóż do polowania”
Założenia i kontekst techniczny
Surowce w magazynie są trzymane jako enum Resources::ResourceType i ilość (int), obsługiwane przez StorageSystem.
Ekwipunek osadnika trzyma obiekty Item poprzez InventoryComponent.
Crafting ma szkielet w CraftingSystem, receptury używają CraftingIngredient z polem resourceType jako string CraftingIngredient::resourceType.
Wyposażenie jest w EquipmentComponent, a slot „ręka główna” to EquipmentItem::EquipmentSlot::MAIN_HAND.
Render osadników w aktualnym kodzie jest realizowany w implementacji Colony::render() (deklaracja w Colony).
---

1. Minimalny, deterministyczny przepływ AI „zdobądź nóż”
Cel
Osadnik ma doprowadzić do stanu: w slocie EquipmentItem::EquipmentSlot::MAIN_HAND znajduje się nóż.
Maszyna stanów
mermaid

stateDiagram-v2
    [*] --> CheckStorage
    CheckStorage --> TakeFromStorage : resources_ok
    CheckStorage --> GatherMissing : missing
    TakeFromStorage --> CraftKnife
    GatherMissing --> DepositToStorage : gathered
    DepositToStorage --> CheckStorage
    CraftKnife --> EquipKnife : crafted
    EquipKnife --> Done
    Done --> [*]

Definicja stanów i warunków przejść
Stan CheckStorage
Wejście:
Odczytaj dostępność surowców w wybranym storageId:
drewno: StorageSystem::getResourceAmount() z Resources::ResourceType::Wood
kamień: StorageSystem::getResourceAmount() z Resources::ResourceType::Stone
Warunki:
resources_ok: ilości spełniają recepturę noża (minimalnie: Wood=1, Stone=1; konkret przeniesiony do receptury).
missing: brakuje co najmniej jednego surowca.
Stan TakeFromStorage
Minimalna wersja deterministyczna:
Nie przenosimy surowców do ekwipunku jako Item.
„Pobranie” oznacza jedynie rezerwację/upewnienie się, że crafting będzie mógł odjąć zasoby ze storage:
Wersja minimalna: brak rezerwacji, tylko natychmiastowe przejście do craftingu (deterministyczne, ale bez blokady na współbieżność).
(Opcjonalnie, jeśli w przyszłości będzie wielu osadników): wprowadzić rezerwację per storageId i receptura.
Przejście:
Zawsze do CraftKnife (bo poprzedni stan zagwarantował resources_ok).
Stan GatherMissing
Wejście:
Ustal jeden brakujący typ (priorytet deterministyczny): najpierw Wood, potem Stone.
Zachowanie:
Osadnik idzie do najbliższego źródła surowca i wykonuje interakcję zbierania (logika jest częściowo w systems/InteractionSystem.h).
Warunek wyjścia:
gathered: osadnik zdobył co najmniej 1 jednostkę brakującego surowca.
Deterministyczne fallbacki:
Jeśli nie ma źródeł w świecie (brak drzew / kamieni): stan kończy się TaskFailed (mechanizm zależny od istniejącego AI), bez wchodzenia w pętlę nieskończoną.
Stan DepositToStorage
Cel: ujednolicić przepływ tak, aby crafting zawsze konsumował z tego samego miejsca.

Zachowanie:Zachowanie:
Zapisz zebrany surowiec do storageId:
StorageSystem::addResourceToStorage()
Przejście: zawsze do CheckStorage.
Stan CraftKnife
Wejście:

Dodaj zadanie craftingu dla receptury „knife”:Dodaj zadanie craftingu dla receptury „knife”:
CraftingSystem::queueTask()
Wykonanie:
W minimalnej wersji, w momencie startu craftingu od razu konsumuj składniki z storageId (patrz pkt 2).
Postęp zadania jest śledzony przez istniejący model CraftingTask (progress/started).
Warunek wyjścia:
crafted: crafting zakończony, powstał Item noża (wynik z CraftingSystem::completeTask()).
Stan EquipKnife
Wejście:
Nóż trafia do
Nóż trafia do ekwipunku osadnika:Nóż trafia do ekwipunku osadnika:
InventoryComponent::addItem()

Następnie natychmiast wyposaż:Następnie natychmiast wyposaż:
EquipmentComponent::equipItem() do slotu EquipmentItem::EquipmentSlot::MAIN_HAND
Warunek wyjścia:
Po udanym equip → Done.
Jeśli slot zajęty: deterministyczna zasada minimalna: zdejmij stary przedmiot i włóż do inventory (operacja przez EquipmentComponent::unequipItem()), potem equip noża.
Stan Done

Invariant:Invariant:
EquipmentComponent::isSlotOccupied() dla MAIN_HAND = true
EquipmentComponent::getItemInSlot() wskazuje na nóż
---

2. Jak spiąć Storage z Crafting/Inventory
Wybór: wariant A (minimalny do wdrożenia)
Crafting konsumuje zasoby bezpośrednio z StorageSystem jako Resources::ResourceType, a wynik tworzy Item noża i dodaje do inventory.
Powód wyboru
StorageSystem ma już komplet podstawowych operacji:
odczyt: StorageSystem::getResourceAmount()
usuwanie: StorageSystem::removeResourceFromStorage()
dodawanie: StorageSystem::addResourceToStorage()
Unikamy „adaptera” zasoby→Item i sztucznego przenoszenia surowców do InventoryComponent.
Konsekwencje i wymagane dopięcia w Crafting
**Ujednolicenie nazewnictwa składników