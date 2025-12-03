#include "colony.h"
#include <iostream>
#include <vector>
#include "raylib.h" // Dołączamy raylib.h, aby uzyskać dostęp do typów i funkcji

// Jeśli Settler.h istnieje i zawiera definicję Settler, dołącz go.
// W przeciwnym razie, zakładamy forward declaration jest wystarczająca lub Settler jest zdefiniowany gdzie indziej.
// #include "Settler.h" 

// Forward declaration dla Settler, jeśli Settler.h nie jest dostępne lub nie zostało poprawnie uwzględnione
class Settler; 

Colony::Colony() {}

Colony::~Colony() {
    for (Settler* settler : settlers) {
        delete settler;
    }
    settlers.clear();
}

void Colony::initialize() {
    // Raylib handles the rendering automatically
    // Dodanie początkowych osadników dla testów
    addSettler({0.0f, 0.0f, 0.0f});
    addSettler({5.0f, 0.0f, 0.0f});
}

void Colony::addSettler(Vector3 position) {
    // Zakładając, że Settler ma konstruktor przyjmujący pozycję i inne parametry
    // Używamy przykładowych wartości dla rotation i scale
    settlers.push_back(new Settler(position, Vector3{1.0f, 0.0f, 0.0f}, 1.0f)); // Pomarańczowi osadnicy
}

void Colony::update(float deltaTime) {
    for (Settler* settler : settlers) {
        settler->Update(deltaTime);
    }
}

void Colony::render() {
    // Draw each settler as a cube
    for (Settler* settler : settlers) {
        // Używamy zdefiniowanych kolorów z raylib
        Color settlerColor = settler->IsSelected() ? YELLOW : ORANGE; 
        
        // Draw cube - zakładając, że DrawCube jest dostępne z raylib
        DrawCube(settler->position, 1.0f, 1.0f, 1.0f, settlerColor);
        
        // Draw selection indicator
        if (settler->IsSelected()) {
            // Używamy Vector3{0, 1, 0} jako osi obrotu dla DrawCircle3D
            // Zakładamy, że DrawCircle3D jest dostępne z raylib.h
            DrawCircle3D(settler->position, 1.5f, Vector3{0.0f, 1.0f, 0.0f}, 0.0f, Color{255, 255, 0, 128}); // Półprzezroczysty żółty
        }
        
        // Draw movement target
        if (settler->IsMoving()) {
            // Używamy przykładowego koloru dla celu ruchu
            DrawSphere(settler->target, 0.3f, SKYBLUE); 
        }
        
        // Draw a small marker above each settler
        DrawSphere(Vector3{settler->position.x, settler->position.y + 1.5f, settler->position.z}, 
                   0.2f, YELLOW); // Żółta sfera nad osadnikiem
    }
}

Settler* Colony::getSettlerAt(Vector3 position, float radius) {
    for (Settler* settler : settlers) {
        // Poprawione wywołanie Vector3Subtract i Vector3Length
        // Tworzymy tymczasową zmienną, aby uniknąć potencjalnych problemów z konwersją
        Vector3 diff = Vector3Subtract(settler->position, position);
        float distance = Vector3Length(diff);
        if (distance <= radius) {
            return settler;
        }
    }
    return nullptr;
}

void Colony::clearSelection() {
    for (Settler* settler : settlers) {
        settler->SetSelected(false);
    }
}

std::vector<Settler*> Colony::getSelectedSettlers() const {
    std::vector<Settler*> selected;
    for (Settler* settler : settlers) {
        if (settler->IsSelected()) {
            selected.push_back(settler);
        }
    }
    return selected;
}

void Colony::cleanup() {
    // Cleanup is handled in destructor
}