#pragma once
#include <string>

// --- RESOURCE EVENTS ---

// Triggered whenever a resource amount changes in the colony
struct ResourceChangedEvent {
  std::string resourceName;
  int newAmount;
  int changeDelta; // +amount or -amount

  ResourceChangedEvent(std::string name, int amount, int delta)
      : resourceName(name), newAmount(amount), changeDelta(delta) {}
};

// --- SYSTEM EVENTS ---

// Triggered when a new day/phase begins
struct TimeUpdateEvent {
  float currentTime;
  int day;
};
