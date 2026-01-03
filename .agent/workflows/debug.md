---
description: Auto-Diagnostyka i Analiza Logów (Debugger Mode)
---
// turbo-all
# /debug - Protokół Auto-Diagnostyki

1. Kompilacja sprawdzająca: `cmake --build build`
2. Uruchomienie z przechwytywaniem logów: `./build/Simple3DGame.exe > debug_session.log 2>&1`
3. Analiza Ekspercka: `grep -Ei "error|warning|critical|nullptr|crash|failed|assertion" debug_session.log`
4. Proaktywna Naprawa: Agent samodzielnie implementuje fixa i powtarza zadanie.
5. Sprzątanie: `rm debug_session.log`
