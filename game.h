#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

extern int last_frame_time;

void setup();
void process_input();
void update(SDL_Renderer* renderer);
void render(SDL_Renderer* renderer);
void load_textures(SDL_Renderer* renderer);
void cleanup_textures(void);
void cleanup_camera();
void save_score(int score);
int load_high_score();
void record_score(int score);
void IMG_Quit();
void cleanup_camera_window();
void render_camera_frame();
void printrecord();
void render_text(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color);
