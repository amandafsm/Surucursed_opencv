#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

extern int last_frame_time;

void setup();
void process_input();
void update();
void render(SDL_Renderer* renderer);
void load_textures(SDL_Renderer* renderer);
void cleanup_textures(void);
void cleanup_camera();
void IMG_Quit();
void cleanup_camera_window();
void render_camera_frame();
