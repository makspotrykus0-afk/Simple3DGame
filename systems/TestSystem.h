#pragma once

#ifndef TESTSYSTEM_H
#define TESTSYSTEM_H

#include "../core/GameSystem.h"
#include <functional>
#include <vector>
#include <string>

class TestSystem : public GameSystem {
public:
    TestSystem();

    // Dodaj test do systemu
    void addTest(const std::string& testName, std::function<bool()> testFunction);

    // Uruchom wszystkie testy
    void runAllTests();

    // Aktualizacja systemu testów
    void update(float deltaTime) override;

    // Renderowanie systemu testów
    void render() override;

private:
    struct Test {
        std::string name;
        std::function<bool()> function;
    };

    std::vector<Test> m_tests;
};

#endif // TESTSYSTEM_H