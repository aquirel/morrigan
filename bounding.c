// bounding.c - collision detection.

#include <assert.h>
#include <stdbool.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "morrigan.h"
#include "minmax.h"
#include "debug.h"
#include "bounding.h"
#include "landscape.h"

static void __bounding_get_effective_position(const Bounding *b, Vector *result);
static void __box_get_vertices(const Bounding *box, Vector *vertices);
static void __bounding_get_intersection_axes(const Bounding *b, Vector *axes);
static void __assert_bounding(const Bounding *box, BoundingType bounding_type);
static void __swap(double *a, double *b);
static bool __bounding_axis_test(const Bounding *b1, const Bounding *b2, const Vector *axis, double *intersection_time);
static double __get_min_intersection_time(const double *intersection_times, const bool *intersection_facts, size_t intersection_times_count);

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
    __bounding_get_effective_position(box, &p);
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
    __bounding_get_effective_position(sphere, &p);
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

void project_bounding_on_axis(const Bounding *b, const Vector *axis, double *projection_start, double *projection_end)
{
    assert(b && "Bad bounding pointer.");
    assert(axis && "Bad axis pointer.");
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

void project_box_on_axis(const Bounding *box, const Vector *axis, double *projection_start, double *projection_end)
{
    __assert_bounding(box, bounding_box);
    assert(axis && "Bad axis pointer.");
    assert(projection_start && projection_end && "Bad projection pointers.");

    Vector vertices[8];
    __box_get_vertices(box, vertices);

    int min_index = 0,
        max_index = 0;

    for (size_t v = 1; v < sizeof(vertices) / sizeof(vertices[0]); v++)
    {
        double current_min = vector_mul(&vertices[min_index], axis),
               current_max = vector_mul(&vertices[max_index], axis),
               new_value   = vector_mul(&vertices[v], axis);

        if (new_value < current_min)
        {
            min_index = v;
        }
        else if (new_value > current_max)
        {
            max_index = v;
        }
    }

    *projection_start = vector_mul(&vertices[min_index], axis);
    *projection_end   = vector_mul(&vertices[max_index], axis);
}

void project_sphere_on_axis(const Bounding *sphere, const Vector *axis, double *projection_start, double *projection_end)
{
    __assert_bounding(sphere, bounding_sphere);
    assert(axis && "Bad axis pointer.");
    assert(projection_start && projection_end && "Bad projection pointers.");

    Vector p;
    __bounding_get_effective_position(sphere, &p);

    double c = vector_mul(&p, axis);

    *projection_start = c - sphere->data.radius;
    *projection_end   = c + sphere->data.radius;
}

bool projections_are_intersecting(double projection1_start,
                                  double projection1_end,
                                  double projection1_speed,
                                  double projection2_start,
                                  double projection2_end,
                                  double projection2_speed,
                                  double *intersection_time)
{
    assert(projection1_start <= projection1_end &&
           projection2_start <= projection2_end &&
           "Bad projections.");
    assert(intersection_time && "Bad intersection time pointer.");

    bool intersection = !(projection1_end < projection2_start || projection2_end < projection1_start);
    if (intersection)
    {
        *intersection_time = nan(NULL);
        return true;
    }

    if (projection2_end < projection1_start)
    {
        __swap(&projection1_start, &projection2_start);
        __swap(&projection1_end, &projection2_end);
        __swap(&projection1_speed, &projection2_speed);
    }

    assert(projection1_end < projection2_start);

    if (projection1_speed <= 0.0 && projection2_speed >= 0.0)
    {
        *intersection_time = nan(NULL);
        return false;
    }

    double relative_speed;

    if (projection1_speed >= 0.0 && projection2_speed <= 0.0)
    {
        relative_speed = projection1_speed + projection2_speed;
    }
    else
    {
        relative_speed = projection1_speed - projection2_speed;
    }

    if (0.0 > relative_speed)
    {
        *intersection_time = nan(NULL);
        return false;
    }
    else if (0.0 == relative_speed)
    {
        *intersection_time = nan(NULL);
        return false;
    }

    double distance = projection2_start - projection1_end;
    *intersection_time = distance / relative_speed;
    return false;
}

bool intersection_test(const Bounding *b1, const Bounding *b2, double *intersection_time)
{
    assert(b1 && b2 && "Bad bounding pointers.");
    assert(intersection_time && "Bad intersection time pointer.");

    *intersection_time = nan(NULL);

    if (bounding_composite == b2->bounding_type &&
        bounding_composite != b1->bounding_type)
    {
        const Bounding *t = b1;
        b1 = b2;
        b2 = t;
    }

    if (bounding_composite == b1->bounding_type)
    {
        size_t intersections_count = b1->data.composite_data.children_count;
        double *intersection_times = _alloca(intersections_count * sizeof(double)); // compiler bug?
        bool *intersection_facts = _alloca(intersections_count * sizeof(bool)); // compiler bug?
        bool composite_intersection_result = false;

        for (size_t i = 0; i < intersections_count; i++)
        {
            intersection_times[i] = nan(NULL);
            intersection_facts[i] = intersection_test(b2, &b1->data.composite_data.children[i], &intersection_times[i]);
            composite_intersection_result |= intersection_facts[i];
        }

        *intersection_time = __get_min_intersection_time(intersection_times, intersection_facts, intersections_count);
        return composite_intersection_result;
    }

    assert(bounding_composite != b1->bounding_type &&
           bounding_composite != b2->bounding_type &&
           "Can't test composite boundings.");

    Vector axes[6];
    double intersection_times[15]; // 6 + 3 * 3.
    bool intersection_facts[15];

    __bounding_get_intersection_axes(b1, &axes[0]);
    __bounding_get_intersection_axes(b2, &axes[3]);

    bool intersection_result = true;
    for (size_t i = 0; i < 6; i++)
    {
        intersection_times[i] = nan(NULL);
        intersection_facts[i] = __bounding_axis_test(b1, b2, &axes[i], &intersection_times[i]);
        intersection_result &= intersection_facts[i];
    }

    size_t intersection_times_counter = 6;
    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 3; j < 6; j++)
        {
            if (vector_eq(&axes[i], &axes[j]))
            {
                continue;
            }

            Vector t;
            if (!(vector_tolerance_eq(0.0, vector_angle(&axes[i], &axes[j])) ||
                  vector_tolerance_eq(M_PI, vector_angle(&axes[i], &axes[j]))))
            {
                vector_vector_mul(&axes[i], &axes[j], &t);
            }
            else
            {
                vector_get_orthogonal(&axes[i], &t);
            }
            VECTOR_NORMALIZE(&t);

            intersection_facts[intersection_times_counter] = __bounding_axis_test(b1, b2, &t, &intersection_times[intersection_times_counter]);
            intersection_result &= intersection_facts[intersection_times_counter];
            intersection_times_counter++;
        }
    }

    *intersection_time = __get_min_intersection_time(intersection_times, intersection_facts, intersection_times_counter);
    return intersection_result;
}

