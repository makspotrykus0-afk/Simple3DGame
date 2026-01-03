#pragma once
#include <any>
#include <cassert>
#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>


/**
 * @brief Dependency Injection (DI) container for game services.
 */
class DIContainer {
public:
  DIContainer() = default;
  ~DIContainer() = default;

  DIContainer(const DIContainer &) = delete;
  DIContainer &operator=(const DIContainer &) = delete;

  template <typename Interface, typename Implementation, typename... Args>
  void RegisterSingleton(Args &&...args) {
    auto key = std::type_index(typeid(Interface));
    factories[key] = [args = std::make_tuple(
                          std::forward<Args>(args)...)]() mutable -> std::any {
      static std::shared_ptr<Implementation> instance = nullptr;
      if (!instance) {
        instance = std::apply(
            [](auto &&...params) {
              return std::make_shared<Implementation>(
                  std::forward<decltype(params)>(params)...);
            },
            args);
      }
      return instance;
    };
  }

  template <typename Interface, typename Implementation, typename... Args>
  void RegisterTransient(Args &&...args) {
    auto key = std::type_index(typeid(Interface));
    factories[key] = [args = std::make_tuple(
                          std::forward<Args>(args)...)]() mutable -> std::any {
      return std::apply(
          [](auto &&...params) {
            return std::make_shared<Implementation>(
                std::forward<decltype(params)>(params)...);
          },
          args);
    };
  }

  template <typename Interface> std::shared_ptr<Interface> Resolve() {
    auto key = std::type_index(typeid(Interface));
    auto it = factories.find(key);
    if (it == factories.end()) {
      throw std::runtime_error("Service not registered");
    }
    auto anyInstance = it->second();
    try {
      return std::any_cast<std::shared_ptr<Interface>>(anyInstance);
    } catch (const std::bad_any_cast &) {
      throw std::runtime_error("Service type mismatch");
    }
  }

  template <typename Interface> bool IsRegistered() const {
    auto key = std::type_index(typeid(Interface));
    return factories.find(key) != factories.end();
  }

  void Clear() { factories.clear(); }

private:
  std::unordered_map<std::type_index, std::function<std::any()>> factories;
};
