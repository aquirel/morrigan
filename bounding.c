// bounding.c - collision detection.

#include <assert.h>
#include <stdbool.h>
#include <minmax.h>

#include "morrigan.h"
#include "bounding.h"
#include "landscape.h"
#include "debug.h"

static void __bounding_get_effective_position(const Bounding *b, Vector *result, bool use_new_origin);
static void __box_get_vertices(const Bounding *box, Vector *vertices, bool use_new_origin);
static void __assert_bounding(const Bounding *box, BoundingType bounding_type);

bool bounding_intersects_with_landscape(const Landscape *l, const Bounding *b)
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
            sentinel("Unknown bounding type.", "");
    }

    error:
    assert(false);
    return false;
}

bool box_intersects_with_landscape(const Landscape *l, const Bounding *box)
{
    assert(l && "Bad landscape pointer.");
    __assert_bounding(box, bounding_box);

    Vector p;
    __bounding_get_effective_position(box, &p, true);
    // TODO: Here get 8 vertices of box.

    double h = landscape_get_height_at(l, p.x, p.y);
    Vector t;
    vector_scale(box->orientation, box->data.extent.z, &t);
    VECTOR_SUB(&p, &t);

    return h >= p.z;
}

bool sphere_intersects_with_landscape(const Landscape *l, const Bounding *sphere)
{
    assert(l && "Bad landscape pointer.");
    __assert_bounding(sphere, bounding_sphere);

    Vector p;
    __bounding_get_effective_position(sphere, &p, true);
    // TODO: Fix it.

    double h = landscape_get_height_at(l, p.x, p.y);
    return h >= p.z - sphere->data.radius;
}

bool composite_intersects_with_landscape(const Landscape *l, const Bounding *composite)
{
    assert(l && "Bad landscape pointer.");
    assert(composite && "Bad composite pointer.");
    assert(bounding_composite == composite->bounding_type && "Given bounding isn't a composite.");

    for (size_t i = 0; i < composite->data.composite_data.children_count; i++)
    {
        if (bounding_intersects_with_landscape(l, &composite->data.composite_data.children[i]))
        {
            return true;
        }
    }

    return false;
}

void project_bounding_on_axis(const Bounding *b, Axis axis, double *projection_start, double *projection_end)
{
    assert(b && "Bad bounding pointer.");
    assert(projection_start && projection_end && "Bad projection pointers.");

    switch (b->bounding_type)
    {
        case bounding_box:
            project_box_on_axis(b, axis, projection_start, projection_end);
            return;

        case bounding_sphere:
            project_sphere_on_axis(b, axis, projection_start, projection_end);
            return;

        case bounding_composite:
            sentinel("Cannot project composite bounding.", "");

        default:
            sentinel("Unknown bounding type.", "");
    }

    error:
    assert(false);
}

void project_box_on_axis(const Bounding *box, Axis axis, double *projection_start, double *projection_end)
{
    __assert_bounding(box, bounding_box);
    assert(projection_start && projection_end && "Bad projection pointers.");

    Vector vertices[16];
    __box_get_vertices(box, &vertices[0], false);
    __box_get_vertices(box, &vertices[8], true);

    int min_index = 0,
        max_index = 0;

    for (size_t v = 1; v < 16; v++)
    {
        double current_min = get_vector_coord(&vertices[min_index], axis),
               current_max = get_vector_coord(&vertices[max_index], axis),
               new_value   = get_vector_coord(&vertices[v], axis);

        if (new_value < current_min)
        {
            min_index = v;
        }
        else if (new_value > current_max)
        {
            max_index = v;
        }
    }

    *projection_start = get_vector_coord(&vertices[min_index], axis);
    *projection_end   = get_vector_coord(&vertices[max_index], axis);
}

void project_sphere_on_axis(const Bounding *sphere, Axis axis, double *projection_start, double *projection_end)
{
    __assert_bounding(sphere, bounding_sphere);
    assert(projection_start && projection_end && "Bad projection pointers.");

    Vector p1, p2;
    __bounding_get_effective_position(sphere, &p1, false);
    __bounding_get_effective_position(sphere, &p2, true);

    double c1 = get_vector_coord(&p1, axis),
           c2 = get_vector_coord(&p2, axis);

    *projection_start = min(c1, c2) - sphere->data.radius;
    *projection_end   = max(c1, c2) + sphere->data.radius;
}

bool projections_are_intersecting(double projection1_start, double projection1_end, double projection2_start, double projection2_end)
{
    assert(projection1_start <= projection1_end &&
           projection2_start <= projection2_end &&
           "Bad projections.");
    return !(projection1_end <= projection2_start || projection2_end <= projection1_start);
}

