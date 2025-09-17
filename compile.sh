g++ -Wall -std=c++17 main.cpp game.cpp windowManager.cpp $(pkg-config --cflags --libs opencv4 sdl2 SDL2_image) -o surucursed

./surucursed