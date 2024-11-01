#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <SDL2/SDL.h>
#include "../include/constants.h"
#include "../include/map.h"
#include "../include/structs.h"


// Game state
struct {
    // SDL properties
    SDL_Window *window;
    SDL_Texture *texture;
    SDL_Renderer *renderer;
    
    // Main loop flag
    bool quit;
    // Time tracking
    int lastTime;
    float deltaTime;
} state;

Player player;
Enemy enemies[NUM_ENEMIES];
Ray rays[NUM_RAYS];
RayHit *rayHits = NULL;
int numRayHits = 0;
int maxRayHits = 0;

//////////////// SETUP ////////////////
static bool initState(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "failed to initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    state.window = SDL_CreateWindow(WINDOW_TITLE, 
                                    SDL_WINDOWPOS_CENTERED, 
                                    SDL_WINDOWPOS_CENTERED, 
                                    WINDOW_WIDTH, 
                                    WINDOW_HEIGHT, 
                                    SDL_WINDOW_SHOWN);
    if (!state.window) {
        fprintf(stderr, "failed to create SDL window: %s\n", SDL_GetError());
        return false;
    }

    state.renderer = SDL_CreateRenderer(state.window,
                                        -1, 
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!state.renderer) {
        fprintf(stderr, "failed to create SDL renderer: %s\n", SDL_GetError());
        return false;
    }

    state.texture = SDL_CreateTexture(state.renderer,
                                      SDL_PIXELFORMAT_ABGR8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      WINDOW_WIDTH,
                                      WINDOW_HEIGHT);
    if (!state.texture) {
        fprintf(stderr, "failed to create SDL texture: %s\n", SDL_GetError());
        return false;
    }

    state.quit = false;
    state.lastTime = SDL_GetTicks();
    return true;
}

static void initPlayer(void) {
    player.x = 200;
    player.y = 200;
    player.theta = 0;
    player.health = MAX_HEALTH;
}

static void initEnemies(void) {
    for (int i = 0; i < NUM_ENEMIES; ++i) {
        enemies[i].x = rand() % (MAP_WIDTH * TILE_SIZE);
        enemies[i].y = rand() % (MAP_HEIGHT * TILE_SIZE);
        enemies[i].theta = 0;
        enemies[i].health = MAX_HEALTH;
    }
}

static void initAgents(void) {
    initPlayer();
    initEnemies();
}
//////////////// SETUP ////////////////

//////////////// PROCESS INPUT ////////////////
static bool isColliding(float x, float y) {
    int mapX = (int)(x / TILE_SIZE);
    int mapY = (int)(y / TILE_SIZE);

    if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
        return true;
    }

    return map[mapX][mapY] == 1;
}

static Vector2 moveObject(float x, float y, float dx, float dy) {
    // Use state.deltaTime to make movement framerate-independent
    float newX = x + dx * state.deltaTime;
    float newY = y + dy * state.deltaTime;

    if (!isColliding(newX, y)) {
        x = newX;
    }
    if (!isColliding(x, newY)) {
        y = newY;
    }

    return (Vector2){x, y};
}

static void processInput() {
    const Uint8 *keyState = SDL_GetKeyboardState(NULL);
    if (!keyState) return;

    if (keyState[SDL_SCANCODE_ESCAPE]) {
        SDL_Event quitEvent;
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
        return;
    }

    if (keyState[SDL_SCANCODE_W]) {
        Vector2 updatedPos = moveObject(player.x, player.y, cos(player.theta) * PLAYER_SPEED, sin(player.theta) * PLAYER_SPEED);
        player.x = updatedPos.x;
        player.y = updatedPos.y;
    }

    if (keyState[SDL_SCANCODE_S]) {
        Vector2 updatedPos = moveObject(player.x, player.y, -cos(player.theta) * PLAYER_SPEED, -sin(player.theta) * PLAYER_SPEED);
        player.x = updatedPos.x;
        player.y = updatedPos.y;
    }

    if (keyState[SDL_SCANCODE_A]) {
        player.theta -= PLAYER_ROTATION_SPEED;
    }

    if (keyState[SDL_SCANCODE_D]) {
        player.theta += PLAYER_ROTATION_SPEED;
    }
}
//////////////// PROCESS INPUT ////////////////

