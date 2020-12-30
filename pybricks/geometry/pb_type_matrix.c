// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_GEOMETRY

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <pybricks/geometry.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

#if MICROPY_PY_BUILTINS_FLOAT

// pybricks.geometry.Matrix.__init__
STATIC mp_obj_t pb_type_Matrix_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(rows));

    // If the input is already a matrix, just return it
    if (mp_obj_is_type(rows_in, &pb_type_Matrix)) {
        return rows_in;
    }

    // Before we allocate the object, check if it's a 1x1 matrix: C = [[c]],
    // in which case we should just return c as a float.

    // Unpack the main list of rows and get the requested sizes
    size_t m, n;
    mp_obj_t *row_objs, *scalar_objs;
    mp_obj_get_array(rows_in, &m, &row_objs);
    mp_obj_get_array(row_objs[0], &n, &scalar_objs);

    // It's a 1x1 object, assert type and just return it
    if (m == 1 && n == 1) {
        mp_obj_get_float_to_f(scalar_objs[0]);
        return scalar_objs[0];
    }

    // Dimensions must be nonzero
    if (m == 0 || n == 0) {
        // TODO: raise dimension error, m >= 1, n >= 1
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }


    // Create objects and save dimensions
    pb_type_Matrix_obj_t *self = m_new_obj(pb_type_Matrix_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->m = m;
    self->n = n;
    self->data = m_new(float, self->m * self->n);

    // Iterate through each of the rows to get the scalars
    for (size_t r = 0; r < self->m; r++) {

        size_t n;
        mp_obj_get_array(row_objs[r], &n, &scalar_objs);
        if (n != self->n) {
            // TODO: raise dimension error, all rows must have same length
            pb_assert(PBIO_ERROR_INVALID_ARG);
        }
        // Unpack the scalars
        for (size_t c = 0; c < self->n; c++) {
            self->data[r * self->n + c] = mp_obj_get_float_to_f(scalar_objs[c]);
        }
    }

    // Modifiers that allow basic modifications without moving data around
    self->scale = 1;
    self->transposed = false;

    return MP_OBJ_FROM_PTR(self);
}

// Get string representation of the form -123.456
static void print_float(char *buf, float x) {

    // Convert to positive int
    int32_t val = (int32_t)(x * 1000);
    bool negative = val < 0;
    if (negative) {
        val = -val;
    }

    // Limit to 999.999
    if (val >= 999999) {
        val = 999999;
    }

    // Get number of printed digits so we know where to add minus sign later
    int32_t c = snprintf(NULL, 0, "%" PRId32, val);

    // Print the integer
    snprintf(buf, 8, "%7" PRId32, val);

    // If the number was negative, add minus sign in the right place
    if (negative) {
        buf[6 - c <= 2 ? 6 - c : 2] = '-';
    }

    // If there is nothing before the decimal, ensure there is a zero
    buf[3] = buf[3] == ' ' ? '0' : buf[3];

    // Shift last 3 digits to make room for decimal point and insert 0 if empty
    buf[7] = buf[6] == ' ' ? '0' : buf[6];
    buf[6] = buf[5] == ' ' ? '0' : buf[5];
    buf[5] = buf[4] == ' ' ? '0' : buf[4];

    // Terminate and add decimal separator
    buf[4] = '.';
}

// pybricks.geometry.Matrix.__repr__
void pb_type_Matrix_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {

    // Print class name
    mp_print_str(print, "Matrix([\n");

    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Each "-123.456" is 8 chars. Each digit is appended with ", " or "]," so
    // 10 per digit. Each row starts with "    [" and ends with "\n" so we
    // add 6*rows. Plus 1 for null terminator.
    size_t len = self->m * self->n * 10 + self->m * 6 + 1;

    // Allocate the buffer
    char *buf = m_new(char, len);

    // Loop through the rows
    for (size_t r = 0; r < self->m; r++) {

        // Character starting index for this row
        size_t row_start = r * (self->n * 10 + 6);

        // Start row with "    ["
        strcpy(buf + row_start, "    [");

        // Loop through the colums, so the scalars in each row
        for (size_t c = 0; c < self->n; c++) {

            // Starting character index for this column
            size_t col_start = row_start + c * 10 + 5;

            // Get data index of the scalar we will print. Transposed attribute
            // tells us whether data is stored row by row or column by column.
            // (i, j) -> index:
            // regular:    i * self->n + j
            // transposed: j * self->m + i
            size_t idx = self->transposed ? c * self->m + r : r * self->n + c;

            // Get character representation of said value
            print_float(buf + col_start, self->data[idx] * self->scale);

            // Append ", " or "]," after the last value
            if (c < self->n - 1) {
                buf[col_start + 8] = ',';
                buf[col_start + 9] = ' ';
            } else {
                buf[col_start + 8] = ']';
                buf[col_start + 9] = ',';
            }
        }
        // New line at end of row
        buf[row_start + self->n * 10 + 5] = '\n';
    }
    // Terminate string
    buf[len - 1 ] = '\0';

    // Send the bufer to MicroPython
    mp_print_str(print, buf);
    mp_print_str(print, "])");
}

