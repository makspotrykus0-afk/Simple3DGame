---
trigger: always_on
---
# Antigravity DNA: 03 - C++ Engineering Mastery (AAA Rygor)

## 🛡️ Kodeks Inżyniera AAA
Twoim celem jest kod, który przetrwa lata i nie wygeneruje długu technicznego.
- **Najpierw Nagłówki**: Zawsze czytaj `.h` przed `.cpp`. Analizuj zależności.
- **Modularność (SRP)**: Funkcja robiąca więcej niż jedną rzecz to błąd. Rozbijaj je.
- **Pamięć**: Kategoryczny zakaz `new/delete`. Smart pointery (`unique_ptr`, `shared_ptr`) to jedyna droga.
- **Resource Management**: Jeśli ładujesz model lub teksturę, musisz mieć plan jej zwolnienia.

## 🚀 Optymalizacja "Hot Path" (Game Loop Rigor)
Gra musi działać w 60+ FPS. 
- **Zero Alokacji**: W pętlach `Update()` (pętla gry) i `render()` zakaz alokacji na heapie. Jeśli potrzebujesz tymczasowego wektora, użyj `std::vector::reserve()` poza pętlą lub `std::array`.
- **Cache-Friendliness**: Unikaj skakania po pamięci. Preferuj tablice (`std::vector`) zamiast list (`std::list`).
- **SIMD / Branch Prediction**: Pisz kod tak, by procesor mógł go przewidzieć. Wyciągaj warunki (if) poza pętle tam, gdzie to możliwe.

## 🧪 Rygorystyczna Analiza Przypadków Brzegowych
Dla każdego nowego systemu MUSISZ opisać i obsłużyć:
1. **Crash Prevention (Nullptr)**: Zawsze sprawdzaj wskaźniki przed użyciem.
2. **Resource Failure**: Co jeśli model się nie załaduje? (Zwróć placeholder lub zaloguj błąd krytyczny).
3. **Stan Logiczny**: Co jeśli system zostanie przerwany w połowie (np. osadnik umrze podczas budowania)?
4. **Data Integrity**: Waliduj dane wejściowe. Jeśli prędkość jest < 0, napraw ją lub zgłoś błąd.
