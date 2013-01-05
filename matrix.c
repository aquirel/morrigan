// matrix.c - matrix implementation.

#include <assert.h>

#include "matrix.h"

Vector *matrix_vector_mul(const Matrix *m, const Vector *v, Vector *result)
{
    assert(m && v && result && "Bad arguments.");
    result->x = m->values[0][0] * v->x + m->values[0][1] * v->y + m->values[0][2] * v->z;
    result->y = m->values[1][0] * v->x + m->values[1][1] * v->y + m->values[1][2] * v->z;
    result->z = m->values[2][0] * v->x + m->values[2][1] * v->y + m->values[2][2] * v->z;
    return result;
}
