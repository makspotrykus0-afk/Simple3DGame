
#pragma once
#include <memory>

#include <unordered_map>

#include <typeindex>

#include <typeinfo>

#include <any>

#include <functional>

#include <cassert>

#include <stdexcept>
/**



@brief Dependency Injection (DI) container for game services.






Stores services (systems, managers) and allows their registration and retrieval.




Supports both singleton and transient services.

*/

class DIContainer {

public:

DIContainer() = default;

~DIContainer() = default;

// Delete copy and assignment

DIContainer(const DIContainer&) = delete;

DIContainer& operator=(const DIContainer&) = delete;

/**


@brief Registers a service as a singleton.

@tparam Interface Interface type (abstract).

@tparam Implementation Implementation type (concrete).

@param args Arguments for Implementation constructor.

*/

template<typename Interface, typename Implementation, typename... Args>

void RegisterSingleton(Args&&... args) {

auto key = std::type_index(typeid(Interface));

// Store a factory that creates the singleton on first call

factories[key] = args... mutable -> std::any {

static std::shared_ptr<Implementation> instance = nullptr;

if (!instance) {

instance = std::make_shared<Implementation>(std::forward<Args>(args)...);

}

return instance;

};

}


/**


@brief Registers a service as transient (new instance each request).

@tparam Interface Interface type (abstract).

@tparam Implementation Implementation type (concrete).

@param args Arguments for Implementation constructor.

*/

template<typename Interface, typename Implementation, typename... Args>

void RegisterTransient(Args&&... args) {

auto key = std::type_index(typeid(Interface));

factories[key] = args... mutable -> std::any {

return std::make_shared<Implementation>(std::forward<Args>(args)...);

};

(void)sizeof...(Args); // Avoid unused-pack warning in some compilers

}


/**


@brief Resolves a service registered for the given interface.

@tparam Interface Interface type.

@return Shared pointer to the service.

@throw std::runtime_error If service is not registered or type mismatch.

*/

template<typename Interface>

std::shared_ptr<Interface> Resolve() {

auto key = std::type_index(typeid(Interface));

auto it = factories.find(key);

if (it == factories.end()) {

throw std::runtime_error("Service not registered");

}

auto anyInstance = it->second();

try {

return std::any_cast<std::shared_ptr<Interface>>(anyInstance);

} catch (const std::bad_any_cast&) {

throw std::runtime_error("Service type mismatch");

}

}


/**


@brief Checks if a service is registered.

@tparam Interface Interface type.

@return true if registered, false otherwise.

*/

template<typename Interface>

bool IsRegistered() const {

auto key = std::type_index(typeid(Interface));

return factories.find(key) != factories.end();

}


/**


@brief Clears all registered services.

*/

void Clear() {

factories.clear();

}



private:

std::unordered_map<std::type_index, std::function<std::any()>> factories;

};
