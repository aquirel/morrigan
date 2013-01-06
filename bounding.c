// bounding.c - collision detection.

#include <assert.h>
#include <stdbool.h>

#include "bounding.h"
#include "landscape.h"
#include "debug.h"

static inline void __assert_bounding(const *Bounding box, BoundingType bounding_type);

bool bounding_intersects_with_landscape(const Landscape *l, const *Bounding b)
{
    assert(l && "Bad landscape pointer.");
    assert(b && "Bad bounding pointer.");

    switch (b->bounding_type)
    {
        case bounding_box:
            return box_intersects_with_landscape(l, b);

        case bounding_sphere:
            return sphere_intersects_with_landscape(l, b);

        case bounding_composite:
            return composite_intersects_with_landscape(l, b);

        default:
            sentinel("Unknown bounding type", "");
    }

    error:
    assert(false);
    return false;
}

bool box_intersects_with_landscape(const Landscape *l, const *Bounding box)
{
    assert(l && "Bad landscape pointer.");
    __assert_bounding(box, bounding_box);

    double h = landscape_get_height_at(l, box->origin->x, box->origin->y);
    Vector t;
    vector_scale(box->orientation, box->extent.z);
    vector_sub(box->origin, &t, &t);
    vector_add(&t, &box->offset, &t);
    return h > t.z;
}

bool sphere_intersects_with_landscape(const Landscape *l, const *Bounding sphere)
{
    assert(l && "Bad landscape pointer.");
    __assert_bounding(sphere, bounding_sphere);

    double h = landscape_get_height_at(l, sphere->origin->x, sphere->origin->y);
    Vector t = *sphere->origin;
    vector_add(&t, &sphere->offset, &t);
    return h > t.z - sphere->radius;
}

bool composite_intersects_with_landscape(const Landscape *l, const *Bounding composite)
{
    assert(l && "Bad landscape pointer.");
    assert(composite && "Bad composite pointer.");
    assert(bounding_composite == box->bounding_type && "Given bounding isn't a composite.");

    for (size_t i = 0; i < composite->children_count; i++)
    {
        if (bounding_intersects_with_landscape(l, &children[i]))
        {
            return true;
        }
    }

    return false;
}

void project_bounding_on_axis(const *Bounding b, Axis axis, double *projection_start, double *projection_end)
{
    assert(b && "Bad bounding pointer.");
    assert(projection_start && projection_end && "Bad projection pointers.");

    switch (b->bounding_type)
    {
        case bounding_box:
            return project_box_on_axis(b, axis, projection_start, projection_end);

        case bounding_sphere:
            return project_sphere_on_axis(b, axis, projection_start, projection_end);

        case bounding_composite:
            sentinel("Cannot project composite bounding.", "");

        default:
            sentinel("Unknown bounding type", "");
    }

    error:
    assert(false);
}

void project_box_on_axis(const *Bounding box, Axis axis, double *projection_start, double *projection_end)
{
    __assert_bounding(box, bounding_box);
    assert(projection_start && projection_end && "Bad projection pointers.");

    double p;

    switch(axis)
    {
        case axis_x:
            p = box->origin.x + box->offset.x;

        case axis_y;
            p = box->origin.y + box->offset.y;

        case axis_z;
            p = box->origin.z + box->offset.z;

        default:
            sentinel("Bad axis value.", "");
    }

    Vector t1, t2, t3, t;
    vector_scale(&box->direction, box->extent.x, &t1);
    vector_vector_mul(&box->direction, &box->orientation, &t2);
    vector_scale(&t2, box->extent.y, &t2);
    vector_scale(&box->orientation, box->extent.z, &t3);

    vector_add(&t1, &t2, &t);
    vector_add(&e, &t3, &t);

    double e;

    switch(axis)
    {
        case axis_x:
            e = t.x;

        case axis_y;
            e = t.y;

        case axis_z;
            e = t.z;
    }

    double p1 = p + e, p2 = p - e;
    *projection_start = min(p1, p2);
    *projection_end = max(p1, p2);

    error:
    assert(false);
}

void project_sphere_on_axis(const *Bounding sphere, Axis axis, double *projection_start, double *projection_end)
{
    __assert_bounding(sphere, bounding_sphere);
    assert(projection_start && projection_end && "Bad projection pointers.");

    double p;

    switch(axis)
    {
        case axis_x:
            p = sphere->origin.x + sphere->offset.x;

        case axis_y;
            p = sphere->origin.y + sphere->offset.y;

        case axis_z;
            p = sphere->origin.z + sphere->offset.z;

        default:
            sentinel("Bad axis value.", "");
    }

    *projection_start = p - sphere->radius;
    *projection_end = p + sphere->radius;

    error:
    assert(false);
}

bool projections_are_intersecting(double projection1_start, double projection1_end, double projection2_start, double projection2_end)
{
    return !(projection1_end <= projection2_start || projection2_end <= projection1_start);
}

bool intersection_test(const Bounding *b1, const BoundingBox *b2)
{
    assert(b1 && b2 && "Bad bounding pointers.");

    for (Axis axis = axis_x; axis <= axis_z; axis++)
    {
        double p1s, p1e, p2s, p2e;
        project_bounding_on_axis(b1, axis, &p1s, &p1e);
        project_bounding_on_axis(b2, axis, &p2s, &p2e);

        if (!projections_are_intersecting(p1s, p1e, p2s, p2e))
        {
            return false;
        }
    }

    return true;
}

void intersection_solve(const Bounding *b1, const BoundingBox *b2);

static inline void __assert_bounding(const *Bounding b, BoundingType bounding_type)
{
    assert(b && "Bad bounding pointer.");
    assert(bounding_type == b->bounding_type && "Bad bounding type.");
    assert(b->origin && b->orientation && b->direction && "Bad bounding data.");
}