// pybricks.geometry.Matrix._add
STATIC mp_obj_t pb_type_Matrix__add(mp_obj_t lhs_obj, mp_obj_t rhs_obj, bool add) {

    // Get left and right matrices
    pb_type_Matrix_obj_t *lhs = MP_OBJ_TO_PTR(lhs_obj);
    pb_type_Matrix_obj_t *rhs = MP_OBJ_TO_PTR(rhs_obj);

    // Verify matching dimensions else raise error
    if (lhs->n != rhs->n || lhs->m != rhs->m) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Result has same shape as both sides
    pb_type_Matrix_obj_t *ret = m_new_obj(pb_type_Matrix_obj_t);
    ret->base.type = &pb_type_Matrix;
    ret->m = lhs->m;
    ret->n = rhs->n;
    ret->data = m_new(float, ret->m * ret->n);

    // Scale must be reset; it has been and multiplied out above
    ret->scale = 1;
    ret->transposed = false;

    // Add the matrices by looping over rows and columns
    for (size_t r = 0; r < ret->m; r++) {
        for (size_t c = 0; c < ret->n; c++) {
            // This entry is obtained as the sum of scalars of both matrices
            // First find the index of sources and destination.
            size_t ret_idx = r * ret->n + c;
            size_t lhs_idx = lhs->transposed ? c * ret->m + r : ret_idx;
            size_t rhs_idx = rhs->transposed ? c * ret->m + r : ret_idx;

            // Either add or subtract to get result.
            if (add) {
                ret->data[ret_idx] = lhs->data[lhs_idx] * lhs->scale + rhs->data[rhs_idx] * rhs->scale;
            } else {
                ret->data[ret_idx] = lhs->data[lhs_idx] * lhs->scale - rhs->data[rhs_idx] * rhs->scale;
            }

        }
    }

    return MP_OBJ_FROM_PTR(ret);
}

// pybricks.geometry.Matrix._mul
STATIC mp_obj_t pb_type_Matrix__mul(mp_obj_t lhs_in, mp_obj_t rhs_in) {

    // Get left and right matrices
    pb_type_Matrix_obj_t *lhs = MP_OBJ_TO_PTR(lhs_in);
    pb_type_Matrix_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);

    // Verify matching dimensions else raise error
    if (lhs->n != rhs->m) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Result has as many rows as left hand side and as many columns as right hand side.
    pb_type_Matrix_obj_t *ret = m_new_obj(pb_type_Matrix_obj_t);
    ret->base.type = &pb_type_Matrix;
    ret->m = lhs->m;
    ret->n = rhs->n;
    ret->data = m_new(float, ret->m * ret->n);

    // Scale is commutative, so we can do it separately
    ret->scale = lhs->scale * rhs->scale;
    ret->transposed = false;

    // Multiply the matrices by looping over rows and columns
    for (size_t r = 0; r < ret->m; r++) {
        for (size_t c = 0; c < ret->n; c++) {
            // This entry is obtained as the sum of the products of the entries
            // of the r'th row of lhs and the c'th column of rhs, so size lhs->n.
            float sum = 0;
            for (size_t k = 0; k < lhs->n; k++) {
                // k'th entry on the r'th row (i = r, j = k)
                size_t lhs_idx = lhs->transposed ? k * lhs->m + r : r * lhs->n + k;
                // k'th entry on the c'th column  (i = k, j = c)
                size_t rhs_idx = rhs->transposed ? c * rhs->m + k : k * rhs->n + c;
                sum += lhs->data[lhs_idx] * rhs->data[rhs_idx];
            }
            ret->data[ret->n * r + c] = sum;
        }
    }

    // If the result is a 1x1, return as scalar. This solves all the
    // usual matrix library problems where you have to type things like
    // C[0][0] just to get the scalar, such as for the inner product of two
    // vectors. The same is done for 1x1 initialization above.
    if (ret->m == 1 && ret->n == 1) {
        return mp_obj_new_float_from_f(ret->data[0] * ret->scale);
    }

    return MP_OBJ_FROM_PTR(ret);
}

