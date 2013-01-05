// matrix.h - matrix implementation.

#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "vector.h"

typedef struct Matrix
{
    double values[3][3];
} Matrix;

Vector *matrix_vector_mul(const Matrix *m, const Vector *v, Vector *result); // matrix * vector.

#endif
