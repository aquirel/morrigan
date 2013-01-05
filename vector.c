// vector.c - vector implementation.

#include <assert.h>
#include <math.h>

#include "vector.h"
#include "matrix.h"

double vector_eps = 1e-10;

static inline bool __tolerance_eq(double v1, double v2);

bool vector_eq(const Vector *v1, const Vector *v2)
{
    assert(v1 && v2 && "Bad vector pointers.");
    return __tolerance_eq(v1->x, v2->x) &&
           __tolerance_eq(v1->y, v2->y) &&
           __tolerance_eq(v1->z, v2->z);
}

bool vector2_eq(const Vector *v1, const Vector *v2)
{
    assert(v1 && v2 && "Bad vector pointers.");
    return __tolerance_eq(v1->x, v2->x) &&
           __tolerance_eq(v1->y, v2->y);
}

Vector *vector_add(const Vector *v1, const Vector *v2, Vector *result)
{
    assert(v1 && v2 && result && "Bad vector pointers.");
    result->x = v1->x + v2->x;
    result->y = v1->y + v2->y;
    result->z = v1->z + v2->z;
    return result;
}

Vector *vector_sub(const Vector *v1, const Vector *v2, Vector *result)
{
    assert(v1 && v2 && result && "Bad vector pointers.");
    result->x = v1->x - v2->x;
    result->y = v1->y - v2->y;
    result->z = v1->z - v2->z;
    return result;
}

double vector_mul(const Vector *v1, const Vector *v2)
{
    assert(v1 && v2 && "Bad vector pointers.");
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

Vector *vector_scale(const Vector *v, double factor, Vector *result)
{
    assert(v && result && "Bad vector pointers.");
    result->x = v->x * factor;
    result->y = v->y * factor;
    result->z = v->z * factor;
    return result;
}

double vector_length(const Vector *v)
{
    assert(v && "Bad vector pointer.");
    return sqrt(pow(v->x, 2.0) + pow(v->y, 2.0) + pow(v->z, 2.0));
}

Vector *vector_normalize(const Vector *v, Vector *result)
{
    assert(v && result && "Bad vector pointers.");
    return vector_scale(v, 1.0 / vector_length(v), result);
}

double vector_distance(const Vector *v1, const Vector *v2)
{
    assert(v1 && v2 && "Bad vector pointers.");
    Vector t;
    vector_sub(v1, v2, &t);
    return vector_length(&t);
}

Vector *vector_reflect(const Vector *v, const Vector *normal, Vector *result)
{
    assert(v && normal && result && "Bad vector pointers.");
    double an = vector_mul(v, normal);
    Vector t;
    vector_scale(normal, -2.0 * an, &t);
    return vector_add(v, &t, result);
}

Vector *vector_vector_mul(const Vector *v1, const Vector *v2, Vector *result)
{
    assert(v1 && v2 && result && "Bad vector pointers.");
    result->x = v1->y * v2->z - v1->z * v2->y;
    result->y = v1->z * v2->x - v1->x * v2->z;
    result->z = v1->x * v2->y - v1->y * v2->x;
    return result;
}

Vector *vector_rotate(const Vector *v, const Vector *axis, double angle, Vector *result)
{
    assert(v && axis && result && "Bad vector pointers.");

    double x = axis->x;
    double y = axis->y;
    double z = axis->z;
    double c = cos(angle);
    double s = sin(angle);

    Matrix rotation_matrix;
    rotation_matrix.values[0][0] = c + (1 - c) * x * x;
    rotation_matrix.values[0][1] = (1 - c) * x * y - s * z;
    rotation_matrix.values[0][2] = (1 - c) * x * z + s * y;
    rotation_matrix.values[1][0] = (1 - c) * y * x + s * z;
    rotation_matrix.values[1][1] = c + (1 - c) * y * y;
    rotation_matrix.values[1][2] = (1 - c) * y * z - s * x;
    rotation_matrix.values[2][0] = (1 - c) * z * x - s * y;
    rotation_matrix.values[2][1] = (1 - c) * z * y + s * x;
    rotation_matrix.values[2][2] = c + (1 - c) * z * z;

    return matrix_vector_mul(&rotation_matrix, v, result);
}

Vector *vector_zero(Vector *v)
{
    assert(v && "Bad vector pointer.");
    v->x = v->y = v->z = 0.0;
    return v;
}

double vector_angle(const Vector *v1, const Vector *v2)
{
    assert(v1 && v2 && "Bad vector pointer.");
    return vector_mul(v1, v2) / (vector_length(v1) * vector_length(v2));
}

static inline bool __tolerance_eq(double v1, double v2)
{
    return fabs(v1 - v2) <= vector_eps;
}
