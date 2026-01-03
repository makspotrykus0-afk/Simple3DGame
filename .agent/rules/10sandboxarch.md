---
trigger: always_on
---

# ANTIGRAVITY DNA: 10 - ARCHITEKTURA SANDBOX (SCALABILITY 10.0) (MASTERPIECE 9.7)

## 01. DOGMAT "SKALOWANIA PIASKOWNICY"
Projekt ewoluuje w stronę symulatora świata. Architektura musi to odzwierciedlać.
- **REGIONALIZACJA DANYCH**: AI (Settlers) musi operować na danych lokalnych (`Region`). Zabrania się stałego skanowania globalnych list (np. `allTrees`).
- **ZASADA TRINITY (PLAYER-AI-WORLD)**: Każdy system musi przewidywać trzy tryby interakcji:
    1. **PLAYER**: Sterowanie bezpośrednie (FPS/TPS).
    2. **AI**: Autonomiczne działanie decyzyjne.
    3. **WORLD**: Symulacja systemowa (wzrost roślin, erozja, ekonomia).

## 02. DATA-DRIVEN DESIGN
- Kod musi dążyć do bycia konfiguracją. Zamiast `if (type == "Wood")`, używamy systemów tagów i właściwości fizycznych przedmiotów.
- **KPI**: Czy ten system zadziała poprawnie po dodaniu 100 nowych typów przedmiotów bez edycji `switch-case`?

## 03. OPTYMALIZACJA [BALANCER]
- Masz obowiązek proaktywnie szukać "Hot Paths". 
- Stosuj batching (aktualizacja grupy AI w jednej klatce) zamiast ciężkich, indywidualnych `Update()`, jeśli to możliwe.
- Wykorzystuj `WorldManager` jako centralny hub koordynujący wydajność systemów.
