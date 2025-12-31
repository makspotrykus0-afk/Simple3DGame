#pragma once

#include "raymath.h" // Używamy Vector3 zdefiniowanego w raymath.h

// Definicja struktury BoxCollider
class BoxCollider {
public:
    // Use Vector3 from raylib.h, ensure it's included before raymath.h
    Vector3 center;
    Vector3 size;

    // Konstruktor domyślny z inicjalizacją składowych
    BoxCollider()
    {
        center.x = 0.0f; center.y = 0.0f; center.z = 0.0f;
        size.x = 1.0f; size.y = 1.0f; size.z = 1.0f;
    }

    // Konstruktor z parametrami
    BoxCollider(Vector3 c, Vector3 s) : center(c), size(s) {}
};