static double __get_min_intersection_time(const double *intersection_times, const bool *intersection_facts, size_t intersection_times_count)
{
    assert(intersection_times && "Bad intersection times pointer.");
    assert(intersection_times_count && "Bad intersection count.");

    int min_intersection_time = -1;
    for (size_t i = 0; i < intersection_times_count; i++)
    {
        if (isnan(intersection_times[i]) || intersection_facts[i])
        {
            continue;
        }

        if (-1 == min_intersection_time ||
            intersection_times[i] < intersection_times[min_intersection_time])
        {
            min_intersection_time = i;
        }
    }

    if (-1 != min_intersection_time)
    {
        return intersection_times[min_intersection_time];
    }

    return nan(NULL);
}

static bool __bounding_axis_test(const Bounding *b1, const Bounding *b2, const Vector *axis, double *intersection_time)
{
    assert(b1 &&
           bounding_composite != b1->bounding_type &&
           b2 &&
           bounding_composite != b2->bounding_type &&
           "Bad boundings.");
    assert(axis && "Bad axis pointer.");
    assert(intersection_time && "Bad intersection time pointer.");

    double p1s, p1e, p2s, p2e;
    double b1_speed = vector_mul(b1->direction, axis) * (*b1->speed),
           b2_speed = vector_mul(b2->direction, axis) * (*b2->speed);

    project_bounding_on_axis(b1, axis, &p1s, &p1e);
    project_bounding_on_axis(b2, axis, &p2s, &p2e);

    return projections_are_intersecting(p1s, p1e, b1_speed, p2s, p2e, b2_speed, intersection_time);
}