bool intersection_test(const Bounding *b1, const Bounding *b2)
{
    assert(b1 && b2 && "Bad bounding pointers.");

    if (bounding_composite == b2->bounding_type &&
        bounding_composite != b1->bounding_type)
    {
        const Bounding *t = b1;
        b1 = b2;
        b2 = t;
    }

    if (bounding_composite == b1->bounding_type)
    {
        for (size_t i = 0; i < b1->data.composite_data.children_count; i++)
        {
            if (intersection_test(b2, &b1->data.composite_data.children[i]))
            {
                return true;
            }
        }

        return false;
    }

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

void intersection_resolve(const Bounding *b1, const Bounding *b2)
{
    assert(b1 && b2 && "Bad bounding pointers.");
    memcpy(b1->origin, b1->previous_origin, sizeof(Vector));
    memcpy(b2->origin, b2->previous_origin, sizeof(Vector));
}

static void __bounding_get_effective_position(const Bounding *b, Vector *result, bool use_new_origin)
{
    assert(b && "Bad bounding pointer.");
    assert(b->origin && b->previous_origin && b->orientation && b->direction && "Bad bounding data.");
    assert(result && "Bad result pointer.");

    Vector p = *(use_new_origin ? b->origin : b->previous_origin),
           t,
           side;
    if (!vector_eq(b->orientation, b->direction))
    {
        vector_vector_mul(b->orientation, b->direction, &side);
    }
    else
    {
        vector_get_orthogonal(b->direction, &side);
    }
    VECTOR_NORMALIZE(&side);

    double sx, sy, sz;
    vector_scale(b->direction, b->offset.x, &t);
    sx = vector_length(&t);
    vector_scale(&side, b->offset.y, &t);
    sy = vector_length(&t);
    vector_scale(b->orientation, b->offset.z, &t);
    sz = vector_length(&t);

    p.x += sx;
    p.y += sy;
    p.z += sz;

    *result = p;
}

static void __box_get_vertices(const Bounding *box, Vector *vertices, bool use_new_origin)
{
    __assert_bounding(box, bounding_box);
    assert(vertices && "Bad vertices pointer.");

    Vector p;
    __bounding_get_effective_position(box, &p, use_new_origin);

    Vector e[3];

    e[0] = *box->direction; // d.
    VECTOR_NORMALIZE(&e[0]);
    VECTOR_SCALE(&e[0], box->data.extent.x);

    vector_vector_mul(box->orientation, box->direction, &e[1]); // s.
    VECTOR_NORMALIZE(&e[1]);
    VECTOR_SCALE(&e[1], box->data.extent.y);

    e[2] = *box->orientation; // t.
    VECTOR_NORMALIZE(&e[2]);
    VECTOR_SCALE(&e[2], box->data.extent.z);

    for (size_t i = 0; i < 8; i++)
    {
        Vector v;
        vector_zero(&v);

        for (size_t j = 0; j < 3; j++)
        {
            (i & (1 << j) ? vector_add : vector_sub)(&v, &e[j], &v);
        }

        vector_add(&p, &v, &vertices[i]);
    }
}

static void __assert_bounding(const Bounding *b, BoundingType bounding_type)
{
    assert(b && "Bad bounding pointer.");
    assert(bounding_type == b->bounding_type && "Bad bounding type.");
    assert(b->origin && b->previous_origin && b->orientation && b->direction && "Bad bounding data.");
}

#if defined(BOUNDING_TESTS)
#include <stdio.h>

#include "testhelp.h"

int main(void)
{
    Landscape *l = landscape_create(2, 1);

    landscape_set_height_at_node(l, 0, 0, 0.0);
    landscape_set_height_at_node(l, 0, 1, 0.0);
    landscape_set_height_at_node(l, 1, 0, 0.0);
    landscape_set_height_at_node(l, 1, 1, 0.0);

    Vector origin = { .x = 0.5, .y = 0.5, .z = 0.5 };
    Vector orientation = { .x = 0, .y = 0, .z = 1 };
    Vector direction = { .x = 1, .y = 0, .z = 0 };
    Bounding b =
    {
        .origin = &origin,
        .previous_origin = &origin,
        .orientation = &orientation,
        .direction = &direction,
        .offset = { .x = 0, .y = 0, .z = 0 },
        .bounding_type = bounding_box,
        .data = { .extent = { .x = 1, .y = 1, .z = 1 } }
    };

    test_cond("Box landscape test 1.", bounding_intersects_with_landscape(l, &b));
    origin.z = 1.5;
    test_cond("Box landscape test 2.", !bounding_intersects_with_landscape(l, &b));
    b.bounding_type = bounding_sphere;
    b.data.radius = 3;
    test_cond("Sphere landscape test 1.", bounding_intersects_with_landscape(l, &b));
    b.data.radius = 1;
    test_cond("Sphere landscape test 2.", !bounding_intersects_with_landscape(l, &b));

    Bounding composite_members[] =
    {
        {
            .origin = &origin,
            .previous_origin = &origin,
            .orientation = &orientation,
            .direction = &direction,
            .offset = { .x = 0, .y = 0, .z = 0 },
            .bounding_type = bounding_box,
            .data = { .extent = { .x = 2, .y = 2, .z = 2 } }
        },
        {
            .origin = &origin,
            .previous_origin = &origin,
            .orientation = &orientation,
            .direction = &direction,
            .offset = { .x = 0, .y = 0, .z = 1 },
            .bounding_type = bounding_sphere,
            .data = { .radius = 1 }
        }
    };

    b.bounding_type = bounding_composite;
    b.data.composite_data.children = composite_members;
    b.data.composite_data.children_count = 2;

    test_cond("Composite landscape test.", bounding_intersects_with_landscape(l, &b));

    test_cond("Test intersection 1.", intersection_test(&composite_members[0], &composite_members[1]));
    composite_members[1].offset.z = 3;
    test_cond("Test intersection 2.", !intersection_test(&composite_members[0], &composite_members[1]));

    landscape_destroy(l);
    test_report();
    return EXIT_SUCCESS;
}

#endif
