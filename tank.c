// tank.c - tank definition.

#include <assert.h>
#include <stdbool.h>

#include "tank.h"
#include "landscape.h"

void tank_change_turn(Tank *tank);
void tank_change_power(Tank *tank);
void tank_move(Tank *tank, const Landscape *l);
void tank_rotate_turret(Tank *tank);

void tank_initialize(Tank *tank, const Vector *position, const Vector *top, int team)
{
    assert(tank && "Bad tank pointer.");
    assert(position && top && "Bad geometry pointers.");

    *tank = (Tank)
    {
        .position = { .x = position->x, .y = position->y, .z = position->z },
        .previous_position = { .x = position->x, .y = position->y, .z = position->z },
        .direction = { .x = 1, .y = 0, .z = 0 },
        .orientation = { .x = top->x, .y = top->y, .z = top->z },
        .speed = 0,
        .bounding_primitives =
        {
            {
                .origin = &tank->position,
                .previous_origin = &tank->previous_position,
                .orientation = &tank->orientation,
                .direction = &tank->direction,
                .offset = { .x = 0, .y = 0, .z = 0 },
                .bounding_type = bounding_box,
                .data = { .extent = TANK_BOUNDING_BOX_EXTENT }
            },
            {
                .origin = &tank->position,
                .previous_origin = &tank->previous_position,
                .orientation = &tank->orientation,
                .direction = &tank->direction,
                .offset = { .x = 0, .y = 0, .z = TANK_BOUNDING_SPHERE_RADIUS },
                .bounding_type = bounding_sphere,
                .data = { .radius = TANK_BOUNDING_SPHERE_RADIUS }
            }
        },
        .bounding =
        {
            .origin = &tank->position,
            .previous_origin = &tank->previous_position,
            .orientation = &tank->orientation,
            .direction = &tank->direction,
            .offset = { .x = 0, .y = 0, .z = 0 },
            .bounding_type = bounding_composite,
            .data.composite_data = 
            {
                .children = tank->bounding_primitives,
                .children_count = TANK_BOUNDING_PRIMITIVES
            }
        },
        .hp = TANK_HP,
        .team = team,
        .engine_power = 0,
        .engine_power_target = 0,
        .fire_delay = 0,
        .turret_direction = { .x = 1, .y = 0, .z = 0 },
        .turret_direction_target = { .x = 1, .y = 0, .z = 0 },
        .turn_angle_target = 0
    };
}

void tank_tick(Tank *tank, const Landscape *l)
{
    assert(tank && "Bad tank pointer.");

    tank_change_power(tank);
    tank_move(tank, l);
    tank_change_turn(tank);
    tank_rotate_turret(tank);
}

void tank_turn(Tank *tank, double turn_angle)
{
    assert(tank && "Bad tank pointer.");
    assert(-M_PI <= turn_angle && turn_angle <= M_PI && "Bad turb angle.");
    tank->turn_angle_target = turn_angle;
}

void tank_look_at(Tank *tank, const Vector *look)
{
    assert(tank && "Bad tank pointer.");
    assert(look && "Bad look pointer.");

    tank->turret_direction_target = *look;
    VECTOR_NORMALIZE(&tank->turret_direction_target);

    if (TANK_MIN_LOOK_Z > tank->turret_direction_target.z)
    {
        tank->turret_direction_target.z = TANK_MIN_LOOK_Z;
        VECTOR_NORMALIZE(&tank->turret_direction_target);
    }
    else if (TANK_MAX_LOOK_Z < tank->turret_direction_target.z)
    {
        tank->turret_direction_target.z = TANK_MAX_LOOK_Z;
        VECTOR_NORMALIZE(&tank->turret_direction_target);
    }
}

void tank_set_power(Tank *tank, int power)
{
    assert(tank && "Bad tank pointer.");

    if(TANK_MIN_POWER > power)
    {
        power = TANK_MIN_POWER;
    }
    else if(TANK_MAX_POWER < power)
    {
        power = TANK_MAX_POWER;
    }

    tank->engine_power_target = power;
}

void tank_change_turn(Tank *tank)
{
    assert(tank && "Bad tank pointer.");

    if (vector_eps >= tank->turn_angle_target)
    {
        return;
    }

    double step_turn_angle;
    if (fabs(tank->turn_angle_target) <= TANK_MAX_TURN_SPEED)
    {
        step_turn_angle = tank->turn_angle_target;
        tank->turn_angle_target = 0;
    }
    else
    {
        step_turn_angle = TANK_MAX_TURN_SPEED;
        tank->turn_angle_target += (-1.0) * signbit(tank->turn_angle_target) * TANK_MAX_TURN_SPEED;
    }

    VECTOR_ROTATE(&tank->direction, &tank->orientation, step_turn_angle);
}

void tank_change_power(Tank *tank)
{
    assert(tank && "Bad tank pointer.");

    if (tank->engine_power_target == tank->engine_power)
    {
        return;
    }

    if (abs(tank->engine_power_target - tank->engine_power) <= TANK_POWER_CHANGE_STEP)
    {
        tank->engine_power = tank->engine_power_target;
        return;
    }

    if (tank->engine_power_target > tank->engine_power)
    {
        tank->engine_power -= TANK_POWER_CHANGE_STEP;
    }
    else
    {
        tank->engine_power += TANK_POWER_CHANGE_STEP;
    }
}

void tank_move(Tank *tank, const Landscape *l)
{
    assert(tank && "Bad tank pointer.");

    tank->previous_position = tank->position;

    if (0 == tank->engine_power)
    { // We hold brakes.
        tank->speed = 0.0;
        return;
    }

    tank->speed = TANK_POWER_TO_SPEED_COEFFICIENT * tank->engine_power;

    while (true)
    {
        Vector t;
        vector_scale(&tank->direction, tank->speed, &t);
        vector_add(&tank->previous_position, &t, &tank->position);
        tank->position.z = landscape_get_height_at(l, tank->position.x, tank->position.y);

        vector_sub(&tank->position, &tank->previous_position, &t);
        double new_speed = vector_length(&t);

        if (fabs(new_speed - tank->speed) < vector_eps)
        {
            break;
        }

        tank->speed = new_speed;
    }

    landscape_get_normal_at(l, tank->position.x, tank->position.y, &tank->orientation);
}

void tank_rotate_turret(Tank *tank)
{
    assert(tank && "Bad tank pointer.");

    if (vector_eq(&tank->turret_direction_target, &tank->turret_direction))
    {
        return;
    }

    double actual_angle = acos(vector_mul(&tank->turret_direction, &tank->turret_direction_target) /
                               (vector_length(&tank->turret_direction) * vector_length(&tank->turret_direction_target)));

    if (fabs(actual_angle) <= TANK_MAX_TURRET_TURN_SPEED)
    {
        tank->turret_direction = tank->turret_direction_target;
        return;
    }

    Vector axis;
    vector_vector_mul(&tank->turret_direction, &tank->turret_direction_target, &axis);
    vector_rotate(&tank->turret_direction, &axis, TANK_MAX_TURRET_TURN_SPEED, &tank->turret_direction);
}
