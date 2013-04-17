// tank.h - tank definition.

#pragma once
#ifndef __TANK_H__
#define __TANK_H__

#pragma message("__TANK_H__")

#define _USE_MATH_DEFINES
#include <math.h>
#include <threads.h>

#include "morrigan.h"
#include "vector.h"
#include "bounding.h"

#define TANK_HP 100
#define TANK_MIN_ENGINE_POWER -10
#define TANK_MAX_ENGINE_POWER 100
#define TANK_ENGINE_POWER_CHANGE_STEP 5
#define TANK_ENGINE_POWER_TO_SPEED_COEFFICIENT 0.5
#define TANK_MIN_LOOK_Z (-M_PI / 12)
#define TANK_MAX_LOOK_Z (M_PI / 4)
#define TANK_MAX_TURN_SPEED (M_PI / 12)
#define TANK_MAX_TURRET_TURN_SPEED (M_PI / 6)
#define TANK_FIRE_DELAY 10
#define TANK_BOUNDING_BOX_EXTENT { .x = 10, .y = 6, .z = 2 }
#define TANK_BOUNDING_SPHERE_RADIUS (3.75)
#define TANK_GUN_LENGTH 15
#define TANK_BOUNDING_PRIMITIVES 2
#define TANK_OBSERVING_RANGE 32

#pragma pack(push, 8)

typedef struct Tank
{
    mtx_t mtx;

    // Common game object fields.
    Vector position;
    Vector previous_position;
    Vector direction; // Look.
    Vector orientation; // Top.
    double speed;
    Bounding bounding_primitives[TANK_BOUNDING_PRIMITIVES], bounding;

    // Tank-specific fields.
    int hp;
    int team;
    int engine_power, engine_power_target;
    int fire_delay; // In ticks.
    Vector turret_direction, turret_direction_target;
    double turn_angle_target;
} Tank;

#pragma pack(pop)

void tank_initialize(Tank *tank, const Vector *position, const Vector *top, int team);
void tank_destroy(Tank *tank);
bool tank_tick(Tank *tank, const Landscape *l);

void tank_rotate_direction(Vector *direction, const Vector *old_orientation, const Vector *orientation);
void tank_change_turn_worker(double *turn_angle_target, double max_turn_speed, Vector *direction, Vector *orientation);
void tank_rotate_turret_worker(Vector *turret_direction_target, Vector *turret_direction, double max_turret_turn_speed);

// Protocol functions.
void tank_turn(Tank *tank, double turn_angle);
void tank_look_at(Tank *tank, const Vector *look);
void tank_set_engine_power(Tank *tank, int power);
bool tank_shoot(Tank *tank);
double tank_get_heading(Tank *tank);

#endif /* __TANK_H__ */
