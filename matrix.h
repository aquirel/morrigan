// matrix.h - matrix implementation.

#pragma once
#ifndef __MATRIX_H__
#define __MATRIX_H__

#pragma message("__MATRIX_H__")

#include "morrigan.h"
#include "vector.h"

#pragma pack(push, 4)

typedef struct Matrix
{
    double values[3][3];
} Matrix;

#pragma pack(pop)

Vector *matrix_vector_mul(const Matrix *m, const Vector *v, Vector *result); // matrix * vector.

#endif /* __MATRIX_H__ */
