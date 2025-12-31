#pragma once

#include <any>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <string>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <chrono>

/**
@brief System zdarzeń implementujący Observer Pattern i event caching
*/
class EventSystem {
public:
using HandlerId = size_t;
using EventHandler = std::function<void(const std::any& eventData)>;
struct Event {
    std::type_index type;
    std::any data;
    double timestamp;
    int priority;
    Event(std::type_index t, std::any d, double ts = 0.0, int p = 0)
        : type(t), data(std::move(d)), timestamp(ts), priority(p) {}
    bool operator<(const Event& other) const {
        return priority < other.priority;
    }
};
struct HandlerInfo {
    HandlerId id;
    std::string systemName;
    EventHandler handler;
    int priority;
    HandlerInfo(HandlerId hid, const std::string& name, EventHandler h, int p)
        : id(hid), systemName(name), handler(std::move(h)), priority(p) {}
    static std::unique_ptr<HandlerInfo> create(HandlerId hid, const std::string& name, EventHandler h, int p) {
        return std::make_unique<HandlerInfo>(hid, name, std::move(h), p);
    }
};
EventSystem() = default;
~EventSystem() = default;
template<typename EventType>
HandlerId registerHandler(const EventHandler& handler, const std::string& systemName, int priority = 0) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    HandlerId id = generateHandlerId();
    auto typeIndex = std::type_index(typeid(EventType));
    m_handlers[typeIndex].push_back(HandlerInfo::create(id, systemName, handler, priority));
    sortHandlers(typeIndex);
    return id;
}
bool unregisterHandler(HandlerId handlerId);
template<typename EventType>
void sendEvent(const EventType& eventData) {
    Event event(std::type_index(typeid(EventType)), std::any(eventData));
    processEventSync(event);
}
template<typename EventType>
void queueEvent(const EventType& eventData, int priority = 0) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    Event event(std::type_index(typeid(EventType)), std::any(eventData), getCurrentTimestamp(), priority);
    m_eventQueue.push(event);
}
size_t processEvents(size_t maxEvents = 0);
void clearEventQueue();
size_t getQueueSize() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_eventQueue.size();
}
void clearAllHandlers();
size_t getTotalHandlersCount() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& pair : m_handlers) {
        count += pair.second.size();
    }
    return count;
}
private:
HandlerId generateHandlerId();
void processEventSync(const Event& event);
void sortHandlers(std::type_index typeIndex);
double getCurrentTimestamp() const;
std::unordered_map<std::type_index, std::vector<std::unique_ptr<HandlerInfo>>> m_handlers;
std::priority_queue<Event> m_eventQueue;
mutable std::shared_mutex m_mutex;
size_t m_totalEventsProcessed = 0;
HandlerId m_nextHandlerId = 0;

// caching support (optional)
bool m_eventCachingEnabled = false;
struct EventCache {
    void addEvent(std::type_index, const std::any&) {}
    void cleanup() {}
} m_eventCache;
};
class EventBus {
public:
static EventBus& getInstance() {
static EventBus instance;
return instance;
}
EventSystem& getEventSystem() { return m_eventSystem; }
template<typename EventType>
static void send(const EventType& eventData) {
    getInstance().m_eventSystem.sendEvent<EventType>(eventData);
}
template<typename EventType>
static void queue(const EventType& eventData, int priority = 0) {
    getInstance().m_eventSystem.queueEvent<EventType>(eventData, priority);
}
template<typename EventType>
static EventSystem::HandlerId registerHandler(const EventSystem::EventHandler& handler, const std::string& systemName, int priority = 0) {
    return getInstance().m_eventSystem.registerHandler<EventType>(handler, systemName, priority);
}
private:
EventSystem m_eventSystem;
};

#define EVENT_BUS EventBus::getInstance()