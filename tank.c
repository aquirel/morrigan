// tank.c - tank definition.

#include <assert.h>
#include <stdbool.h>

#include "tank.h"
#include "landscape.h"
#include "debug.h"

static void __tank_change_turn(Tank *tank);
static void __tank_change_engine_power(Tank *tank);
static bool __tank_move(Tank *tank, const Landscape *l);
static void __tank_rotate_turret(Tank *tank);

void tank_initialize(Tank *tank, const Vector *position, const Vector *top, int team)
{
    assert(tank && "Bad tank pointer.");
    assert(position && top && "Bad geometry pointers.");

    *tank = (Tank) {
        .position          = { .x = position->x, .y = position->y, .z = position->z },
        .previous_position = { .x = position->x, .y = position->y, .z = position->z },
        .direction         = { .x  = 1, .y = 0, .z = 0 },
        .orientation       = { .x = top->x, .y = top->y, .z = top->z },
        .speed             = 0,
        .bounding_primitives = {
            {
                .origin          = &tank->position,
                .previous_origin = &tank->previous_position,
                .orientation     = &tank->orientation,
                .direction       = &tank->direction,
                .offset          = { .x = 0, .y = 0, .z = 0 },
                .speed           = &tank->speed,
                .bounding_type   = bounding_box,
                .data            = { .extent = TANK_BOUNDING_BOX_EXTENT }
            },
            {
                .origin          = &tank->position,
                .previous_origin = &tank->previous_position,
                .orientation     = &tank->orientation,
                .direction       = &tank->direction,
                .offset          = { .x = 2, .y = 0, .z = TANK_BOUNDING_SPHERE_RADIUS },
                .speed           = &tank->speed,
                .bounding_type   = bounding_sphere,
                .data            = { .radius = TANK_BOUNDING_SPHERE_RADIUS }
            }
        },
        .bounding = {
            .origin              = &tank->position,
            .previous_origin     = &tank->previous_position,
            .orientation         = &tank->orientation,
            .direction           = &tank->direction,
            .offset              = { .x = 0, .y = 0, .z = 0 },
            .speed               = &tank->speed,
            .bounding_type       = bounding_composite,
            .data.composite_data = {
                .children       = tank->bounding_primitives,
                .children_count = TANK_BOUNDING_PRIMITIVES
            }
        },
        .hp                      = TANK_HP,
        .team                    = team,
        .engine_power            = 0,
        .engine_power_target     = 0,
        .fire_delay              = 0,
        .turret_direction        = { .x = 1, .y = 0, .z = 0 },
        .turret_direction_target = { .x = 1, .y = 0, .z = 0 },
        .turn_angle_target       = 0,
        .statistics              = { .ticks = 0, .hp = 0, .direct_hits = 0, .hits = 0, .got_direct_hits = 0, .got_hits = 0 },
        .last_shell_id           = -1
    };

    tank_rotate_direction(&tank->direction, &(Vector) { .x = 0, .y = 0, .z = 1}, &tank->orientation);
}

