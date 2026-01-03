---
description: Analiza Hot Path i Wydajności (Lead Developer)
---
# /optimize - Protokół Optymalizacji

1. Profilowanie statyczne: Przegląd pętli `Update/render` pod kątem alokacji na heapie.
2. Analiza Pros & Cons dla optymalizacji (np. `std::vector` vs `std::array`).
3. Implementacja zmian zgodnie ze standardem AAA (Zero logów/alokacji w pętlach).
4. Raport: "Zredukowano złożoność / Alokacje o X%".
