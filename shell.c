// shell.c - shell implementation.

#include <assert.h>

#include "shell.h"

void shell_initialize(Shell *shell, const Vector *position, const Vector *direction)
{
    assert(shell && "Bad shell pointer.");
    assert(position && direction && "Bad geometry pointers.");

    *shell = (Shell) {
        .position          = { .x = position->x, .y = position->y, .z = position->z },
        .previous_position = { .x = position->x, .y = position->y, .z = position->z },
        .direction         = { .x = direction->x, .y = direction->y, .z = direction->z },
        .speed             = SHELL_DEFAULT_SPEED,
        .bounding = {
            .origin = &shell->position,
            .previous_origin = &shell->previous_position,
            .direction = &shell->direction,
            .offset = { .x = 0, .y = 0, .z = 0 },
            .bounding_type = bounding_sphere,
            .data = { .radius = SHELL_RADIUS }
        }
    };
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

    if (0.0 > shell->position.x ||
        0.0 > shell->position.y ||
        l->tile_size * l->landscape_size < shell->position.x ||
        l->tile_size * l->landscape_size < shell->position.y)
    {
        return false;
    }

    double h = landscape_get_height_at(l, shell->position.x, shell->position.y);
    return h < shell->position.z;
}

