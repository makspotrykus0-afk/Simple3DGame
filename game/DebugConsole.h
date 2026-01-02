#pragma once

#include "raylib.h"
#include <string>
#include <vector>
#include <functional>
#include <map>

class DebugConsole {
public:
    static DebugConsole& getInstance();

    void toggle();
    bool isVisible() const;
    void update();
    void render();
    void log(const std::string& message);
    void executeCommand(const std::string& commandLine);

    // Rejestracja komendy z lambda/funkcją
    void registerCommand(const std::string& name, std::function<void(const std::vector<std::string>&)> callback, const std::string& helpText);

private:
    DebugConsole(); // Singleton
    ~DebugConsole() = default;

    // Prywatne metody pomocnicze
    void handleInput();
    void renderOverlay();
    void renderLog();
    void renderInput();

    // Stan konsoli
    bool m_isVisible;
    std::string m_currentInput;
    std::vector<std::string> m_logBuffer;
    std::vector<std::string> m_commandHistory;
    int m_historyIndex;
    float m_cursorBlinkTimer;
    bool m_showCursor;
    int m_scrollOffset;

    // Struktura komendy
    struct Command {
        std::function<void(const std::vector<std::string>&)> action;
        std::string helpText;
    };

    std::map<std::string, Command> m_commands;

    // Dostęp do zewnętrznych systemów (opcjonalny, można używać GameSystem::get...)
    void registerDefaultCommands();
};
