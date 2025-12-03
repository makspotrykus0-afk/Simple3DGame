#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <any>
#include <string>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <chrono>

/**
 * @brief System zdarzeń implementujący Observer Pattern i event caching
 * 
 * EventSystem zarządza komunikacją między systemami gry poprzez system zdarzeń.
 * Zapewnia:
 * - Asynchroniczne i synchroniczne przekazywanie zdarzeń
 * - Event caching dla optymalizacji
 * - Thread-safe operacje
 * - Lazy loading event handler'ów
 */
class EventSystem {
public:
    /**
     * @brief Typ dla identyfikatora event handlera
     */
    using HandlerId = size_t;

    /**
     * @brief Typ funkcji obsługującej zdarzenia
     */
    using EventHandler = std::function<void(const std::any& eventData)>;

    /**
     * @brief Struktura reprezentująca zdarzenie
     */
    struct Event {
        /** Typ zdarzenia */
        std::type_index type;
        /** Dane zdarzenia */
        std::any data;
        /** Timestamp zdarzenia */
        double timestamp;
        /** Priorytet zdarzenia (wyższy = wcześniejsze przetwarzanie) */
        int priority;

        /**
         * @brief Konstruktor zdarzenia
         */
        Event(std::type_index t, std::any d, double ts = 0.0, int p = 0)
            : type(t), data(std::move(d)), timestamp(ts), priority(p) {}

        /**
         * @brief Operator porównania dla kolejki priorytetowej
         */
        bool operator<(const Event& other) const {
            return priority < other.priority; // Max-heap - wyższy priorytet = wcześniej
        }
    };

    /**
     * @brief Struktura opisująca zarejestrowanego handlera
     */
    struct HandlerInfo {
        /** ID handlera */
        HandlerId id;
        /** Nazwa systemu rejestrującego */
        std::string systemName;
        /** Funkcja obsługująca */
        EventHandler handler;
        /** Priorytet handlera */
        int priority;

        /**
         * @brief Konstruktor
         */
        HandlerInfo(HandlerId hid, const std::string& name, EventHandler h, int p)
            : id(hid), systemName(name), handler(std::move(h)), priority(p) {}

        /**
         * @brief Tworzy nową instancję HandlerInfo
         */
        static std::unique_ptr<HandlerInfo> create(HandlerId hid, const std::string& name, EventHandler h, int p) {
            return std::make_unique<HandlerInfo>(hid, name, std::move(h), p);
        }
    };

    /**
     * @brief Konstruktor
     */
    EventSystem() = default;

    /**
     * @brief Destruktor
     */
    ~EventSystem() = default;

    /**
     * @brief Rejestruje handler dla określonego typu zdarzenia
     * @param eventType Typ zdarzenia
     * @param handler Funkcja obsługująca
     * @param systemName Nazwa systemu rejestrującego
     * @param priority Priorytet handlera
     * @return ID zarejestrowanego handlera
     */
    template<typename EventType>
    HandlerId registerHandler(const EventHandler& handler, 
                             const std::string& systemName,
                             int priority = 0) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        HandlerId id = generateHandlerId();
        auto typeIndex = std::type_index(typeid(EventType));
        
        m_handlers[typeIndex].push_back(HandlerInfo::create(id, systemName, handler, priority));
        
        // Sortuj handlery według priorytetu (wyższy = wcześniej)
        sortHandlers(typeIndex);
        
        return id;
    }

    /**
     * @brief Usuwa handler na podstawie ID
     * @param handlerId ID handlera do usunięcia
     * @return true jeśli handler został znaleziony i usunięty
     */
    bool unregisterHandler(HandlerId handlerId);

    /**
     * @brief Wysyła synchroniczne zdarzenie (natychmiastowe przetwarzanie)
     * @param eventData Dane zdarzenia
     */
    template<typename EventType>
    void sendEvent(const EventType& eventData) {
        Event event(std::type_index(typeid(EventType)), std::any(eventData));
        processEventSync(event);
    }

    /**
     * @brief Wysyła asynchroniczne zdarzenie (dodane do kolejki)
     * @param eventData Dane zdarzenia
     * @param priority Priorytet zdarzenia
     */
    template<typename EventType>
    void queueEvent(const EventType& eventData, int priority = 0) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        Event event(std::type_index(typeid(EventType)), std::any(eventData), 
                   getCurrentTimestamp(), priority);
        m_eventQueue.push(event);
    }

    /**
     * @brief Przetwarza wszystkie zdarzenia w kolejce
     * @param maxEvents Maksymalna liczba zdarzeń do przetworzenia (0 = wszystkie)
     * @return Liczba przetworzonych zdarzeń
     */
    size_t processEvents(size_t maxEvents = 0);

    /**
     * @brief Czyści kolejkę zdarzeń
     */
    void clearEventQueue();

    /**
     * @brief Pobiera rozmiar kolejki zdarzeń
     * @return Liczba zdarzeń w kolejce
     */
    size_t getQueueSize() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_eventQueue.size();
    }

    /**
     * @brief Czyści wszystkie handlery
     */
    void clearAllHandlers();

    /**
     * @brief Włącza/wyłącza event caching
     * @param enabled Flaga włączenia
     */
    void setEventCachingEnabled(bool enabled) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_eventCachingEnabled = enabled;
    }

    /**
     * @brief Pobiera statystyki systemu zdarzeń
     */
    struct EventStats {
        size_t totalEventsProcessed;
        size_t totalHandlersRegistered;
        size_t currentQueueSize;
        double averageProcessingTime;
    };

    /**
     * @brief Pobiera statystyki systemu
     * @return Statystyki
     */
    EventStats getStats() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return EventStats{
            m_totalEventsProcessed,
            getTotalHandlersCount(),
            m_eventQueue.size(),
            m_averageProcessingTime
        };
    }