// pybricks.geometry.Matrix._scale
STATIC mp_obj_t pb_type_Matrix__scale(mp_obj_t self_in, float scale) {
    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_type_Matrix_obj_t *copy = m_new_obj(pb_type_Matrix_obj_t);
    copy->base.type = &pb_type_Matrix;

    // Point to the same data instead of copying
    copy->data = self->data;
    copy->n = self->n;
    copy->m = self->m;
    copy->scale = self->scale * scale;
    copy->transposed = self->transposed;

    return MP_OBJ_FROM_PTR(copy);
}

// pybricks.geometry.Matrix._get_scalar
float pb_type_Matrix_get_scalar(mp_obj_t self_in, size_t r, size_t c) {
    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (r >= self->m || c >= self->n) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    size_t idx = self->transposed ? c * self->m + r : r * self->n + c;
    return self->data[idx] * self->scale;
}

// pybricks.geometry.Matrix._T
STATIC mp_obj_t pb_type_Matrix__T(mp_obj_t self_in) {
    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_type_Matrix_obj_t *copy = m_new_obj(pb_type_Matrix_obj_t);
    copy->base.type = &pb_type_Matrix;

    // Point to the same data instead of copying
    copy->data = self->data;
    copy->n = self->m;
    copy->m = self->n;
    copy->scale = self->scale;
    copy->transposed = !self->transposed;

    return MP_OBJ_FROM_PTR(copy);
}

STATIC void pb_type_Matrix_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    // Read only
    if (dest[0] == MP_OBJ_NULL) {
        // Create and return transpose
        if (attr == MP_QSTR_T) {
            dest[0] = pb_type_Matrix__T(self_in);
            return;
        }
        // Create and return shape tuple
        if (attr == MP_QSTR_shape) {
            pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

            mp_obj_t shape[] = {
                MP_OBJ_NEW_SMALL_INT(self->m),
                MP_OBJ_NEW_SMALL_INT(self->n)
            };

            dest[0] = mp_obj_new_tuple(2, shape);
            return;
        }
    }
}

STATIC mp_obj_t pb_type_Matrix_unary_op(mp_unary_op_t op, mp_obj_t o_in) {

    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(o_in);

    switch (op) {
        // Hash is the same as in generic unary op
        case MP_UNARY_OP_HASH:
            return MP_OBJ_NEW_SMALL_INT((mp_uint_t)o_in);
        // Positive just returns the original object
        case MP_UNARY_OP_POSITIVE:
            return o_in;
        // Negative returns a scaled copy
        case MP_UNARY_OP_NEGATIVE:
            return pb_type_Matrix__scale(o_in, -1);
        // Get absolute vale (magnitude)
        case MP_UNARY_OP_ABS: {
            // For vectors, this is the norm
            if (self->m == 1 || self->n == 1) {
                size_t len = self->m * self->n;
                float squares = 0;
                for (size_t i = 0; i < len; i++) {
                    squares += self->data[i] * self->data[i];
                }
                return mp_obj_new_float_from_f(sqrtf(squares) * self->scale);
            }
            // Determinant not implemented
            return MP_OBJ_NULL;
        }
        default:
            // Other operations are not supported
            return MP_OBJ_NULL;
    }
}

STATIC mp_obj_t pb_type_Matrix_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {

    switch (op) {
        case MP_BINARY_OP_ADD:
        case MP_BINARY_OP_INPLACE_ADD:
            return pb_type_Matrix__add(lhs_in, rhs_in, true);
        case MP_BINARY_OP_SUBTRACT:
        case MP_BINARY_OP_INPLACE_SUBTRACT:
            return pb_type_Matrix__add(lhs_in, rhs_in, false);
        case MP_BINARY_OP_MULTIPLY:
        case MP_BINARY_OP_INPLACE_MULTIPLY:
            // If right of operand is a number, just scale to be faster
            if (mp_obj_is_float(rhs_in) || mp_obj_is_int(rhs_in)) {
                return pb_type_Matrix__scale(lhs_in, mp_obj_get_float_to_f(rhs_in));
            }
            // Otherwise we have to do full multiplication.
            return pb_type_Matrix__mul(lhs_in, rhs_in);
        case MP_BINARY_OP_REVERSE_MULTIPLY:
            // This gets called for c*A, so scale A by c (rhs/lhs is meaningless here)
            return pb_type_Matrix__scale(lhs_in, mp_obj_get_float_to_f(rhs_in));
        case MP_BINARY_OP_TRUE_DIVIDE:
        case MP_BINARY_OP_INPLACE_TRUE_DIVIDE:
            // Scalar division by c is scalar multiplication by 1/c
            return pb_type_Matrix__scale(lhs_in, 1 / mp_obj_get_float_to_f(rhs_in));
        default:
            // Other operations not supported
            return MP_OBJ_NULL;
    }
}

