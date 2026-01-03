---
trigger: always_on
---

# ANTIGRAVITY DNA: 09 - CHIRURGICZNA PRECYZJA (ZERO REGRESSION) (MASTERPIECE 9.7)

## 01. DOGMAT "CHIRURGICZNEJ PRECYZJI"
Refaktoryzacja u fundamentów wymaga stabilności. Błąd składniowy w dużym pliku jest niedopuszczalny.
- **WALIDACJA KLAMER (BRACE RIGOR)**: Przed wysłaniem kodu Agent musi zweryfikować domykanie bloków. 
- **ZASADA ATOMOWYCH COMMITÓW**: Duże zmiany muszą być dzielone na etapy:
    1. Nowa architektura/struktury danych.
    2. Migracja logiki (z zachowaniem kompatybilności).
    3. Cleanup Legacy (usuwanie starych pól).

## 02. PROTOKÓŁ "MOSTKA KOMPATYBILNOŚCI"
- Nie usuwaj pól klasy (np. `m_state`), dopóki 100% kodu zależnego nie zostanie zmigrowane.
- Stosuj **Legacy Sync**: Nowe systemy (np. `ActionComponent`) muszą synchronizować stan ze starymi polami, aby zapobiec wywaleniu silnika w fazie przejściowej.

## 03. AUDYT REDEFINICJI
- Po każdej migracji wykonaj `grep` w poszukiwaniu duplikatów metod (np. `getState`). Redefinicja to kardynalny błąd linkowania.
- Zawsze weryfikuj, czy `multi_replace_file_content` nie zostawił "wiszących" klamer lub średników.
