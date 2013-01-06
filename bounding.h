// bounding.h - collision detection.

#ifndef __BOUNDING_H__
#define __BOUNDING_H__

#include <stdbool.h>
#include <stdlib.h>

#include "vector.h"
#include "landscape.h"

typedef enum BoundingType
{
    bounding_box,
    bounding_sphere,
    bounding_composite
} BoundingType;

typedef struct Bounding
{
    Vector *origin;
    Vector *previous_origin;
    Vector offset;
    BoundingType bounding_type;
    union
    {
        Vector extent;
        double radius;
        union
        {
            Bounding *children;
            size_t children_count;
        }
    } data;
} Bounding;

bool box_intersects_with_landscape(const Landscape *l, const *Bounding box);
bool sphere_intersects_with_landscape(const Landscape *l, const *Bounding sphere);
bool composite_intersects_with_landscape(const Landscape *l, const *Bounding composite);

void project_bounding_on_axis(const *Bounding b, int axis, double *projection_start, double *projection_end)
void project_box_on_axis(const *Bounding box, int axis, double *projection_start, double *projection_end);
void project_sphere_on_axis(const *Bounding sphere, int axis, double *projection_start, double *projection_end);
bool projections_are_intersecting(double projection1_start, double projection1_end, double projection2_start, double projection2_end);

bool intersection_test(const Bounding *b1, const BoundingBox *b2);
void intersection_solve(const Bounding *b1, const BoundingBox *b2);

#endif
