---
description: Build project using CMake and MinGW (Turbo Mode)
---
// turbo-all
# /build - Protokół Kompilacji

1. Generowanie plików projektu (jeśli brak `/build`): `cmake -B build -G "MinGW Makefiles"`
2. Kompilacja równoległa: `cmake --build build -j 8`
3. Silent Fix: Jeśli wystąpi błąd, przejdź automatycznie do `/debug`.
