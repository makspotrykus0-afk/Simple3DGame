#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <mutex>

template <typename T>
class ObjectPool {
public:
    using FactoryFunction = std::function<std::unique_ptr<T>()>;

    ObjectPool(FactoryFunction factory, size_t initialSize = 10)
        : m_factory(factory), m_initialSize(initialSize) {
        growPool();
    }

    // Pobierz obiekt z puli
    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_pool.empty()) {
            growPool();
        }

        auto obj = std::move(m_pool.back());
        m_pool.pop_back();
        return obj;
    }

    // Zwróć obiekt do puli
    void release(std::unique_ptr<T> obj) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pool.push_back(std::move(obj));
    }

    // Aktualizuj rozmiar puli
    void setPoolSize(size_t newSize) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (newSize > m_pool.size()) {
            size_t toAdd = newSize - m_pool.size();
            for (size_t i = 0; i < toAdd; ++i) {
                m_pool.push_back(m_factory());
            }
        } else if (newSize < m_pool.size()) {
            size_t toRemove = m_pool.size() - newSize;
            for (size_t i = 0; i < toRemove; ++i) {
                m_pool.pop_back();
            }
        }
    }

    // Pobierz aktualny rozmiar puli
    size_t getPoolSize() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool.size();
    }

private:
    // Rozszerz pulę o początkowy rozmiar
    void growPool() {
        for (size_t i = 0; i < m_initialSize; ++i) {
            m_pool.push_back(m_factory());
        }
    }

    FactoryFunction m_factory;
    std::vector<std::unique_ptr<T>> m_pool;
    size_t m_initialSize;
    mutable std::mutex m_mutex;
};