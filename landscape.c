// landscape.c - landscape implementation.

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include "landscape.h"
#include "debug.h"

static inline void __validate_location(const Landscape *l, size_t x, size_t y);
static inline void __get_location_triangle(const Landscape *l, double x, double y, Vector *a, Vector *b, Vector *c);

Landscape *landscape_load(const char *filename, size_t tile_size, double scale)
{
    fprintf(stderr, "landscape_load.\n");
    FILE *landscape_file = fopen(filename, "rb");
    check(landscape_file, "Failed to open landscape file.", "");
    check(0 == fseek(landscape_file, 0, SEEK_END), "fseek() failed.", "");

    int file_size = ftell(landscape_file);
    check(0 < file_size, "ftell() failed.", "");
    int landscape_size = sqrt(file_size);
    fprintf(stderr, "landscape_size = %d.\n", landscape_size);
    check(file_size == landscape_size * landscape_size, "Landscape isn't square.", "");

    Landscape *l = landscape_create(landscape_size, tile_size);
    check_mem(l);

    rewind(landscape_file);

    double *p = l->height_map;
    for (size_t i = 0; i < file_size; i++)
    {
        uint8_t t;
        check(1 == fread(&t, 1, 1, landscape_file), "fread() failed.", "");
        p[i] = scale * t;
    }

    fclose(landscape_file);
    fprintf(stderr, "landscape_load end.\n");
    return l;

    error:
    fprintf(stderr, "landscape_load error.\n");
    if (landscape_file)
    {
        fclose(landscape_file);

        if (l)
        {
            landscape_destroy(l);
        }
    }

    return NULL;
}

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
    fprintf(stderr, "landscape_destroy start.\n");
    assert(l && l->height_map && "Nothing to destroy.");
    free(l->height_map);
    free(l);
    fprintf(stderr, "landscape_destroy end.\n");
}

double landscape_get_height_at_node(const Landscape *l, size_t y, size_t x)
{
    __validate_location(l, x, y);
    return l->height_map[y * l->landscape_size + x];
}

void landscape_set_height_at_node(const Landscape *l, size_t y, size_t x, double h)
{
    __validate_location(l, x, y);
    l->height_map[y * l->landscape_size + x] = h;
}

double landscape_get_height_at(const Landscape *l, double x, double y)
{
    assert(l && "Bad landscape pointer.");
    assert(x >= 0 && y >= 0 && "Negative position values.");

    Vector a, b, c;
    __get_location_triangle(l, x, y, &a, &b, &c);

    double plane_a = -(c.y * b.z - a.y * b.z - c.y * a.z + a.z * b.y + c.z * a.y - b.y * c.z);
    double plane_b = (a.y * c.x + b.y * a.x + c.y * b.x - b.y * c.x - a.y * b.x - c.y * a.x);
    double plane_c = (b.z * c.x + a.z * b.x + c.z * a.x - a.z * c.x - b.z * a.x - b.x * c.z);
    double plane_d = -plane_a * a.x - plane_b * a.z - plane_c * a.y;

    x /= l->tile_size;
    y /= l->tile_size;
    return -(plane_a * x + plane_c * y + plane_d) / plane_b;
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

void landscape_get_tile(const Landscape *l, double x, double y, size_t *tile_x, size_t *tile_y)
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
    landscape_get_tile(l, x, y, &t_x, &t_y);
    __validate_location(l, t_x, t_y);

    a->x = t_x + 1.0;
    a->y = t_y;
    a->z = landscape_get_height_at_node(l, (size_t) a->y, (size_t) a->x);

    b->x = t_x;
    b->y = t_y + 1.0;
    b->z = landscape_get_height_at_node(l, (size_t) b->y, (size_t) b->x);

    Vector c1 = { .x = t_x + 1, .y = t_y + 1, .z = 0 },
           c2 = { .x = t_x, .y = t_y, .z = 0 };

    *c = x - trunc(x) + y - trunc(y) < 1.0 ? c2 : c1;
    c->z = landscape_get_height_at_node(l, (size_t) c->y, (size_t) c->x);
}

#if defined(LANDSCAPE_TESTS)
#include <stdio.h>

#include "vector.h"
#include "testhelp.h"

int main(void)
{
    Landscape *l = landscape_create(2, 4);
    test_cond("Crete landscape.", l);

    landscape_set_height_at_node(l, 0, 0, 0.1);
    landscape_set_height_at_node(l, 0, 1, 0.2);
    landscape_set_height_at_node(l, 1, 0, 0.1);
    landscape_set_height_at_node(l, 1, 1, 0.2);

    test_cond("Get height 1.", vector_tolerance_eq(0.1, landscape_get_height_at(l, 0, 0)));
    test_cond("Get height 2.", vector_tolerance_eq(0.1, landscape_get_height_at(l, 0, 4 - vector_eps)));
    test_cond("Get height 3.", vector_tolerance_eq(0.2, landscape_get_height_at(l, 4 - vector_eps, 0)));
    test_cond("Get height 4.", vector_tolerance_eq(0.2, landscape_get_height_at(l, 4 - vector_eps, 4 - vector_eps)));
    test_cond("Get height 5.", vector_tolerance_eq(0.15, landscape_get_height_at(l, 2 - vector_eps, 0)));

    landscape_destroy(l);
    l = landscape_create(2, 4);
    landscape_set_height_at_node(l, 0, 0, 0.0);
    landscape_set_height_at_node(l, 0, 1, 1.0);
    landscape_set_height_at_node(l, 1, 0, 0.0);
    landscape_set_height_at_node(l, 1, 1, 1.0);

    Vector n, v = { .x = -1, .y = 0, .z = 1 };
    VECTOR_NORMALIZE(&v);

    landscape_get_normal_at(l, 0.5, 0.5, &n);
    test_cond("Get normal 1.", vector_eq(&n, &v));
    landscape_get_normal_at(l, 0, 0, &n);
    test_cond("Get normal 1.", vector_eq(&n, &v));

    landscape_destroy(l);
    test_report();
    return EXIT_SUCCESS;
}

#endif
