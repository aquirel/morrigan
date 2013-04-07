// shell.h - shell definition.

#pragma once
#ifndef __SHELL_H__
#define __SHELL_H__

#include <stdbool.h>

#include "vector.h"
#include "landscape.h"
#include "bounding.h"

#define SHELL_DEFAULT_SPEED 128.0
#define SHELL_RADIUS 0.1
#define SHELL_G_ACCELERATION 0.5

typedef struct Shell
{
    Vector position;
    Vector previous_position;
    Vector direction;
    Bounding bounding;
    double speed;
} Shell;

void shell_initialize(Shell *shell, const Vector *position, const Vector *direction);
bool shell_tick(Shell *shell, const Landscape *landscape);

#endif /* __SHELL_H__ */

