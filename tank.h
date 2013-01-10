// tank.h - tank definition.

#ifndef __TANK_H__
#define __TANK_H__

#define _USE_MATH_DEFINES
#include <math.h>

#include "vector.h"
#include "bounding.h"

#define TANK_HP 100
#define TANK_MAX_POWER 100
#define TANK_MIN_POWER -10
#define TANK_POWER_CHANGE_STEP 5
#define TANK_POWER_TO_SPEED_COEFFICIENT 0.5
#define TANK_MIN_LOOK_Z (-M_PI / 12)
#define TANK_MAX_LOOK_Z (M_PI / 4)
#define TANK_MAX_TURN_SPEED (M_PI / 12)
#define TANK_MAX_TURRET_TURN_SPEED (M_PI / 6)
#define TANK_FIRE_DELAY 300
#define TANK_BOUNDING_BOX_EXTENT { .x = 5, .y = 3, .z = 2 }
#define TANK_BOUNDING_SPHERE_RADIUS 1
#define TANK_BOUNDING_PRIMITIVES 2

typedef struct Tank
{
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

void tank_initialize(Tank *tank, const Vector *position, const Vector *top, int team);
void tank_tick(Tank *tank, const Landscape *l);

// Protocol functions.
void tank_turn(Tank *tank, double turn_angle);
void tank_look_at(Tank *tank, const Vector *look);
void tank_set_power(Tank *tank, int power);

#endif
