#include "EventSystem.h"
#include <algorithm>
#include <chrono>

EventSystem::HandlerId EventSystem::generateHandlerId() {
    return ++m_nextHandlerId;
}

bool EventSystem::unregisterHandler(HandlerId handlerId) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    for (auto& [type, handlers] : m_handlers) {
        auto it = std::find_if(handlers.begin(), handlers.end(),
            [handlerId](const std::unique_ptr<HandlerInfo>& info) {
                return info->id == handlerId;
            });
            
        if (it != handlers.end()) {
            handlers.erase(it);
            return true;
        }
    }
    return false;
}

void EventSystem::processEventSync(const Event& event) {
    std::vector<EventHandler> handlersToCall;
    
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_handlers.find(event.type);
        if (it != m_handlers.end()) {
            handlersToCall.reserve(it->second.size());
            for (const auto& info : it->second) {
                handlersToCall.push_back(info->handler);
            }
        }
    }
    
    // Execute handlers without lock to prevent deadlocks if handlers register/send events
    for (const auto& handler : handlersToCall) {
        try {
            handler(event.data);
        } catch (...) {
            // Prevent crashes from handler exceptions
        }
    }
    
    if (m_eventCachingEnabled) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_eventCache.addEvent(event.type, event.data);
        m_eventCache.cleanup();
    }
    
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_totalEventsProcessed++;
    }
}

size_t EventSystem::processEvents(size_t maxEvents) {
    size_t processedCount = 0;
    
    while (true) {
        // Use a dummy type_index for initialization
        Event event(std::type_index(typeid(void)), std::any()); 
        
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            if (m_eventQueue.empty()) {
                break;
            }
            
            if (maxEvents > 0 && processedCount >= maxEvents) {
                break;
            }
            
            event = m_eventQueue.top();
            m_eventQueue.pop();
        }
        
        processEventSync(event);
        processedCount++;
    }
    
    return processedCount;
}

void EventSystem::clearEventQueue() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    while(!m_eventQueue.empty()) {
        m_eventQueue.pop();
    }
}

void EventSystem::clearAllHandlers() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_handlers.clear();
}

void EventSystem::sortHandlers(std::type_index typeIndex) {
    // This is called from registerHandler where lock is already acquired
    auto it = m_handlers.find(typeIndex);
    if (it != m_handlers.end()) {
        std::sort(it->second.begin(), it->second.end(),
            [](const std::unique_ptr<HandlerInfo>& a, const std::unique_ptr<HandlerInfo>& b) {
                return a->priority > b->priority;
            });
    }
}

double EventSystem::getCurrentTimestamp() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
}
