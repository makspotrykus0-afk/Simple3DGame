cmake_minimum_required(VERSION 3.10)
project(Simple3DGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(OS_WINDOWS TRUE)
elseif(APPLE)
    set(OS_MACOS TRUE)
elseif(UNIX)
    set(OS_LINUX TRUE)
endif()

add_subdirectory(raylib)

add_executable(Simple3DGame
    src/main.cpp
    core/GameEngine.cpp
    core/EventSystem.cpp
    core/GameSystem.cpp
    core/DIContainer.cpp
    game/Terrain.cpp
    game/Colony.cpp
    game/WorldManager.cpp
    game/Region.cpp
    game/Faction.cpp
    game/Settlement.cpp
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
    game/BuildingInstance.cpp
    game/Bed.cpp
    game/Settler.cpp
    game/DebugConsole.cpp
    game/Projectile.cpp
    game/NavigationGrid.cpp
    systems/EditorSystem.cpp
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
    systems/TimeCycleSystem.cpp
    systems/NeedsSystem.cpp
    systems/CraftingSystem.cpp
    components/ResourceComponent.cpp
    components/InteractionComponent.cpp
    components/InventoryComponent.cpp
    components/StatsComponent.cpp
    components/NeedComponent.cpp
    components/NavComponent.cpp
    components/ActionComponent.cpp
    components/BuildingComponent.cpp
    components/SkillsComponent.cpp
    components/PositionComponent.cpp
    components/TreeComponent.cpp
    components/EquipmentComponent.cpp
    entities/PlayerCharacter.cpp
    entities/Ant.cpp
)

target_include_directories(Simple3DGame PRIVATE
    src
    core
    systems
    components
    entities
    game
    raylib/src
)

target_link_libraries(Simple3DGame PRIVATE raylib)

if(OS_WINDOWS)
    target_link_libraries(Simple3DGame PRIVATE
        opengl32
        glu32
        winmm
    )
endif()

if(OS_LINUX)
    target_link_libraries(Simple3DGame PRIVATE
        m
        dl
        X11
    )
endif()

if(OS_MACOS)
    target_link_libraries(Simple3DGame PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

target_compile_options(Simple3DGame PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(Simple3DGame PRIVATE
    GRAPHICS_API_OPENGL_33
    PLATFORM_DESKTOP
)

enable_testing()
