// shell.h - shell definition.

#pragma once
#ifndef __SHELL_H__
#define __SHELL_H__

#pragma message("__SHELL_H__")

#include <stdbool.h>

#include "vector.h"
#include "landscape.h"
#include "bounding.h"

#define SHELL_DEFAULT_SPEED 128.0
#define SHELL_RADIUS 0.1
#define SHELL_G_ACCELERATION 0.5
#define SHELL_HIT_AMOUNT 20
#define SHELL_EXPLOSION_DAMAGE 15
#define SHELL_EXPLOSION_RADIUS 20

#pragma pack(push, 8)

typedef struct Shell
{
    Vector position;
    Vector previous_position;
    Vector direction;
    Bounding bounding;
    double speed;
} Shell;

#pragma pack(pop)

Shell *shell_create(const Vector *position, const Vector *direction);
bool shell_tick(Shell *shell, const Landscape *landscape);

#endif /* __SHELL_H__ */
