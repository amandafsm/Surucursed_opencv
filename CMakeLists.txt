cmake_minimum_required(VERSION 3.10)
project(surucursed_opencv_sdl)

set(CMAKE_CXX_STANDARD 17)
set(SOURCES
    main.cpp
    game.cpp
    windowManager.cpp
)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2IMAGE REQUIRED SDL2_image)

include_directories(${OpenCV_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2IMAGE_LIBRARY_DIRS})

add_executable(surucursed ${SOURCES})
target_link_libraries(surucursed ${OpenCV_LIBS} ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES})