// shell.c - shell implementation.

#include <assert.h>

#include "debug.h"
#include "shell.h"

static bool __shell_position_tester(const Landscape *l, const Vector *position);

Shell *shell_create(const Vector *position, const Vector *direction)
{
    assert(position && direction && "Bad geometry pointers.");

    Shell *shell = NULL;
    check_mem(shell = calloc(1, sizeof(Shell)));

    *shell = (Shell) {
        .position          = { .x = position->x,  .y = position->y,  .z = position->z  },
        .previous_position = { .x = position->x,  .y = position->y,  .z = position->z  },
        .direction         = { .x = direction->x, .y = direction->y, .z = direction->z },
        .speed             = SHELL_DEFAULT_SPEED,
        .bounding = {
            .origin          = &shell->position,
            .previous_origin = &shell->previous_position,
            .direction       = &shell->direction,
            .orientation     = &shell->direction,
            .offset          = { .x = 0, .y = 0, .z = 0 },
            .bounding_type   = bounding_sphere,
            .data            = { .radius = SHELL_RADIUS }
        }
    };

    return shell;
    error:
    return NULL;
}

bool shell_tick(Shell *shell, const Landscape *l)
{
    assert(shell && "Bad shell pointer.");
    assert(l && "Bad landscape pointer.");

    shell->previous_position = shell->position;

    Vector t;
    vector_scale(&shell->direction, shell->speed, &t);
    VECTOR_ADD(&shell->position, &t);

    t.x = t.y = 0.0;
    t.z = -SHELL_G_ACCELERATION;
    VECTOR_ADD(&shell->position, &t);

    if (__shell_position_tester(l, &shell->position))
    {
        return true;
    }

    assert(__shell_position_tester(l, &shell->previous_position) && "Bad previous position.");

    vector_sub(&shell->position, &shell->previous_position, &t);
    Vector d = t;
    VECTOR_NORMALIZE(&d);
    double length = vector_length(&t),
           left_length  = 0.0,
           right_length = 1.0;;

    while (!vector_tolerance_eq(right_length, left_length))
    {
        double c = (left_length + right_length) / 2.0;
        vector_scale(&d, c * length, &t);
        vector_add(&shell->previous_position, &t, &t);

        if (__shell_position_tester(l, &t))
        {
            left_length = c;
        }
        else
        {
            right_length = c;
        }
    }

    double c = (left_length + right_length) / 2.0;
    vector_scale(&d, c * length, &t);
    vector_add(&shell->previous_position, &t, &shell->position);
    return false;
}

static bool __shell_position_tester(const Landscape *l, const Vector *position)
{
    if (0.0 > position->x ||
        0.0 > position->y ||
        l->tile_size * l->landscape_size < position->x ||
        l->tile_size * l->landscape_size < position->y)
    {
        return false;
    }

    double h = landscape_get_height_at(l, position->x, position->y);
    return h < position->z;
}
