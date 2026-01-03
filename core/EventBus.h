#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <algorithm>
#include <iostream>

// Base Event Interface
struct IEvent {
    virtual ~IEvent() = default;
};

// Event Bus System
// Provides a decoupled way for systems to communicate.
class EventBus {
public:
    using ListenerID = int;

    static EventBus& GetInstance() {
        static EventBus instance;
        return instance;
    }

    // Subscribe to a specific event type T
    template<typename T>
    ListenerID Subscribe(std::function<void(const T&)> callback) {
        std::type_index typeIdx = std::type_index(typeid(T));
        
        // Ensure vector exists
        if (listeners.find(typeIdx) == listeners.end()) {
            listeners[typeIdx] = std::make_unique<HandlerList>();
        }

        auto* handlers = static_cast<TypedHandlerList<T>*>(listeners[typeIdx].get());
        if (!handlers) {
            // Should not happen if logic is correct, but for safety re-create
            listeners[typeIdx] = std::make_unique<TypedHandlerList<T>>();
            handlers = static_cast<TypedHandlerList<T>*>(listeners[typeIdx].get());
        }

        int id = nextListenerId++;
        handlers->callbacks.push_back({id, callback});
        return id;
    }

    // Publish an event of type T to all subscribers
    template<typename T>
    void Publish(const T& event) {
        std::type_index typeIdx = std::type_index(typeid(T));
        
        if (listeners.find(typeIdx) != listeners.end()) {
            auto* handlers = static_cast<TypedHandlerList<T>*>(listeners[typeIdx].get());
            if (handlers) {
                for (const auto& entry : handlers->callbacks) {
                    entry.callback(event);
                }
            }
        }
    }

    // Unsubscribe (Optional, simpler implementation for now just ignores ID or we can add it later)
    // For Phase 2 prototype, we might skip full unsubscribe logic to keep it simple, 
    // but a proper system needs it. Let's add partial support.
    template<typename T>
    void Unsubscribe(ListenerID id) {
        std::type_index typeIdx = std::type_index(typeid(T));
        if (listeners.find(typeIdx) != listeners.end()) {
            auto* handlers = static_cast<TypedHandlerList<T>*>(listeners[typeIdx].get());
            if (handlers) {
                auto& vec = handlers->callbacks;
                vec.erase(std::remove_if(vec.begin(), vec.end(), 
                    [id](const auto& entry){ return entry.id == id; }), vec.end());
            }
        }
    }

    // Clear all listeners (e.g. on shutdown)
    void Reset() {
        listeners.clear();
        nextListenerId = 0;
    }

private:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    struct IHandlerList {
        virtual ~IHandlerList() = default;
    };

    template<typename T>
    struct TypedHandlerList : IHandlerList {
        struct Entry {
            ListenerID id;
            std::function<void(const T&)> callback;
        };
        std::vector<Entry> callbacks;
    };

    std::unordered_map<std::type_index, std::unique_ptr<IHandlerList>> listeners;
    ListenerID nextListenerId = 0;
};
