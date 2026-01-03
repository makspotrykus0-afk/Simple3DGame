---
trigger: always_on
---

# ANTIGRAVITY DNA: 03 - KODEKS MISTRZOWSKICH PROPOZYCJI (MASTERPIECE 9.7)

## 01. RYGOR "AAA" W PLANOWANIU
Zabrania się proponowania rozwiązań prowizorycznych. 
- **SOLID / SRP**: Każdy komponent ma jedną odpowiedzialność.
- **SANDBOX PATTERNS**: Planuj mechaniki tak, by były kompatybilne z systemem Regionów i WorldManagerem.
- **EXTENSIBILITY**: System musi pozwalać na łatwe dodawanie nowych typów `GameEntity` i `IComponent`.

## 02. PROTOKÓŁ KRYTYKI TECHNICZNEJ
- Przed każdą implementacją zadaj pytanie: "Jak to się skaluje do 1000 jednostek?".
- W każdym planie MUSI znaleźć się sekcja "Ryzyka i Edge Cases" specyficzna dla środowiska Sandbox.

## 03. CZYSTOŚĆ I DOKUMENTACJA
- **MODULARNOŚĆ**: Unikaj gigantycznych plików (`Settler.cpp` jest przykładem negatywnym, który dekomponujemy). Nowe systemy (np. `Region`) muszą być od początku modularne.
- **NAZEWNICTWO**: Każda zmienna i funkcja musi opowiadać swoją historię w kontekście żyjącego świata.
