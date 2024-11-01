#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <SDL2/SDL.h>

// Math constants
#define PI 3.14159265358979
#define TWO_PI 6.28318530717959
#define DEG2RAD PI / 180.0f
#define RAD2DEG 180.0f / PI

// Window properties
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE "C Game"
#define FPS 30
#define TARGET_TICK_TIME (1000 / FPS)
#define TILE_SIZE 64

// Player POV
#define FOV 60.0 * (PI / 180.0)
#define NUM_RAYS WINDOW_WIDTH

// Enemies
#define NUM_ENEMIES 10
#define ENEMY_WIDTH 10
#define ENEMY_HEIGHT 10
#define ENEMY_SPEED 30
#define ENEMY_ROTATION_SPEED 0.05

// // Raycasting
// #define MAX_HITS 80
// #define MAX_RAY_LENGTH 1000

// Colors
#define COLOR_RED   (SDL_Color){255, 0, 0, 255}
#define COLOR_GREEN (SDL_Color){0, 255, 0, 255}
#define COLOR_BLUE  (SDL_Color){0, 0, 255, 255}
#define COLOR_WHITE (SDL_Color){255, 255, 255, 255}
#define COLOR_BLACK (SDL_Color){0, 0, 0, 255}
#define COLOR_LIGHT_GRAY (SDL_Color){200, 200, 200, 255}

#define COLOR_WALL COLOR_LIGHT_GRAY
#define COLOR_PLAYER COLOR_BLUE
#define COLOR_ENEMY COLOR_GREEN

// Player properties
#define PLAYER_SPEED 100
#define PLAYER_ROTATION_SPEED 0.1

#endif // CONSTANTS_H