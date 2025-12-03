# Minimalna wersja CMake
cmake_minimum_required(VERSION 3.25)

# Nazwa projektu
project(Simple3DGame VERSION 1.0 LANGUAGES CXX)

# Ustawienie standardu C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_EXTENSIONS OFF)

# Wykrywanie systemu operacyjnego
if(WIN32)
    set(OS_WINDOWS TRUE)
elseif(APPLE)
    set(OS_MACOS TRUE)
elseif(UNIX)
    set(OS_LINUX TRUE)
endif()

# Dodajemy raylib jako podkatalog do kompilacji
add_subdirectory(raylib EXCLUDE_FROM_ALL)

# Tworzymy plik wykonywalny naszego projektu
add_executable(${PROJECT_NAME}
    # Główny plik
    src/main.cpp

    # Game
    game/Terrain.cpp
    game/Colony.cpp
    game/BuildingTask.cpp
    game/GatheringTask.cpp
    game/ResourceNode.cpp
    game/ResourceTypes.cpp
    game/Item.cpp
    game/InteractableObject.cpp
    game/BuildingBlueprint.cpp
    game/Door.cpp
    game/Tree.cpp
    game/Animal.cpp  
    game/ColonyAI.cpp
    game/Bed.cpp
    game/Settler.cpp
    game/Projectile.cpp # Added Projectile.cpp
    game/NavigationGrid.cpp # Added NavigationGrid.cpp

    # Core
    core/GameEngine.cpp
    core/EventSystem.cpp
    core/GameSystem.cpp # Ensure GameSystem.cpp is included

    # Systemy ECS
    systems/ResourceSystem.cpp
    systems/SkillsSystem.cpp
    systems/EquipmentSystem.cpp
    systems/FoodSystem.cpp
    systems/InteractionSystem.cpp
    systems/InventorySystem.cpp
    systems/BuildingSystem.cpp
    systems/StorageSystem.cpp
    systems/TestSystem.cpp
    systems/UISystem.cpp
    systems/TimeCycleSystem.cpp # Added TimeCycleSystem

    # Komponenty
    components/ResourceComponent.cpp
    components/InteractionComponent.cpp
    components/InventoryComponent.cpp
    components/StatsComponent.cpp
    components/BuildingComponent.cpp
    components/SkillsComponent.cpp
    components/PositionComponent.cpp
    components/TreeComponent.cpp
    components/EquipmentComponent.cpp

    # Entities
    entities/PlayerCharacter.cpp
    entities/Ant.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    core
    systems
    components
    entities
    game
    raylib/src # Raylib header is in raylib/src
)

# Add these defines after the target definition to avoid MSVC issues
# The NOMINMAX and NOSIGN defines are typically for Windows/MSVC.
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX NOSIGN)
endif()

# Link raylib and other libraries to our main target
target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    # Windows specific libraries
    opengl32.lib # Essential for OpenGL rendering
    glu32.lib    # OpenGL Utility Library, often needed with OpenGL
    winmm.lib    # Windows Multimedia library, used for timing functions
)

# Add preprocessor definitions for graphics and platform
target_compile_definitions(${PROJECT_NAME} PRIVATE GRAPHICS_API_OPENGL_33 PLATFORM_DESKTOP)

# Compilation options for different systems
if(OS_WINDOWS)
    # MSVC specific compile options
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /FS)
    else() # MinGW or other GCC-like compilers on Windows
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
elseif(OS_LINUX)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(${PROJECT_NAME} PRIVATE m dl X11)
elseif(OS_MACOS)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

if (${BUILD_EXAMPLES})
    message(STATUS "Building examples is enabled")
    add_subdirectory(examples)
endif()

enable_testing()