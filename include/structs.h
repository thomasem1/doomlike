#ifndef STRUCTS_H
#define STRUCTS_H

#define MAX_HEALTH 100

#define HIT_NONE 0
#define HIT_WALL 1
#define HIT_ENEMY 2

typedef struct {
    float x;
    float y;
} Vector2;

typedef struct {
    float x;
    float y;
    float theta;
    int health;
} Player;

typedef struct {
    float x;
    float y;
    float theta;
    int health;
} Enemy;

typedef struct {
    float x;
    float y;
    int hitType;
    int hitIndex;
} Ray;

typedef struct {
    float distance;
    float height;
    float angle;
    int hitType;
    int hitIndex;
} RayHit;

#endif // STRUCTS_H