STATIC mp_obj_t pb_type_Matrix_subscr(mp_obj_t self_in, mp_obj_t index_in, mp_obj_t value_in) {

    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (value_in == MP_OBJ_SENTINEL) {
        // Integer index is just reading straight from self->data[idx].
        // But we need to do some checks to make sure we read within bounds.
        mp_int_t len = self->m * self->n;
        mp_int_t idx = -1;

        if (mp_obj_is_int(index_in)) {
            // Get requested index as int
            idx = mp_obj_get_int(index_in);

            // This is Python, so allow for negative index
            if (idx < 0) {
                idx += len;
            }

            // Data may be stored as transposed for efficiency reasons,
            // but the user will still expect a consistent value by index.
            if (self->transposed) {
                idx = idx / self->n + (idx % self->n) * self->m;
            }
        } else {
            // Get requested value at (row, col) pair
            size_t s;
            mp_obj_t *shape;
            mp_obj_get_array(index_in, &s, &shape);

            // Only proceed if the index is indeed of shape (row, col)
            if (s == 2) {
                // Get row index, allowing for negative
                mp_int_t r = mp_obj_get_int(shape[0]);
                if (r < 0) {
                    r += self->m;
                }
                // Get col index, allowing for negative
                mp_int_t c = mp_obj_get_int(shape[1]);
                if (c < 0) {
                    c += self->n;
                }
                // Make sure requested row/col exist within (m, n)
                if (c >= 0 && r >= 0 && (size_t)c < self->n && (size_t)r < self->m) {
                    idx = self->transposed ? c * self->m + r : r * self->n + c;
                }
            }
        }

        // Make sure we have meanwhile a valid index by now
        if (idx < 0 || idx >= len) {
            // FIXME: raise dimension error
            mp_raise_msg(&mp_type_IndexError, MP_ERROR_TEXT("index out of range"));
        }

        // Return result
        return mp_obj_new_float_from_f(self->scale * self->data[idx]);
    }
    return MP_OBJ_NULL;
}

// type(pybricks.geometry.Matrix)
const mp_obj_type_t pb_type_Matrix = {
    { &mp_type_type },
    .name = MP_QSTR_Matrix,
    .print = pb_type_Matrix_print,
    .make_new = pb_type_Matrix_make_new,
    .attr = pb_type_Matrix_attr,
    .unary_op = pb_type_Matrix_unary_op,
    .binary_op = pb_type_Matrix_binary_op,
    .subscr = pb_type_Matrix_subscr,
};

// pybricks.geometry._make_vector
mp_obj_t pb_type_Matrix_make_vector(size_t m, float *data, bool normalize) {

    // Create object and save dimensions
    pb_type_Matrix_obj_t *mat = m_new_obj(pb_type_Matrix_obj_t);
    mat->base.type = &pb_type_Matrix;
    mat->m = m;
    mat->n = 1;
    mat->data = m_new(float, m);

    // Copy data and compute norm
    float squares = 0;
    for (size_t i = 0; i < m; i++) {
        mat->data[i] = data[i];
        squares += data[i] * data[i];
    }
    mat->scale = normalize ? 1 / sqrtf(squares) : 1;

    return MP_OBJ_FROM_PTR(mat);
}

// pybricks.geometry._make_bitmap
mp_obj_t pb_type_Matrix_make_bitmap(size_t m, size_t n, float scale, uint32_t src) {

    // Create object and save dimensions
    pb_type_Matrix_obj_t *mat = m_new_obj(pb_type_Matrix_obj_t);
    mat->base.type = &pb_type_Matrix;
    mat->m = m;
    mat->n = n;
    mat->scale = scale;
    mat->data = m_new(float, m * n);

    for (size_t i = 0; i < m * n; i++) {
        mat->data[m * n - i - 1] = (src & (1 << i)) != 0;
    }

    return MP_OBJ_FROM_PTR(mat);
}

#endif // MICROPY_PY_BUILTINS_FLOAT

#endif // PYBRICKS_PY_GEOMETRY
