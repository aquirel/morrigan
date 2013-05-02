// bounding.h - collision detection.

#pragma once
#ifndef __BOUNDING_H__
#define __BOUNDING_H__

//#pragma message("__BOUNDING_H__")

#include <stdbool.h>
#include <stdlib.h>

#include "morrigan.h"
#include "vector.h"
#include "landscape.h"

typedef enum BoundingType
{
    bounding_box,
    bounding_sphere,
    bounding_composite
} BoundingType;

#pragma pack(push, 4)

typedef struct Bounding Bounding;

typedef struct CompositeBoundingData
{
    Bounding *children;
    size_t children_count;
} CompositeBoundingData;

typedef struct Bounding
{
    Vector *origin;
    Vector *previous_origin;
    Vector *orientation;
    Vector *direction;
    Vector offset;
    BoundingType bounding_type;
    union
    {
        Vector extent;
        double radius;
        CompositeBoundingData composite_data;
    } data;
} Bounding;

#pragma pack(pop)

bool bounding_intersects_with_landscape(const Landscape *l, const Bounding *b);
bool box_intersects_with_landscape(const Landscape *l, const Bounding *box);
bool sphere_intersects_with_landscape(const Landscape *l, const Bounding *sphere);
bool composite_intersects_with_landscape(const Landscape *l, const Bounding *composite);

void project_bounding_on_axis(const Bounding *b, Axis axis, double *projection_start, double *projection_end);
void project_box_on_axis(const Bounding *box, Axis axis, double *projection_start, double *projection_end);
void project_sphere_on_axis(const Bounding *sphere, Axis axis, double *projection_start, double *projection_end);
bool projections_are_intersecting(double projection1_start, double projection1_end, double projection2_start, double projection2_end);

bool intersection_test(const Bounding *b1, const Bounding *b2);
void intersection_resolve(const Bounding *b1, const Bounding *b2);

#endif /* __BOUNDING_H__ */
