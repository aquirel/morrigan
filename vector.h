// vector.h - vector implementation.

#pragma once
#ifndef __VECTOR_H__
#define __VECTOR_H__

#pragma message("__VECTOR_H__")

#include <stdbool.h>

#include "morrigan.h"

extern double vector_eps;

#pragma pack(push, 4)

typedef struct Vector
{
    double x, y, z;
} Vector;

#pragma pack(pop)

bool vector_tolerance_eq(double v1, double v2);
bool vector_eq(const Vector *v1, const Vector *v2);
bool vector2_eq(const Vector *v1, const Vector *v2);

Vector *vector_add(const Vector *v1, const Vector *v2, Vector *result);
#define VECTOR_ADD(v1, v2) vector_add((v1), (v2), (v1))
Vector *vector_sub(const Vector *v1, const Vector *v2, Vector *result);
#define VECTOR_SUB(v1, v2) vector_sub((v1), (v2), (v1))
double vector_mul(const Vector *v1, const Vector *v2);
Vector *vector_scale(const Vector *v, double factor, Vector *result);
#define VECTOR_SCALE(v, f) vector_scale((v), (f), (v))
double vector_length(const Vector *v);
Vector *vector_normalize(const Vector *v, Vector *result);
#define VECTOR_NORMALIZE(v) vector_normalize((v), (v))

double vector_distance(const Vector *v1, const Vector *v2);
Vector *vector_reflect(const Vector *v, const Vector *normal, Vector *result);
#define VECTOR_REFLECT(v, n) vector_reflect((v), (n), (v))
Vector *vector_vector_mul(const Vector *v1, const Vector *v2, Vector *result);
#define VECTOR_VECTOR_MUL(v1, v2) vector_vector_mul((v1), (v2), (v1))

Vector *vector_rotate(const Vector *v, const Vector *axis, double angle, Vector *result);
#define VECTOR_ROTATE(v, axis, angle) vector_rotate((v), (axis), (angle), (v))

Vector *vector_zero(Vector *v);
double vector_angle(const Vector *v1, const Vector *v2);

#endif /* __VECTOR_H__ */
