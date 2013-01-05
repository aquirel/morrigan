// landscape.c - landscape implementation.

#include <assert.h>
#include <math.h>

#include "landscape.h"
#include "debug.h"

static inline void __get_tile(const Landscape *l, double x, double y, size_t *tile_x, size_t *tile_y);
static inline void __validate_location(const Landscape *l, size_t x, size_t y);
static inline void __get_location_triangle(const Landscape *l, double x, double y, Vector *a, Vector *b, Vector *c);
static inline double __get_height_at(const Landscape *l, size_t x, size_t y);

Landscape *landscape_create(size_t landscape_size, size_t tile_size)
{
    assert(landscape_size && "Bad landscape size.");
    if (!tile_size)
    {
        tile_size = TILE_SIZE;
    }

    Landscape *l = (Landscape *) calloc(1, sizeof(Landscape));
    check_mem(l);

    l->height_map = (double *) calloc(landscape_size * landscape_size, sizeof(double));
    check_mem(l->height_map);

    l->landscape_size = landscape_size;
    l->tile_size = tile_size;

    return l;
    error:
    if (l && l->height_map)
    {
        free(l->height_map);
    }
    if (l)
    {
        free(l);
    }
    return NULL;
}

void landscape_destroy(Landscape *l)
{
    assert(l && l->height_map && "Nothing to destroy.");
    free(l->height_map);
    free(l);
}

double landscape_get_height_at(const Landscape *l, double x, double y)
{
    assert(l && "Bad landscape pointer.");
    assert(x >= 0 && y >= 0 && "Negative position values.");

    Vector a, b, c;
    __get_location_triangle(l, x, y, &a, &b, &c);
    size_t t_x, t_y;
    __get_tile(l, x, y, &t_x, &t_y);

    double plane_a = -(c.y * b.z - a.y * b.z - c.y * a.z + a.z * b.y + c.z * a.y - b.y * c.z);
    double plane_b = (a.y * c.x + b.y * a.x + c.y * b.x - b.y * c.x - a.y * b.x - c.y * a.x);
    double plane_c = (b.z * c.x + a.z * b.x + c.z * a.x - a.z * c.x - b.z * a.x - b.x * c.z);
    double plane_d = -plane_a * a.x - plane_b * a.z - plane_c * a.y;

    return -(plane_a * t_x + plane_c * t_y + plane_d) / plane_b;
}

Vector *landscape_get_normal_at(const Landscape *l, double x, double y, Vector *result)
{
    assert(l && "Bad landscape pointer.");
    assert(x >= 0 && y >= 0 && "Negative position values.");
    assert(result && "Bad result vector.");

    Vector a, b, c;
    __get_location_triangle(l, x, y, &a, &b, &c);

    Vector ab, ac;
    vector_sub(&b, &a, &ab);
    vector_sub(&c, &a, &ac);

    vector_vector_mul(&ab, &ac, result);
    VECTOR_NORMALIZE(result);

    if (0.0 > result->z)
    {
        VECTOR_SCALE(result, -1);
    }

    return result;
}

static inline void __get_tile(const Landscape *l, double x, double y, size_t *tile_x, size_t *tile_y)
{
    assert(l && "Bad landscape pointer.");
    assert(x >= 0 && y >= 0 && "Negative position values.");
    *tile_x = (size_t) (x / l->tile_size);
    *tile_y = (size_t) (y / l->tile_size);
}

static inline void __validate_location(const Landscape *l, size_t x, size_t y)
{
    assert(l && "Bad landscape pointer.");
    assert(x >= 0 && x < l->landscape_size && "X value out of range.");
    assert(y >= 0 && y < l->landscape_size && "Y value out of range.");
}

static inline void __get_location_triangle(const Landscape *l, double x, double y, Vector *a, Vector *b, Vector *c)
{
    assert(l && "Bad landscape pointer.");
    assert(x >= 0 && y >= 0 && "Negative position values.");
    assert(a && b && c && "Bad vector pointers.");

    size_t t_x, t_y;
    __get_tile(l, x, y, &t_x, &t_y);
    __validate_location(l, t_x, t_y);

    a->x = t_x + 1;
    a->y = t_y;
    a->z = __get_height_at(l, (size_t) a->y, (size_t) a->x);

    b->x = t_x;
    b->y = t_y + 1;
    b->z = __get_height_at(l, (size_t) b->y, (size_t) b->x);

    Vector c1 = { .x = t_x + 1, .y = t_y + 1, .z = 0 },
           c2 = { .x = t_x, .y = t_y, .z = 0 };

    *c = x - trunc(x) + y - trunc(y) < 1.0 ? c2 : c1;
    c->z = __get_height_at(l, (size_t) c->y, (size_t) c->x);
}

static inline double __get_height_at(const Landscape *l, size_t y, size_t x)
{
    __validate_location(l, x, y);
    return l->height_map[y * l->landscape_size + x];
}
