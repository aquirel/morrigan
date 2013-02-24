// landscape.h - landscape implementation.

#pragma once
#ifndef __LANDSCAPE_H__
#define __LANDSCAPE_H__

#include <stdlib.h>

#include "morrigan.h"
#include "vector.h"

#define TILE_SIZE 16

typedef struct Landscape
{
    size_t landscape_size;
    size_t tile_size;
    double *height_map;
} Landscape;

Landscape *landscape_load(const char *filename, size_t tile_size, double scale);
Landscape *landscape_create(size_t landscape_size, size_t tile_size);
void landscape_destroy(Landscape *l);

double landscape_get_height_at_node(const Landscape *l, size_t y, size_t x);
void landscape_set_height_at_node(const Landscape *l, size_t y, size_t x, double h);

double landscape_get_height_at(const Landscape *l, double x, double y);
#define LANDSCAPE_GET_HEIGHT_AT(l, v) landscape_get_height_at((l), (v)->x, (v)->y)

Vector *landscape_get_normal_at(const Landscape *l, double x, double y, Vector *result);
#define LANDSCAPE_GET_NORMAL_AT(l, v, r) landscape_get_normal_at((l), (v)->x, (v)->y, r)

void landscape_get_tile(const Landscape *l, double x, double y, size_t *tile_x, size_t *tile_y);

#endif /* __LANDSCAPE_H__ */
