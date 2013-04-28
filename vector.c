// vector.c - vector implementation.

#include <assert.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "vector.h"
#include "matrix.h"

bool vector_tolerance_eq(double v1, double v2)
{
    return fabs(v1 - v2) <= VECTOR_EPS;
}

bool vector_eq(const Vector *v1, const Vector *v2)
{
    assert(v1 && v2 && "Bad vector pointers.");
    return vector_tolerance_eq(v1->x, v2->x) &&
           vector_tolerance_eq(v1->y, v2->y) &&
           vector_tolerance_eq(v1->z, v2->z);
}

bool vector2_eq(const Vector *v1, const Vector *v2)
{
    assert(v1 && v2 && "Bad vector pointers.");
    return vector_tolerance_eq(v1->x, v2->x) &&
           vector_tolerance_eq(v1->y, v2->y);
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

    double s   = sin(angle / 2.0),
           c   = cos(angle / 2.0),
           q_x = axis->x * s,
           q_y = axis->y * s,
           q_z = axis->z * s,
           q_w = c,
           q_l = sqrt(q_x * q_x + q_y * q_y + q_z * q_z + q_w * q_w);

    q_x /= q_l;
    q_y /= q_l;
    q_z /= q_l;
    q_w /= q_l;

    Matrix rotation_matrix = {
        .values = {
            { 1.0 - 2.0 * q_y * q_y - 2.0 * q_z * q_z, 2.0 * q_x * q_y - 2.0 * q_z * q_w,       2.0 * q_x * q_z + 2.0 * q_y * q_w       },
            { 2.0 * q_x * q_y + 2.0 * q_z * q_w,       1.0 - 2.0 * q_x * q_x - 2.0 * q_z * q_z, 2.0 * q_y * q_z - 2.0 * q_x * q_w       },
            { 2.0 * q_x * q_z - 2.0 * q_y * q_w,       2.0 * q_y * q_z + 2.0 * q_x * q_w      , 1.0 - 2.0 * q_x * q_x - 2.0 * q_y * q_y }
        }
    };

    return matrix_vector_mul(&rotation_matrix, v, result);
}

Vector *vector_zero(Vector *v)
{
    assert(v && "Bad vector pointer.");
    v->x = v->y = v->z = 0.0;
    return v;
}

Vector *vector_get_orthogonal(const Vector *v, Vector *result)
{
    assert(v && result && "Bad vector pointers.");

    Vector axis = { .x = 1, .y = 0, .z = 0 };
    double angle = vector_angle(&axis, v);
    if (vector_tolerance_eq(0, angle) ||
        vector_tolerance_eq(M_PI, angle))
    {
        axis.x = 0;
        axis.y = 1;
    }

    vector_vector_mul(v, &axis, result);
    return result;
}

double vector_angle(const Vector *v1, const Vector *v2)
{
    assert(v1 && v2 && "Bad vector pointer.");

    Vector _v1, _v2, _v3;

    vector_normalize(v1, &_v1);
    vector_normalize(v2, &_v2);

    double c = vector_mul(&_v1, &_v2) / (vector_length(&_v1) * vector_length(&_v2)),
           s;

    vector_vector_mul(&_v1, &_v2, &_v3);
    s = vector_length(&_v3);

    if (c > 1.0)       c = 1.0;
    else if (c < -1.0) c = -1.0;

    if (s > 1.0)       s = 1.0;
    else if (s < -1.0) s = -1.0;

    if (0.0 == s)
    {
        return c >= 0.0 ? 0.0 : M_PI;
    }

    if (0.0 == c)
    {
        return s >= 0.0 ? M_PI_2 : M_PI * 3.0 / 2.0;
    }

    double ac = acos(c),
           as = asin(s);

    return as < 0.0 ? 2.0 * M_PI - ac : ac;
}

#if defined(VECTOR_TESTS)
#include <stdio.h>

#include "testhelp.h"

int main(void)
{
    Vector a, b, t;
    a.x = a.y = a.z = 1;
    b.x = 1;
    b.y = 2;
    b.z = 3;

    vector_add(&a, &b, &t);
    test_cond("Test add.", 2 == t.x && 3 == t.y && 4 == t.z);
    vector_sub(&a, &b, &t);
    test_cond("Test sub.", 0 == t.x && -1 == t.y && -2 == t.z);
    test_cond("Test mul.", 1 + 2 + 3 == vector_mul(&a, &b));
    vector_scale(&b, 2, &t);
    test_cond("Test scale.", 2 == t.x && 4 == t.y && 6 == t.z);
    test_cond("Test length.", vector_tolerance_eq(sqrt(3), vector_length(&a)));
    vector_normalize(&a, &t);
    test_cond("Test normalize.", vector_tolerance_eq(1, vector_length(&t)));
    test_cond("Test distance.", vector_tolerance_eq(sqrt(5), vector_distance(&a, &b)));

    Vector d, n;
    d.x = d.y = -1;
    d.z = 0;
    n.x = n.z = 0;
    n.y = 1;

    vector_reflect(&d, &n, &t);
    test_cond("Test reflect.", vector_tolerance_eq(-1, t.x) && vector_tolerance_eq(1, t.y));

    vector_vector_mul(&a, &b, &t);
    test_cond("Test vector mul 1.", vector_tolerance_eq(0, vector_mul(&a, &t)) && vector_tolerance_eq(0, vector_mul(&b, &t)));

    a = (Vector) { .x = 1, .y = 0, .z = 0 };
    b = (Vector) { .x = 0, .y = 1, .z = 0 };
    test_cond("Test vector mul 2.", 0.0 == vector_mul(&a, &b));
    test_cond("Test vector angle.", M_PI_2 == vector_angle(&a, &b));

    vector_zero(&t);
    test_cond("Test zero.", 0 == t.x && 0 == t.y && 0 == t.z);

    a.x = 1;
    a.y = a.z = 0;
    b.x = b.y = 0;
    b.z = 1;
    vector_rotate(&a, &b, M_PI_2, &t);
    test_cond("Test rotate 1.", vector_tolerance_eq(0, t.x) && vector_tolerance_eq(1, t.y) && vector_tolerance_eq(0, t.z));
    test_cond("Test rotate 2.", M_PI_2 == vector_angle(&a, &b));

    a.x = 1.0;
    a.y = 2.0;
    a.z = 0.0;
    t.x = 0.0;
    t.y = 0.0;
    t.z = 1.0;
    vector_rotate(&a, &t, M_PI / 6.0, &b);;
    test_cond("Test rotate 3.", vector_tolerance_eq(M_PI / 6.0, vector_angle(&a, &b)));

    vector_get_orthogonal(&a, &b);
    test_cond("Test get any orthogonal 1.", vector_tolerance_eq(0, vector_mul(&a, &b)));
    a.x = -1;
    a.y = 0;
    a.z = 0;
    vector_get_orthogonal(&a, &b);
    test_cond("Test get any orthogonal 2.", vector_tolerance_eq(0, vector_mul(&a, &b)));

    test_report();
    return EXIT_SUCCESS;
}

#endif