bool tank_tick(Tank *tank, const Landscape *l)
{
    assert(tank && "Bad tank pointer.");

    tank->statistics.ticks++;
    bool result = true;

    if (0 != tank->fire_delay && -1 != tank->fire_delay)
    {
        tank->fire_delay--;
    }

    __tank_change_engine_power(tank);
    result = __tank_move(tank, l);
    __tank_change_turn(tank);
    __tank_rotate_turret(tank);

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

    Vector t = *look;
    VECTOR_NORMALIZE(&t);

    if (TANK_MIN_LOOK_Z <= t.z &&
        TANK_MAX_LOOK_Z >= t.z)
    {
        tank->turret_direction_target = t;
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

    tank->engine_power_target = (double) power;
}

bool tank_shoot(Tank *tank)
{
    assert(tank && "Bad tank pointer.");

    if (0 == tank->fire_delay)
    {
        tank->fire_delay = -1;
        return true;
    }

    return false;
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

static void __tank_change_turn(Tank *tank)
{
    assert(tank && "Bad tank pointer.");
    tank_change_turn_worker(&tank->turn_angle_target, TANK_MAX_TURN_SPEED, &tank->direction, &tank->orientation);
}

void tank_change_turn_worker(double *turn_angle_target, double max_turn_speed, Vector *direction, Vector *orientation)
{
    assert(turn_angle_target && "Bad turn angle pointer.");
    assert(isnormal(max_turn_speed) && "Bad turn max turn speed.");
    assert(direction && "Bad direction pointer.");
    assert(orientation && "Bad orientation pointer.");

    if (vector_tolerance_eq(0, *turn_angle_target))
    {
        return;
    }

    double step_turn_angle;
    if (fabs(*turn_angle_target) <= max_turn_speed)
    {
        step_turn_angle = *turn_angle_target;
        *turn_angle_target = 0.0;
    }
    else
    {
        step_turn_angle = max_turn_speed * (signbit(*turn_angle_target) ? -1.0 : 1.0);
        *turn_angle_target += (-1.0) * max_turn_speed * (signbit(*turn_angle_target) ? -1.0 : 1.0);
    }

    VECTOR_ROTATE(direction, orientation, step_turn_angle);
}

static void __tank_change_engine_power(Tank *tank)
{
    assert(tank && "Bad tank pointer.");

    if (tank->engine_power_target == tank->engine_power)
    {
        return;
    }

    if (fabs(tank->engine_power_target - tank->engine_power) <= TANK_ENGINE_POWER_CHANGE_STEP)
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

static bool __tank_move(Tank *tank, const Landscape *l)
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

        if (composite_intersects_with_landscape(l, &tank->bounding))
        {
            tank->position.z = landscape_get_height_at(l, tank->position.x, tank->position.y);
        }

        vector_sub(&tank->position, &tank->previous_position, &t);
        double new_speed = vector_length(&t);

        if (vector_tolerance_eq(new_speed, tank->speed))
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
    tank_rotate_direction(&tank->direction, &old_orientation, &tank->orientation);

    return true;
}

static void __tank_rotate_turret(Tank *tank)
{
    assert(tank && "Bad tank pointer.");
    tank_rotate_turret_worker(&tank->turret_direction_target, &tank->turret_direction, TANK_MAX_TURRET_TURN_SPEED);
}

void tank_rotate_turret_worker(Vector *turret_direction_target, Vector *turret_direction, double max_turret_turn_speed)
{
    assert(turret_direction_target && "Bad target direction pointer.");
    assert(turret_direction && "Bad direction pointer.");
    assert(isnormal(max_turret_turn_speed) && "Bad max turn speed.");

    if (vector_eq(turret_direction_target, turret_direction))
    {
        return;
    }

    Vector axis;
    double angle;

    Vector d = *turret_direction,
           t = *turret_direction_target;
    d.z = t.z = 0.0;
    VECTOR_NORMALIZE(&d);
    VECTOR_NORMALIZE(&t);

    if (!vector_eq(&d, &t))
    {
        angle = vector_angle(&d, &t);
    }
    else
    {
        d = *turret_direction;
        t = *turret_direction_target;
        VECTOR_NORMALIZE(&d);
        VECTOR_NORMALIZE(&t);
        angle = vector_angle(&d, &t);
    }

    vector_vector_mul(&d, &t, &axis);
    VECTOR_NORMALIZE(&axis);

    if (fabs(angle) > max_turret_turn_speed)
    {
        angle = max_turret_turn_speed * (signbit(angle) ? -1.0 : 1.0);
    }

    VECTOR_ROTATE(turret_direction, &axis, angle);
    VECTOR_NORMALIZE(turret_direction);
}

void tank_rotate_direction(Vector *direction, const Vector *old_orientation, const Vector *orientation)
{
    assert(direction && old_orientation && orientation && "Bad orientation pointers.");

    if (0.0 == vector_mul(orientation, direction))
    { // Already orthogonal.
        return;
    }

    // Get orientation plane.
    Vector normal;
    vector_vector_mul(old_orientation, direction, &normal);
    VECTOR_NORMALIZE(&normal);

    // Project new orientation on old plane.
    Vector p = *orientation, t;
    vector_scale(&normal, vector_mul(&normal, &p), &t);
    VECTOR_SUB(&p, &t);

    // Rotate direction.
    VECTOR_NORMALIZE(&p);
    vector_rotate(&p, &normal, M_PI_2, direction);
}