private:
    /**
     * @brief Generuje unikalne ID handlera
     * @return Nowe ID
     */
    HandlerId generateHandlerId();

    /**
     * @brief Przetwarza pojedyncze zdarzenie synchronicznie
     * @param event Zdarzenie do przetworzenia
     */
    void processEventSync(const Event& event);

    /**
     * @brief Sortuje handlery według priorytetu
     * @param typeIndex Typ zdarzenia
     */
    void sortHandlers(std::type_index typeIndex);

    /**
     * @brief Pobiera aktualny timestamp
     * @return Aktualny czas w sekundach
     */
    double getCurrentTimestamp() const;

    /**
     * @brief Pobiera łączną liczbę zarejestrowanych handlerów
     * @return Liczba handlerów
     */
    size_t getTotalHandlersCount() const;

    /**
     * @brief Cache dla często używanych zdarzeń
     */
    struct EventCache {
        std::unordered_map<std::type_index, std::vector<std::any>> cachedEvents;
        std::chrono::high_resolution_clock::time_point lastCleanup;
        
        EventCache() : lastCleanup(std::chrono::high_resolution_clock::now()) {}
        
        void addEvent(const std::type_index& type, const std::any& event) {
            if (cachedEvents[type].size() < 100) { // Limit cache size
                cachedEvents[type].push_back(event);
            }
        }
        
        void cleanup() {
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCleanup).count() > 60) {
                cachedEvents.clear();
                lastCleanup = now;
            }
        }
    };

private:
    /** Mapa handlerów według typu zdarzenia */
    std::unordered_map<std::type_index, std::vector<std::unique_ptr<HandlerInfo>>> m_handlers;
    
    /** Kolejka zdarzeń do przetworzenia */
    std::priority_queue<Event> m_eventQueue;
    
    /** Mutex dla thread-safety */
    mutable std::shared_mutex m_mutex;
    
    /** Cache zdarzeń dla optymalizacji */
    EventCache m_eventCache;
    
    /** Flaga włączenia event caching */
    bool m_eventCachingEnabled = false;
    
    /** Statystyki */
    size_t m_totalEventsProcessed = 0;
    double m_averageProcessingTime = 0.0;
    HandlerId m_nextHandlerId = 0;
};

/**
 * @brief Event Bus dla łatwiejszego zarządzania zdarzeniami
 */
class EventBus {
public:
    /**
     * @brief Pobiera globalną instancję EventBus
     * @return Referencja do globalnej instancji
     */
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    /**
     * @brief Pobiera system zdarzeń
     * @return Referencja do systemu zdarzeń
     */
    EventSystem& getEventSystem() { return m_eventSystem; }

    /**
     * @brief Wyślij zdarzenie
     * @tparam EventType Typ zdarzenia
     * @param eventData Dane zdarzenia
     */
    template<typename EventType>
    static void send(const EventType& eventData) {
        getInstance().m_eventSystem.sendEvent<EventType>(eventData);
    }

    /**
     * @brief Kolejkuj zdarzenie
     * @tparam EventType Typ zdarzenia
     * @param eventData Dane zdarzenia
     * @param priority Priorytet
     */
    template<typename EventType>
    static void queue(const EventType& eventData, int priority = 0) {
        getInstance().m_eventSystem.queueEvent<EventType>(eventData, priority);
    }

    /**
     * @brief Zarejestruj handler
     * @tparam EventType Typ zdarzenia
     * @param handler Funkcja obsługująca
     * @param systemName Nazwa systemu
     * @param priority Priorytet
     * @return ID handlera
     */
    template<typename EventType>
    static EventSystem::HandlerId registerHandler(const EventSystem::EventHandler& handler,
                                                const std::string& systemName,
                                                int priority = 0) {
        return getInstance().m_eventSystem.registerHandler<EventType>(handler, systemName, priority);
    }

private:
    /** System zdarzeń */
    EventSystem m_eventSystem;
};

/**
 * @def EVENT_BUS
 * @brief Makro dla łatwiejszego dostępu do globalnego EventBus
 */
#define EVENT_BUS EventBus::getInstance()

/**
 * @def SEND_EVENT(eventData)
 * @brief Makro do wysyłania zdarzenia
 */
#define SEND_EVENT(eventData) \
    EVENT_BUS.getEventSystem().sendEvent(eventData)

/**
 * @def QUEUE_EVENT(eventData, priority)
 * @brief Makro do kolejkowania zdarzenia
 */
#define QUEUE_EVENT(eventData, priority) \
    EVENT_BUS.getEventSystem().queueEvent(eventData, priority)