static void __bounding_get_intersection_axes(const Bounding *b, Vector *axes)
{
    assert(b && "Bad bounding pointer.");
    assert(bounding_composite != b->bounding_type && "Can't get axes of composite bounding.");
    assert(axes && "Bad axes pointer.");

    axes[0] = *b->direction;
    axes[1] = *b->orientation;
    if (!vector_eq(b->direction, b->orientation))
    {
        vector_vector_mul(b->direction, b->orientation, &axes[2]);
    }
    else
    {
        vector_get_orthogonal(b->direction, &axes[2]);
    }
    VECTOR_NORMALIZE(&axes[2]);
}

void intersection_resolve(const Bounding *b1, const Bounding *b2)
{
    assert(b1 && b2 && "Bad bounding pointers.");
    memcpy(b1->origin, b1->previous_origin, sizeof(Vector));
    memcpy(b2->origin, b2->previous_origin, sizeof(Vector));
}

static void __bounding_get_effective_position(const Bounding *b, Vector *result)
{
    assert(b && "Bad bounding pointer.");
    assert(b->origin && b->previous_origin && b->orientation && b->direction && "Bad bounding data.");
    assert(result && "Bad result pointer.");

    Vector p = *b->origin,
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

static void __box_get_vertices(const Bounding *box, Vector *vertices)
{
    __assert_bounding(box, bounding_box);
    assert(vertices && "Bad vertices pointer.");

    Vector p;
    __bounding_get_effective_position(box, &p);

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

static void __swap(double *a, double *b)
{
    double t = *a;
    *a = *b;
    *b = t;
}

#if defined(BOUNDING_TESTS)
#include <stdio.h>

#include "tank.h"
#include "shell.h"

#include "testhelp.h"

int main(void)
{
    Landscape *l = landscape_create(2, 1, 1.0);

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

    double speed = 0;
    Bounding composite_members[] =
    {
        {
            .origin = &origin,
            .previous_origin = &origin,
            .orientation = &orientation,
            .direction = &direction,
            .offset = { .x = 0, .y = 0, .z = 0 },
            .speed = &speed,
            .bounding_type = bounding_box,
            .data = { .extent = { .x = 2, .y = 2, .z = 2 } }
        },
        {
            .origin = &origin,
            .previous_origin = &origin,
            .orientation = &orientation,
            .direction = &direction,
            .offset = { .x = 0, .y = 0, .z = 1 },
            .speed = &speed,
            .bounding_type = bounding_sphere,
            .data = { .radius = 1 }
        }
    };

    b.bounding_type = bounding_composite;
    b.data.composite_data.children = composite_members;
    b.data.composite_data.children_count = 2;

    test_cond("Composite landscape test.", bounding_intersects_with_landscape(l, &b));

    double t;
    test_cond("Test intersection 1.", intersection_test(&composite_members[0], &composite_members[1], &t));
    composite_members[1].offset.z = 3.5;
    test_cond("Test intersection 2.", !intersection_test(&composite_members[0], &composite_members[1], &t));

    landscape_destroy(l);

    Tank tank;
    tank_initialize(&tank, &(Vector) { .x = 0, .y = 0, .z = 0 }, &(Vector) { .x = 0, .y = 0, .z = 1 }, 0);
    Shell *s = shell_create(&(Vector) { .x = 20, .y = 0, .z = 0.5 }, &(Vector) { .x = -1, .y = 0, .z = 0 });

    bool result = intersection_test(&tank.bounding, &s->bounding, &t);
    test_cond("Test intersection 3.", !result && !isnan(t));

    s->direction.x = 1.0;
    result = intersection_test(&tank.bounding, &s->bounding, &t);
    test_cond("Test intersection 4.", !result && isnan(t));

    s->position.x = -20;
    s->position.y = -20;
    s->direction.x = 1;
    s->direction.y = 1;
    VECTOR_NORMALIZE(&s->direction);
    result = intersection_test(&tank.bounding, &s->bounding, &t);
    test_cond("Test intersection 5.", !result && !isnan(t));

    test_report();
    return EXIT_SUCCESS;
}

#endif
