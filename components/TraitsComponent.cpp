#include "TraitsComponent.h"
#include "../game/Settler.h"

TraitsComponent::TraitsComponent(Settler *owner) : m_owner(owner) {}

void TraitsComponent::initialize() {
  // Basic traits randomization could go here if not set externally
  // For now we assume traits are added by Settler factory/logic
}

void TraitsComponent::addTrait(TraitType trait) { m_traits.push_back(trait); }

bool TraitsComponent::hasTrait(TraitType trait) const {
  for (const auto &t : m_traits) {
    if (t == trait)
      return true;
  }
  return false;
}

float TraitsComponent::getWorkSpeedModifier() const {
  float modifier = 1.0f;
  if (hasTrait(TraitType::HARDWORKING))
    modifier += 0.2f; // +20%
  if (hasTrait(TraitType::LAZY))
    modifier -= 0.3f; // -30%
  if (hasTrait(TraitType::NIGHT_OWL)) {
    // Simple day/night check (assuming global time is accessable or passed)
    // Placeholder: neutral for now
  }
  return modifier;
}

float TraitsComponent::getEnergyDecayModifier() const {
  float modifier = 1.0f;
  if (hasTrait(TraitType::HARDWORKING))
    modifier += 0.1f; // Tires faster
  if (hasTrait(TraitType::LAZY))
    modifier -= 0.1f; // Tires slower
  return modifier;
}

float TraitsComponent::getHungerDecayModifier() const {
  float modifier = 1.0f;
  if (hasTrait(TraitType::GLUTTON))
    modifier += 0.3f; // Gets hungry faster
  if (hasTrait(TraitType::ASCETIC))
    modifier -= 0.2f; // Gets hungry slower
  return modifier;
}

std::string TraitsComponent::getTraitsString() const {
  std::string result = "";
  for (const auto &t : m_traits) {
    switch (t) {
    case TraitType::HARDWORKING:
      result += "[Worker] ";
      break;
    case TraitType::LAZY:
      result += "[Lazy] ";
      break;
    case TraitType::NIGHT_OWL:
      result += "[Owl] ";
      break;
    case TraitType::GLUTTON:
      result += "[Glutton] ";
      break;
    case TraitType::ASCETIC:
      result += "[Ascetic] ";
      break;
    }
  }
  return result;
}
