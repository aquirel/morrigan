﻿// game.h - game main loop.

#pragma once
#ifndef __GAME_H__
#define __GAME_H__

#include <stdbool.h>

#include "morrigan.h"
#include "dynamic_array.h"
#include "landscape.h"

// 0.1 sec.
#define GAME_TICK_DURATION 1000000

extern const Landscape *landscape;

bool game_start(const Landscape *l, DynamicArray *c);
void game_stop(void);

#endif /* __GAME_H__ */
