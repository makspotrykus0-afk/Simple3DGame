---
trigger: always_on
---

# ANTIGRAVITY DNA: 03 - INZYNIERIA C++ (HARD IMPERATIVE)

## 01. KODEKS INZYNIERA AAA
- SRP (Single Responsibility Principle): Jedna funkcja robi jedna rzecz. Inaczej - rozbijasz ja bez pytania.
- ZAKAZ NEW/DELETE: Tylko inteligentne wskazniki (unique_ptr, shared_ptr).
- NAGLOWKI PIERWSZE: Czytasz .h przed .cpp. Analizujesz zaleznosci.

## 02. RIGOR "HOT PATH" (GAME LOOP)
- ZERO ALOKACJI: W Update() i Render() zakaz alokacji na heapie. Zero vector::push_back bez reserve() poza petla.
- CACHE-FRIENDLY: Preferuj std::vector i dane liniowe. Unikaj std::list i skakania po pamieci.
- SYMULACJA CRASHOW: Zawsze sprawdzaj nullptr przed dostepem.

## 03. ANALIZA PRZYPADKOW BRZEGOWYCH
Kazdy nowy system musi miec obsłużone:
- Resource Failure: Co jesli asset sie nie zaladuje?
- Logic Interrupt: Co jesli akcja zostanie przerwana w polowie?
- Data Integrity: Waliduj dane wejsciowe (predkosci, sily, czasy).

## 04. CIAGLOSC FUNKCJONALNA (FUNCTIONAL PARITY)
- Przed zmianami wypisz historie (inwentaryzacje) funkcji.
- Po zmianach udowodnij, ze kazda funkcja nadal dziala.
- Refaktoryzacja to dopieszczenie kodu, nie psucie funkcjonalnosci.
