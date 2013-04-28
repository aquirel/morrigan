// matrix.h - matrix implementation.

#pragma once
#ifndef __MATRIX_H__
#define __MATRIX_H__

//#pragma message("__MATRIX_H__")

#include "morrigan.h"
#include "vector.h"

#pragma pack(push, 4)

typedef struct Matrix
{
    double values[3][3];
} Matrix;

#pragma pack(pop)

Vector *matrix_vector_mul(const Matrix *m, const Vector *v, Vector *result); // matrix * vector.
#define MATRIX_VECTOR_MUL(m, v) matrix_vector_mul((m), (v), (v))

Matrix *matrix_matrix_mul(const Matrix *m1, const Matrix *m2, Matrix *result);
#define MATRIX_MATRIX_MUL(m1, m2) matrix_matrix_mul((m1), (m2), (m1))

Matrix *matrix_invert(const Matrix *m, Matrix *result);
#define MATRIX_INVERT(m) matrix_invert((m), (m))

#endif /* __MATRIX_H__ */
