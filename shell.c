// shell.c - shell implementation.

#include <assert.h>
#include <math.h>

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
            .speed           = &shell->speed,
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

    Vector vector_speed;
    vector_scale(&shell->direction, shell->speed, &vector_speed);
    Vector g = { .x = 0, .y = 0, .z = -SHELL_G_ACCELERATION };
    VECTOR_ADD(&vector_speed, &g);

    VECTOR_ADD(&shell->position, &vector_speed);

    vector_sub(&shell->position, &shell->previous_position, &shell->direction);
    VECTOR_NORMALIZE(&shell->direction);
    shell->speed = vector_mul(&shell->direction, &vector_speed);

    double intersection = landscape_intersects_with_segment(l,
                                                            &shell->previous_position,
                                                            &shell->position);
    if (isnan(intersection))
    {
        return !(0.0 > shell->position.x ||
                 0.0 > shell->position.y ||
                 l->tile_size * l->landscape_size < shell->position.x ||
                 l->tile_size * l->landscape_size < shell->position.y);
    }

    Vector shell_direction;
    vector_sub(&shell->position, &shell->previous_position, &shell_direction);
    VECTOR_SCALE(&shell_direction, intersection);
    vector_add(&shell->previous_position, &shell_direction, &shell->position);
    return false;
}
