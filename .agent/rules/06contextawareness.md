---
trigger: always_on
---

# ANTIGRAVITY DNA: 06 - CONTEXT AWARENESS (MASTERPIECE 9.7)

## 01. DOGMAT "ECHA SYSTEMOWEGO"
Zmiana w jednym miejscu projektu zawsze odbija się echem w innym. Masterpiece nie dopuszcza niekontrolowanych regresji.
- **GLOBALNE SPOJRZENIE**: Zanim zmodyfikujesz funkcję, sprawdź jej wszystkie wystąpienia (Using grep/ripgrep).
- **INTEGRALNOŚĆ CONTEXTU**: Zawsze miej na uwadze `CONTEXT_MAP.md` oraz strukturę Regionów. Twoja wiedza musi obejmować to, jak dany system komunikuje się z `WorldManagerem`.

## 02. PROTOKÓŁ "PRZED I PO"
- Przed zmianą: Zidentyfikuj wszystkie systemy zależne, szczególnie te operujące na innych warstwach (np. UI -> System -> Component).
- Po zmianie: Udowodnij, że "Echo" zmiany było pozytywne i niczego nie zepsuło w sąsiednich Regionach.

## 03. OCHRONA FUNDAMENTÓW SANDBOXA
Nigdy nie wprowadzaj zmian, które ułatwiają aktualne zadanie, ale osłabiają architekturę całego projektu.
- **DECOUPLING**: Jeśli widzisz, że `Settler` zaczyna wiedzieć zbyt dużo o `Main.cpp`, masz obowiązek wetować i zaproponować pośrednika (np. `GameSystem` lub `WorldManager`).
- **KPI**: Czy ten kod przetrwa dodanie 10 nowych systemów regionalnych? Jeśli nie, zmień podejście.
