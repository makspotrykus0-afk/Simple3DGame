#pragma once

#include "InteractableObject.h"
#include "Colony.h" // Potrzebne do sprawdzania dystansu do osadników

class Door : public BaseInteractableObject {
public:
    Door(const std::string& name, Vector3 position, float rotation, Colony* colony);
    virtual ~Door() = default;

    void update(float deltaTime) override;
    InteractionResult interact(GameEntity* player) override;
    void render() const; // Metoda do renderowania drzwi

    void setOpen(bool open);
    bool isOpen() const { return m_isOpen; }
    
    void setLocked(bool locked) { m_isLocked = locked; }
    bool isLocked() const { return m_isLocked; }

    // Override getBoundingBox to change size when open
    BoundingBox getBoundingBox() const override;

private:
    Colony* m_colony; // Referencja do kolonii, aby sprawdzać osadników
    bool m_isOpen;
    bool m_isLocked;
    float m_currentAngle;
    float m_baseRotation; // Rotacja budynku/drzwi w świecie
    float m_openAngle;    // Kąt o jaki drzwi się otwierają (np. 90 stopni)
    float m_doorSpeed;    // Prędkość otwierania/zamykania

    // Wymiary drzwi
    float m_width;
    float m_height;
    float m_thickness;

    // Logika automatyzacji
    void checkAutoOpen();
};