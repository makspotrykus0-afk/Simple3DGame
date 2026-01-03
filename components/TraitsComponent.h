#ifndef TRAITS_COMPONENT_H
#define TRAITS_COMPONENT_H

#include "IComponent.h"
#include <iostream>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>


class Settler;

enum class TraitType {
  HARDWORKING, // +Work Speed, -Energy
  LAZY,        // -Work Speed, +Happiness
  NIGHT_OWL,   // Peak performance at night
  GLUTTON,     // Eats more often
  ASCETIC      // Needs less food/comfort
};

class TraitsComponent : public IComponent {
public:
  TraitsComponent(Settler *owner);

  std::type_index getComponentType() const override {
    return std::type_index(typeid(TraitsComponent));
  }

  void initialize() override;
  void update(float deltaTime) override {}
  void render() override {}
  void shutdown() override {}

  // Trait Management
  void addTrait(TraitType trait);
  bool hasTrait(TraitType trait) const;

  // Modifiers (used by Utility AI and Actions)
  float getWorkSpeedModifier() const;
  float getEnergyDecayModifier() const;
  float getHungerDecayModifier() const;

  std::string getTraitsString() const;

private:
  Settler *m_owner;
  std::vector<TraitType> m_traits;
};

#endif
