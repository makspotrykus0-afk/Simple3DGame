---
trigger: always_on
---

# ANTIGRAVITY DNA: 05 - LOG HYGIENE (MASTERPIECE 9.7)

## 01. DOGMAT "CZYSTEGO SUMIENIA LOGÓW"
Terminal nie jest brudnopisem. Jest Twoją kroniką techniczną, która musi być czytelna i konkretna.
- **ZAKAZ SPAMU**: Usuwaj z logów wszelkie powtarzalne, mało znaczące komunikaty (np. "Moving...").
- **WAGA INFORMACJI**: Każdy log w konsoli musi mieć realną wartość dla diagnozy lub potwierdzenia sukcesu.
- **NARRACJA TECHNICZNA**: Logi powinny opowiadać historię operacji w systemie Sandbox (np. `[WorldManager] Region (2,3) loaded` -> `[Settler] Assigned to Region (2,3)` -> `[Settler] Resource found`).

## 02. SILENT LOG FIX [BALANCER RIGOR]
- Jeśli zauważysz, że logi spowalniają pętlę Update przy 100+ jednostkach, masz OBOWIĄZEK je uciszyć lub przenieść do trybu Debug/F1.
- Logi wydajnościowe (frame time) powinny pojawiać się tylko w fazie optymalizacji pod nadzorem roli **[BALANCER]**.

## 03. FORMATOWANIE AAA
- Używaj bracketów dla systemów: `[SystemName] Message`.
- Używaj jasnej klasyfikacji: `ERROR`, `WARNING`, `SUCCESS`.
- **ZASADA 9.7**: Loguj kluczowe decyzje emergentnego AI (np. `[Settler] Decision changed: HUNGER > CHOPPING`).
