// game.h - game main loop.

#pragma once
#ifndef __GAME_H__
#define __GAME_H__

//#pragma message("__GAME_H__")

#include <stdbool.h>

#include "morrigan.h"
#include "dynamic_array.h"
#include "landscape.h"

// 0.1 sec.
#define GAME_TICK_DURATION 100000

#define NEAR_SHOOT_NOTIFICATION_RARIUS 100
#define NEAR_EXPLOSION_NOTIFICATION_RARIUS 100

bool game_start(const Landscape *l, DynamicArray *c);
void game_stop(void);

const Landscape *game_get_landscape(void);

#endif /* __GAME_H__ */
