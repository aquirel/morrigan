// matrix.c - matrix implementation.

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "matrix.h"

static int __find_first_non_zero_column(const Matrix *m, int start_index);
static int __find_row_with_non_zero_element_in_column(const Matrix *m, int column_index, int start_index);
static void __matrix_swap_rows(Matrix *m, int row1, int row2);
static void __matrix_divide_row(Matrix *m, int row, double divisor);
static void __matrix_sub_row(Matrix *m, int row, int row_to_substract, double factor);

Vector *matrix_vector_mul(const Matrix *m, const Vector *v, Vector *result)
{
    assert(m && v && result && "Bad arguments.");
    Vector _v = *v;
    result->x = m->values[0][0] * _v.x + m->values[0][1] * _v.y + m->values[0][2] * _v.z;
    result->y = m->values[1][0] * _v.x + m->values[1][1] * _v.y + m->values[1][2] * _v.z;
    result->z = m->values[2][0] * _v.x + m->values[2][1] * _v.y + m->values[2][2] * _v.z;
    return result;
}

Matrix *matrix_matrix_mul(const Matrix *m1, const Matrix *m2, Matrix *result)
{
    assert(m1 && m2 && result && "Bad arguments.");

    Matrix _m1 = *m1;

    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 3; j++)
        {
            double s = 0.0;

            for (size_t k = 0; k < 3; k++)
            {
                s += _m1.values[i][k] * m2->values[k][j];
            }

            result->values[i][j] = s;
        }
    }

    return result;
}

Matrix *matrix_invert(const Matrix *m, Matrix *result)
{
    assert(m && result && "Bad arguments.");

    Matrix t = *m;
    memset(result, 0, sizeof(Matrix));
    result->values[0][0] = result->values[1][1] = result->values[2][2] = 1;

    for (size_t n = 0; n < 3; n++)
    {
        int column_index = __find_first_non_zero_column(&t, n);
        if (-1 == column_index)
        {
            break;
        }

        if (0.0 == t.values[n][column_index])
        {
            int swap_row_index = __find_row_with_non_zero_element_in_column(&t, column_index, n);
            assert(-1 != swap_row_index);
            __matrix_swap_rows(&t, n, swap_row_index);
            __matrix_swap_rows(result, n, swap_row_index);
        }

        double divisor = t.values[n][column_index];
        __matrix_divide_row(&t, n, divisor);
        __matrix_divide_row(result, n, divisor);

        for (size_t row_index = n + 1; row_index < 3; row_index++)
        {
            double factor = t.values[row_index][column_index];
            __matrix_sub_row(&t, row_index, n, factor);
            __matrix_sub_row(result, row_index, n, factor);
        }
    }

    for (size_t n = 0; n < 3; n++)
    {
        double divisor = t.values[n][n];
        if (0.0 == divisor)
        {
            continue;
        }

        __matrix_divide_row(&t, n, divisor);
        __matrix_divide_row(result, n, divisor);
    }

    for (int n = 1; n >= 0; n--)
    {
        for (int row_to_substract = 2; row_to_substract > n; row_to_substract--)
        {
            double factor = t.values[n][row_to_substract];
            __matrix_sub_row(&t, n, row_to_substract, factor);
            __matrix_sub_row(result, n, row_to_substract, factor);
        }
    }

    return result;
}

static int __find_first_non_zero_column(const Matrix *m, int start_index)
{
    assert(m && "Bad matrix pointer.");
    assert(0 <= start_index && start_index < 3 && "Bad start index.");

    int column_index = start_index;
    for (; column_index < 3; column_index++)
    {
        size_t i = 0;
        for (; i < 3; i++)
        {
            if (0.0 != m->values[i][column_index])
            {
                return column_index;
            }
        }
    }

    return -1;
}

static int __find_row_with_non_zero_element_in_column(const Matrix *m, int column_index, int start_index)
{
    assert(m && "Bad matrix pointer.");
    assert(0 <= column_index && column_index < 3 && "Bad column index.");
    assert(0 <= start_index && start_index < 3 && "Bad start index.");

    int row_index = start_index;
    for (; row_index < 3; row_index++)
    {
        if (0.0 != m->values[row_index][column_index])
        {
            return row_index;
        }
    }

    return -1;
}

static void __matrix_swap_rows(Matrix *m, int row1, int row2)
{
    assert(m && "Bad matrix pointer.");
    assert(0 <= row1 && row1 < 3 && "Bad row 1 index.");
    assert(0 <= row2 && row2 < 3 && "Bad row 2 index.");
    assert(row1 != row2 && "Nothing to swap!");

    for (size_t column_index = 0; column_index < 3; column_index++)
    {
        double t = m->values[row1][column_index];
        m->values[row1][column_index] = m->values[row2][column_index];
        m->values[row2][column_index] = t;
    }
}

static void __matrix_divide_row(Matrix *m, int row, double divisor)
{
    assert(m && "Bad matrix pointer.");
    assert(0 <= row && row < 3 && "Bad row index.");
    assert(0.0 != divisor && "Bad divisor.");

    for (size_t column_index = 0; column_index < 3; column_index++)
    {
        m->values[row][column_index] /= divisor;
    }
}

static void __matrix_sub_row(Matrix *m, int row, int row_to_substract, double factor)
{
    assert(m && "Bad matrix pointer.");
    assert(0 <= row && row < 3 && "Bad row index.");
    assert(0 <= row_to_substract && row_to_substract < 3 && "Bad row to substract index.");
    assert(row != row_to_substract && "Can't be the same.");

    for (size_t column_index = 0; column_index < 3; column_index++)
    {
        m->values[row][column_index] -= factor * m->values[row_to_substract][column_index];
    }
}

#if defined(MATRIX_TESTS)
#include <stdio.h>

#include "testhelp.h"

int main(void)
{
    Matrix m1 = {
        .values = {
            { 1, 2, 3 },
            { 3, 2, 1 },
            { 2, 1, 3 }
        }
    },
    m2;

    matrix_invert(&m1, &m2);
    matrix_matrix_mul(&m1, &m2, &m1);

    test_cond("Matrix complex test.",
              vector_tolerance_eq(1.0, m1.values[0][0]) &&
              vector_tolerance_eq(0.0, m1.values[0][1]) &&
              vector_tolerance_eq(0.0, m1.values[0][2]) &&
              vector_tolerance_eq(0.0, m1.values[1][0]) &&
              vector_tolerance_eq(1.0, m1.values[1][1]) &&
              vector_tolerance_eq(0.0, m1.values[1][2]) &&
              vector_tolerance_eq(0.0, m1.values[2][0]) &&
              vector_tolerance_eq(0.0, m1.values[2][1]) &&
              vector_tolerance_eq(1.0, m1.values[2][2]));

    test_report();
    return EXIT_SUCCESS;
}

#endif