//////////////// GAME LOGIC ////////////////
static void clearObjectMap(void) {
    for (int i = 0; i < MAP_WIDTH; ++i) {
        for (int j = 0; j < MAP_HEIGHT; j++) {
            objectMap[i][j] = TILE_EMPTY;
        }
    }
}

static void updateEnemies(void) {
    for (int i = 0; i < NUM_ENEMIES; ++i) {
        if (!enemies[i].health > 0) continue;

        int random_move = rand() % 4;
        Vector2 updatedPos;
        switch (random_move) {
            case 0:
                updatedPos = moveObject(enemies[i].x, enemies[i].y, ENEMY_SPEED, 0);
                break;
            case 1:
                updatedPos = moveObject(enemies[i].x, enemies[i].y, -ENEMY_SPEED, 0);
                break;
            case 2:
                updatedPos = moveObject(enemies[i].x, enemies[i].y, 0, ENEMY_SPEED);
                break;
            case 3:
                updatedPos = moveObject(enemies[i].x, enemies[i].y, 0, -ENEMY_SPEED);
                break;
        }

        // Update the enemy's position
        enemies[i].x = updatedPos.x;
        enemies[i].y = updatedPos.y;
        objectMap[(int)(enemies[i].x / TILE_SIZE)][(int)(enemies[i].y / TILE_SIZE)] = TILE_ENEMY;
    }
}

static void castRays() {
    float angleStep = FOV / NUM_RAYS;
    float angle = player.theta - (FOV * 0.5);

    numRayHits = 0;

    for (int i = 0; i < NUM_RAYS; ++i) {
        float rayX = player.x;
        float rayY = player.y;
        float rayLength = 0;
        float step = 0.5f;

        while (true) {
            rayX += cos(angle) * step;
            rayY += sin(angle) * step;
            rayLength += step;

            if (rayX < 0 || rayX >= MAP_WIDTH * TILE_SIZE || rayY < 0 || rayY >= MAP_HEIGHT * TILE_SIZE) {
                break;
            }

            int mapX = (int)(rayX / TILE_SIZE);
            int mapY = (int)(rayY / TILE_SIZE);


            // Check wall hit
            if (map[mapX][mapY] == TILE_WALL) {
                // float hitX = rayX - player.x;
                // float hitY = rayY - player.y;
                // rays[i].x = sqrtf(hitX * hitX + hitY * hitY);
                rays[i].x = rayLength;
                rays[i].y = angle;
                break;
            } 

            if (objectMap[mapX][mapY] == TILE_ENEMY) {
                for (int j = 0; j < NUM_ENEMIES; j++) {
                    if (!enemies[j].health > 0) continue;
                    float left = enemies[j].x - ENEMY_WIDTH / 2;
                    float right = enemies[j].x + ENEMY_WIDTH / 2;
                    float top = enemies[j].y - ENEMY_HEIGHT / 2;
                    float bottom = enemies[j].y + ENEMY_HEIGHT / 2;
                    if (rayX >= left && rayX <= right && rayY >= top && rayY <= bottom) {
                        if (numRayHits >= maxRayHits) {
                            maxRayHits = maxRayHits ? maxRayHits * 2 : 16;
                            rayHits = realloc(rayHits, maxRayHits * sizeof(RayHit));
                        }
                        rayHits[numRayHits].distance = rayLength;
                        rayHits[numRayHits].height = ENEMY_HEIGHT / rayLength;
                        rayHits[numRayHits].angle = angle;
                        rayHits[numRayHits].hitType = HIT_ENEMY;
                        rayHits[numRayHits].hitIndex = j;
                        numRayHits++;
                    }
                }
            }
        }

        rays[i].y = angle;
        angle += angleStep;
    }
}

static void update(void) {
    clearObjectMap();
    updateEnemies();
    castRays();
}
//////////////// GAME LOGIC ////////////////

//////////////// RENDERING ////////////////
static void drawWalls(void) {
    for (int i = 0; i < NUM_RAYS; ++i) {
        SDL_SetRenderDrawColor(state.renderer, COLOR_WALL.r, COLOR_WALL.g, COLOR_WALL.b, COLOR_WALL.a);
        float wallHeight = (WINDOW_HEIGHT * TILE_SIZE) / (rays[i].x * cos(rays[i].y - player.theta));
        SDL_RenderDrawLine(state.renderer, i, (WINDOW_HEIGHT - wallHeight) * 0.5, i, (WINDOW_HEIGHT + wallHeight) * 0.5);
    }
}

