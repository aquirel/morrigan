// landscape.c - landscape implementation.

#include <assert.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include "debug.h"
#include "minmax.h"
#include "landscape.h"

static void __validate_location(const Landscape *l, size_t x, size_t y);
static void __get_location_triangle(const Landscape *l, double x, double y, Vector *a, Vector *b, Vector *c);
static double __segment_intersects_triangle(const Vector *segment_start,
                                            const Vector *segment_end,
                                            const Vector *a,
                                            const Vector *b,
                                            const Vector *c);

Landscape *landscape_load(const char *filename, size_t tile_size, double scale)
{
    fprintf(stderr, "landscape_load.\n");
    FILE *landscape_file = fopen(filename, "rb");
    check(landscape_file, "Failed to open landscape file.", "");
    check(0 == fseek(landscape_file, 0, SEEK_END), "fseek() failed.", "");

    size_t file_size = ftell(landscape_file);
    check(0 < file_size, "ftell() failed.", "");
    size_t landscape_size = (size_t) sqrt((double) file_size);
    fprintf(stderr, "landscape_size = %d.\n", landscape_size);
    check(file_size == landscape_size * landscape_size, "Landscape isn't square.", "");

    Landscape *l = NULL;
    check_mem(l = landscape_create(landscape_size, tile_size, scale));

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
            l = NULL;
        }
    }

    return NULL;
}

Landscape *landscape_create(size_t landscape_size, size_t tile_size, double scale)
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
    l->scale = scale;

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
    result->x /= l->tile_size;
    result->y /= l->tile_size;
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

double landscape_intersects_with_segment(const Landscape *l,
                                         const Vector *segment_start,
                                         const Vector *segment_end)
{
    assert(l && "Bad landscape pointer.");
    assert(segment_start && "Bad segment start pointer.");
    assert(segment_end && "Bad segment end pointer.");

    double result = nan(NULL);
    for (size_t i = 0; i < l->landscape_size; i++)
    {
        for (size_t j = 0; j < l->landscape_size; j++)
        {
            Vector a = {
                .x = (double) (j * l->tile_size),
                .y = (double) (i * l->tile_size),
                .z = landscape_get_height_at_node(l, i, j)
            };
            Vector b = {
                .x = (double) ((j + 1) * l->tile_size),
                .y = (double) (i * l->tile_size),
                .z = landscape_get_height_at_node(l, i, j + 1)
            };
            Vector c = {
                .x = (double) (j * l->tile_size),
                .y = (double) ((i + 1) * l->tile_size),
                .z = landscape_get_height_at_node(l, i + 1, j)
            };
            Vector d = {
                .x = (double) ((j + 1) * l->tile_size),
                .y = (double) ((i + 1) * l->tile_size),
                .z = landscape_get_height_at_node(l, i + 1, j + 1)
            };

            double t = __segment_intersects_triangle(segment_start,
                                                     segment_end,
                                                     &a,
                                                     &b,
                                                     &c);
            if (!isnan(t))
            {
                if (isnan(result) || t < result)
                {
                    result = t;
                }
            }
            t = __segment_intersects_triangle(segment_start,
                                              segment_end,
                                              &b,
                                              &c,
                                              &d);
            if (!isnan(t))
            {
                if (isnan(result) || t < result)
                {
                    result = t;
                }
            }
        }
    }

    return result;
}

static void __validate_location(const Landscape *l, size_t x, size_t y)
{
    assert(l && "Bad landscape pointer.");
    assert(x >= 0 && x <= l->landscape_size && "X value out of range.");
    assert(y >= 0 && y <= l->landscape_size && "Y value out of range.");
}

static void __get_location_triangle(const Landscape *l, double x, double y, Vector *a, Vector *b, Vector *c)
{
    assert(l && "Bad landscape pointer.");
    assert(x >= 0 && y >= 0 && "Negative position values.");
    assert(a && b && c && "Bad vector pointers.");

    size_t t_x, t_y;
    landscape_get_tile(l, x, y, &t_x, &t_y);
    __validate_location(l, t_x, t_y);

    a->x = (double) t_x + 1.0;
    a->y = (double) t_y;
    a->z = landscape_get_height_at_node(l, (size_t) a->y, (size_t) a->x);

    b->x = (double) t_x;
    b->y = (double) t_y + 1.0;
    b->z = landscape_get_height_at_node(l, (size_t) b->y, (size_t) b->x);

    Vector c1 = { .x = (double) (t_x + 1), .y = (double) (t_y + 1), .z = 0.0 },
           c2 = { .x = (double) t_x, .y = (double) t_y, .z = 0.0 };

    *c = x - ((double) l->tile_size * t_x) +
         y - ((double) l->tile_size * t_y) < l->tile_size ? c2 : c1;
    c->z = landscape_get_height_at_node(l, (size_t) c->y, (size_t) c->x);
}

