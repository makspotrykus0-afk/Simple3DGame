#pragma once
#include "EventSystem.h" // Use existing EventSystem
#include <any>
#include <iostream>
#include <string>


// Test Event Definition
struct TestEvent {
  std::string message;
  TestEvent(std::string msg) : message(msg) {} // Constructor required
};

// Test System that acts as a listener
class EventBusValidator {
public:
  void Initialize() {
    // Subscribe to TestEvent
    EventBus::registerHandler<TestEvent>(
        [](const std::any &e) {
          try {
            const auto &ev = std::any_cast<const TestEvent &>(e);
            std::cout << "[EventBusValidator] RECEIVED: " << ev.message
                      << std::endl;
          } catch (const std::bad_any_cast &err) {
            std::cerr << "[EventBusValidator] ERROR: Bad cast: " << err.what()
                      << std::endl;
          }
        },
        "EventBusValidator");
  }

  void TriggerTest() {
    std::cout << "[EventBusValidator] Publishing 'Hello World' event..."
              << std::endl;
    EventBus::send(TestEvent("Hello EventBus World!"));
  }
};