static void drawPlayer() {
    SDL_SetRenderDrawColor(state.renderer, COLOR_PLAYER.r, COLOR_PLAYER.g, COLOR_PLAYER.b, COLOR_PLAYER.a);
    SDL_Rect playerRect = {(int)player.x - 5, (int)player.y - 5, 10, 10};
    SDL_RenderFillRect(state.renderer, &playerRect);
    SDL_RenderDrawLine(state.renderer, (int)player.x, (int)player.y, 
                        (int)(player.x + cos(player.theta) * 20), 
                        (int)(player.y + sin(player.theta) * 20));
}

static void drawEnemies() {
    SDL_SetRenderDrawColor(state.renderer, COLOR_ENEMY.r, COLOR_ENEMY.g, COLOR_ENEMY.b, COLOR_ENEMY.a);
    for (int i = 0; i < NUM_ENEMIES; ++i) {
        SDL_Rect enemyRect = {(int)enemies[i].x - 5, (int)enemies[i].y - 5, 10, 10};
        SDL_RenderFillRect(state.renderer, &enemyRect);
    }
}

static void drawDebug() {
    drawPlayer();
    drawEnemies();
}

static int compareRayHits(const void *a, const void *b) {
    RayHit *hitA = (RayHit *)a;
    RayHit *hitB = (RayHit *)b;
    return (hitA->distance - hitB->distance);
}

static void drawRayHits() {
    qsort(rayHits, numRayHits, sizeof(RayHit), compareRayHits);

    for (int i = 0; i < numRayHits; i++) {
        if (rayHits[i].hitType == HIT_ENEMY) {
            SDL_SetRenderDrawColor(state.renderer, COLOR_ENEMY.r, COLOR_ENEMY.g, COLOR_ENEMY.b, COLOR_ENEMY.a);

            // Perspective scaling
            float objHeight = (WINDOW_HEIGHT * TILE_SIZE) / (rayHits[i].distance * cos(rayHits[i].angle - player.theta));
            float objWidth = objHeight * (ENEMY_WIDTH / ENEMY_HEIGHT);  // Scale width relative to height

            // Calculate screenX position in relation to player’s view
            float relativeAngle = rayHits[i].angle - player.theta;
            int screenX = (relativeAngle / FOV) * WINDOW_WIDTH + (WINDOW_WIDTH / 2);

            // Define the rectangle for the enemy
            SDL_Rect enemyRect;
            enemyRect.x = screenX - (objWidth / 2);               // Center the rectangle horizontally
            enemyRect.y = (WINDOW_HEIGHT - objHeight) * 0.5;      // Center vertically based on height
            enemyRect.w = objWidth;
            enemyRect.h = objHeight;

            // Render the rectangle
            SDL_RenderFillRect(state.renderer, &enemyRect);
        }
    }
}


static void render(void) {
    SDL_SetRenderDrawColor(state.renderer, COLOR_BLACK.r, COLOR_BLACK.g, COLOR_BLACK.b, COLOR_BLACK.a);
    SDL_RenderClear(state.renderer);

    // Render environment and player
    drawWalls();
    // drawDebug();
    drawRayHits();

    SDL_RenderPresent(state.renderer);
}
//////////////// RENDERING ////////////////

//////////////// CLEANUP ////////////////
static void closeSDL(void) {
    SDL_DestroyTexture(state.texture);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}
//////////////// CLEANUP ////////////////

int main() {
    if (!initState()) {
        fprintf(stderr, "failed to initialize game state\n");
        return 1;
    }
    initAgents();

    SDL_Event e;

    while (!state.quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                state.quit = true;
            }
        }

        int timeToWait = TARGET_TICK_TIME - (SDL_GetTicks() - state.lastTime);
        if (timeToWait > 0 && timeToWait <= TARGET_TICK_TIME) SDL_Delay(timeToWait);
        state.deltaTime = (SDL_GetTicks() - state.lastTime) / 1000.0f;
        state.lastTime = SDL_GetTicks();

        processInput();
        update();
        render();
    }

    closeSDL();
    return 0;
}