static double __segment_intersects_triangle(const Vector *segment_start,
                                            const Vector *segment_end,
                                            const Vector *a,
                                            const Vector *b,
                                            const Vector *c)
{
    assert(segment_start && "Bad segment start pointer.");
    assert(segment_end && "Bad segment end pointer.");
    assert(a && "Bad vertex a pointer.");
    assert(b && "Bad vertex b pointer.");
    assert(c && "Bad vertex c pointer.");

    Vector ab, ac;
    vector_sub(b, a, &ab);
    vector_sub(c, a, &ac);

    Vector n;
    vector_vector_mul(&ab, &ac, &n);
    VECTOR_NORMALIZE(&n);

    double d = -vector_mul(&n, a);

    Vector segment_direction;
    vector_sub(segment_start, segment_end, &segment_direction);

    double denominator = vector_mul(&n, &segment_direction);

    if (vector_tolerance_eq(0.0, denominator))
    {
        return nan(NULL);
    }

    double numerator = d + vector_mul(&n, segment_start),
           mu = numerator / denominator;

    if (!(0.0 <= mu && mu <= 1.0))
    {
        return nan(NULL);
    }

    Vector p;
    vector_sub(segment_end, segment_start, &p);
    VECTOR_SCALE(&p, mu);
    VECTOR_ADD(&p, segment_start);

    Vector pa1, pa2, pa3;
    vector_sub(a, &p, &pa1);
    VECTOR_NORMALIZE(&pa1);
    vector_sub(b, &p, &pa2);
    VECTOR_NORMALIZE(&pa2);
    vector_sub(c, &p, &pa3);
    VECTOR_NORMALIZE(&pa3);

    double a1    = vector_angle(&pa1, &pa2),
           a2    = vector_angle(&pa2, &pa3),
           a3    = vector_angle(&pa3, &pa1),
           angle = a1 + a2 + a3;
    assert(a1 >= 0.0 && a2 >= 0.0 && a3 >= 0.0);
    while (angle >= 2.0 * M_PI - VECTOR_EPS) angle -= 2.0 * M_PI;

    return vector_tolerance_eq(angle, 0.0) ? mu : nan(NULL);
}

#if defined(LANDSCAPE_TESTS)
#include <stdio.h>

#include "vector.h"
#include "testhelp.h"

int main(void)
{
    Landscape *l = landscape_create(2, 256, 1.0);
    test_cond("Create landscape.", l);

    landscape_set_height_at_node(l, 0, 0, 0.1);
    landscape_set_height_at_node(l, 0, 1, 0.2);
    landscape_set_height_at_node(l, 1, 0, 0.1);
    landscape_set_height_at_node(l, 1, 1, 0.2);

    test_cond("Get height 1.", vector_tolerance_eq(0.1, landscape_get_height_at(l, 0, 0)));
    test_cond("Get height 2.", vector_tolerance_eq(0.1, landscape_get_height_at(l, 0, 256 - VECTOR_EPS)));
    test_cond("Get height 3.", vector_tolerance_eq(0.2, landscape_get_height_at(l, 256 - VECTOR_EPS, 0)));
    test_cond("Get height 4.", vector_tolerance_eq(0.2, landscape_get_height_at(l, 256 - VECTOR_EPS, 256 - VECTOR_EPS)));
    test_cond("Get height 5.", vector_tolerance_eq(0.15, landscape_get_height_at(l, 128 - VECTOR_EPS, 0)));

    landscape_destroy(l);
    l = landscape_create(2, 128, 1.0);
    landscape_set_height_at_node(l, 0, 0, 0.0);
    landscape_set_height_at_node(l, 0, 1, 1.0);
    landscape_set_height_at_node(l, 1, 0, 0.0);
    landscape_set_height_at_node(l, 1, 1, 1.0);

    Vector n, v = { .x = -1.0 / 128.0, .y = 0, .z = 1 };
    VECTOR_NORMALIZE(&v);

    landscape_get_normal_at(l, 0.5, 0.5, &n);
    test_cond("Get normal 1.", vector_eq(&n, &v));
    landscape_get_normal_at(l, 0, 0, &n);
    test_cond("Get normal 2.", vector_eq(&n, &v));

    landscape_destroy(l);

    test_cond("Segment-triangle test.",
              vector_tolerance_eq(0.5,
                                  __segment_intersects_triangle(&(Vector) { .x = 1.5, .y = 0.5, .z = +1 },
                                                                &(Vector) { .x = 0.0, .y = 0.5, .z = -1 },
                                                                &(Vector) { .x = 1, .y = 0, .z = 0 },
                                                                &(Vector) { .x = 0, .y = 1, .z = 0 },
                                                                &(Vector) { .x = 1, .y = 1, .z = 0 })));

    test_report();
    return EXIT_SUCCESS;
}

#endif
