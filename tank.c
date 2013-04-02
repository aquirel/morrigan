// tank.c - tank definition.

#include <assert.h>
#include <stdbool.h>

#include "tank.h"
#include "landscape.h"
#include "debug.h"

void tank_change_turn(Tank *tank);
void tank_change_engine_power(Tank *tank);
bool tank_move(Tank *tank, const Landscape *l);
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

    Vector default_top = { .x = 0, .y = 0, .z = 1 };
    if (0 == memcmp(&default_top, top, sizeof(Vector)))
    {
        return;
    }

    Vector axis;
    vector_vector_mul(&default_top, top, &axis);
    VECTOR_NORMALIZE(&axis);
    double angle = vector_angle(top, &default_top);
    VECTOR_ROTATE(&tank->direction, &axis, angle);
}

void tank_destroy(Tank *tank)
{
    assert(tank && "Bad tank pointer.");
    mtx_destroy(&tank->mtx);
}

bool tank_tick(Tank *tank, const Landscape *l)
{
    assert(tank && "Bad tank pointer.");

    bool result = true;
    check(thrd_success == mtx_lock(&tank->mtx), "Failed to lock tank mutex.", "");
    tank_change_engine_power(tank);
    result = tank_move(tank, l);
    tank_change_turn(tank);
    tank_rotate_turret(tank);
    check(thrd_success == mtx_unlock(&tank->mtx), "Failed to lock tank mutex.", "");
    error:
    return result;
}

void tank_turn(Tank *tank, double turn_angle)
{
    assert(tank && "Bad tank pointer.");
    assert(-M_PI <= turn_angle && turn_angle <= M_PI && "Bad turn angle.");
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

void tank_set_engine_power(Tank *tank, int power)
{
    assert(tank && "Bad tank pointer.");

    if (TANK_MIN_ENGINE_POWER > power)
    {
        power = TANK_MIN_ENGINE_POWER;
    }
    else if (TANK_MAX_ENGINE_POWER < power)
    {
        power = TANK_MAX_ENGINE_POWER;
    }

    tank->engine_power_target = power;
}

double tank_get_heading(Tank *tank)
{
    assert(tank && "Bad tank pointer.");
    double x = tank->direction.x;
    double y = tank->direction.y;
    double s = x + y;
    y /= s;
    x /= s;

    if (x >= 0)
    {
        return M_PI_2 - asin(y);
    }

    if (x < 0)
    {
        return M_PI_2 + asin(y) + M_PI;
    }

    sentinel("Mustn't be here.", "");
    error:
    return 0.0;
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
        tank->turn_angle_target = 0.0;
    }
    else
    {
        step_turn_angle = TANK_MAX_TURN_SPEED;
        tank->turn_angle_target += (-1.0) * TANK_MAX_TURN_SPEED * (signbit(tank->turn_angle_target) ? -1.0 : 1.0);
    }

    VECTOR_ROTATE(&tank->direction, &tank->orientation, step_turn_angle);
}

void tank_change_engine_power(Tank *tank)
{
    assert(tank && "Bad tank pointer.");

    if (tank->engine_power_target == tank->engine_power)
    {
        return;
    }

    if (abs(tank->engine_power_target - tank->engine_power) <= TANK_ENGINE_POWER_CHANGE_STEP)
    {
        tank->engine_power = tank->engine_power_target;
        return;
    }

    if (tank->engine_power_target < tank->engine_power)
    {
        tank->engine_power -= TANK_ENGINE_POWER_CHANGE_STEP;
    }
    else
    {
        tank->engine_power += TANK_ENGINE_POWER_CHANGE_STEP;
    }
}

bool tank_move(Tank *tank, const Landscape *l)
{
    assert(tank && "Bad tank pointer.");

    tank->previous_position = tank->position;

    if (0 == tank->engine_power)
    { // We hold brakes.
        tank->speed = 0.0;
        return true;
    }

    tank->speed = TANK_ENGINE_POWER_TO_SPEED_COEFFICIENT * tank->engine_power;

    double k = 1.0;
    while (true)
    {
        Vector t;
        vector_scale(&tank->direction, k * tank->speed, &t);
        vector_add(&tank->previous_position, &t, &tank->position);

        if (tank->position.x < 0.0 ||
            tank->position.y < 0.0 ||
            tank->position.x > l->landscape_size * l->tile_size ||
            tank->position.y > l->landscape_size * l->tile_size)
        {
            tank->position = tank->previous_position;
            return false;
        }

        tank->position.z = landscape_get_height_at(l, tank->position.x, tank->position.y);

        vector_sub(&tank->position, &tank->previous_position, &t);
        double new_speed = vector_length(&t);

        if (fabs(new_speed - tank->speed) < vector_eps)
        {
            break;
        }

        if (new_speed > tank->speed)
        {
            k -= k / 2.0;
        }
        else
        {
            k += k / 2.0;
        }

        break;
    }

    Vector old_orientation = tank->orientation;
    landscape_get_normal_at(l, tank->position.x, tank->position.y, &tank->orientation);

    if (0 == memcmp(&old_orientation, &tank->orientation, sizeof(Vector)))
    {
        return true;
    }

    Vector axis;
    vector_vector_mul(&old_orientation, &tank->orientation, &axis);
    VECTOR_NORMALIZE(&axis);
    double angle = vector_angle(&tank->orientation, &old_orientation);
    VECTOR_ROTATE(&tank->direction, &axis, angle);
    return true;
}

void tank_rotate_turret(Tank *tank)
{
    assert(tank && "Bad tank pointer.");

    if (vector_eq(&tank->turret_direction_target, &tank->turret_direction))
    {
        return;
    }

    double actual_angle = vector_angle(&tank->turret_direction, &tank->turret_direction_target);
    if (fabs(actual_angle) <= TANK_MAX_TURRET_TURN_SPEED)
    {
        tank->turret_direction = tank->turret_direction_target;
        return;
    }

    Vector axis;
    vector_vector_mul(&tank->turret_direction, &tank->turret_direction_target, &axis);
    vector_rotate(&tank->turret_direction, &axis, TANK_MAX_TURRET_TURN_SPEED, &tank->turret_direction);
}
