#include <inttypes.h>
#include <math.h>
#include <stdio.h>

#include "py/obj.h"

#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

#include "pb_type_matrix.h"

#if MICROPY_PY_BUILTINS_FLOAT

// Class structure for Matrix
typedef struct _pb_type_Matrix_obj_t {
    mp_obj_base_t base;
    float_t *data;
    float_t scale;
    size_t m;
    size_t n;
    bool transposed;
} pb_type_Matrix_obj_t;

// pybricks.robotics.Matrix.__init__
STATIC mp_obj_t pb_type_Matrix_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(rows));

    // If the input is already a matrix, just return it
    if (mp_obj_is_type(rows, &pb_type_Matrix_type)) {
        return rows;
    }

    // Before we allocate the object, check if it's a 1x1 matrix: C = [[c]],
    // in which case we should just return c as a float.

    // Unpack the main list of rows and get the requested sizes
    size_t m, n;
    mp_obj_t *row_objs, *scalar_objs;
    mp_obj_get_array(rows, &m, &row_objs);
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
    self->data = m_new(float_t, self->m * self->n);

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
static void print_float(char *buf, float_t x) {

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

// pybricks.robotics.Matrix.__repr__
void pb_type_Matrix_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {

    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Each "-123.456" is 8. Each digit is appended with ", " or "]\n", so 10 per digit
    // Also each row starts with "[[" or " [" so we add 2*rows. Finally we add \0, so +1.
    size_t len = self->m * self->n * 10 + self->m * 2 + 1;

    // Allocate the buffer
    char *buf = m_new(char, len);

    // Loop through the rows
    for (size_t r = 0; r < self->m; r++) {

        // Character starting index for this row
        size_t row_start = r * (self->n * 10 + 2);

        // Prepend "[[" on first row, else " ["
        buf[row_start] = r == 0 ? '[' : ' ';
        buf[row_start + 1] = '[';

        // Loop through the colums, so the scalars in each row
        for (size_t c = 0; c < self->n; c++) {

            // Starting character index for this column
            size_t col_start = row_start + c * 10 + 2;

            // Get data index of the scalar we will print. Transposed attribute
            // tells us whether data is stored row by row or column by column.
            // (i, j) -> index:
            // regular:    i * self->n + j
            // transposed: j * self->m + i
            size_t idx = self->transposed ? c * self->m + r : r * self->n + c;

            // Get character representation of said value
            print_float(buf + col_start, self->data[idx] * self->scale);

            // Append ", " or "]\n" after the last value
            if (c < self->n - 1) {
                buf[col_start + 8] = ',';
                buf[col_start + 9] = ' ';
            } else {
                buf[col_start + 8] = ']';
                buf[col_start + 9] = '\n';
            }
        }
    }
    // Close the group of rows with "]"
    buf[len - 2 ] = ']';
    buf[len - 1 ] = '\0';

    // Send the bufer to MicroPython
    mp_print_str(print, buf);
}

// pybricks.robotics.Matrix._add
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
    ret->base.type = &pb_type_Matrix_type;
    ret->m = lhs->m;
    ret->n = rhs->n;
    ret->data = m_new(float_t, ret->m * ret->n);

    // Add the matrices by looping over rows and columns
    for (size_t r = 0; r < ret->m; r++) {
        for (size_t c = 0; c < ret->n; c++) {
            // This entry is obtained as the sum of scalars of both matrices
            size_t idx = lhs->transposed ? c * lhs->m + r : r * lhs->n + c;
            if (add) {
                ret->data[idx] = lhs->data[idx] * lhs->scale + rhs->data[idx] * rhs->scale;
            } else {
                ret->data[idx] = lhs->data[idx] * lhs->scale - rhs->data[idx] * rhs->scale;
            }

        }
    }

    // Scale must be reset; it has been and multiplied out above
    ret->scale = 1;
    ret->transposed = false;

    return MP_OBJ_FROM_PTR(ret);
}

// pybricks.robotics.Matrix._mul
STATIC mp_obj_t pb_type_Matrix__mul(mp_obj_t lhs_obj, mp_obj_t rhs_obj) {

    // Get left and right matrices
    pb_type_Matrix_obj_t *lhs = MP_OBJ_TO_PTR(lhs_obj);
    pb_type_Matrix_obj_t *rhs = MP_OBJ_TO_PTR(rhs_obj);

    // Verify matching dimensions else raise error
    if (lhs->n != rhs->m) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Result has as many rows as left hand side and as many columns as right hand side.
    pb_type_Matrix_obj_t *ret = m_new_obj(pb_type_Matrix_obj_t);
    ret->base.type = &pb_type_Matrix_type;
    ret->m = lhs->m;
    ret->n = rhs->n;
    ret->data = m_new(float_t, ret->m * ret->n);

    // Scale is commutative, so we can do it separately
    ret->scale = lhs->scale * rhs->scale;
    ret->transposed = false;

    // Multiply the matrices by looping over rows and columns
    for (size_t r = 0; r < ret->m; r++) {
        for (size_t c = 0; c < ret->n; c++) {
            // This entry is obtained as the sum of the products of the entries
            // of the r'th row of lhs and the c'th column of rhs, so size lhs->n.
            float_t sum = 0;
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

// pybricks.robotics.Matrix._scale
STATIC mp_obj_t pb_type_Matrix__scale(mp_obj_t self_in, float_t scale) {
    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_type_Matrix_obj_t *copy = m_new_obj(pb_type_Matrix_obj_t);
    copy->base.type = &pb_type_Matrix_type;

    // Point to the same data instead of copying
    copy->data = self->data;
    copy->n = self->n;
    copy->m = self->m;
    copy->scale = self->scale * scale;
    copy->transposed = self->transposed;

    return MP_OBJ_FROM_PTR(copy);
}

// pybricks.robotics.Matrix.T
STATIC mp_obj_t pb_type_Matrix_T(mp_obj_t self_in) {
    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_type_Matrix_obj_t *copy = m_new_obj(pb_type_Matrix_obj_t);
    copy->base.type = &pb_type_Matrix_type;

    // Point to the same data instead of copying
    copy->data = self->data;
    copy->n = self->m;
    copy->m = self->n;
    copy->scale = self->scale;
    copy->transposed = !self->transposed;

    return MP_OBJ_FROM_PTR(copy);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Matrix_T_obj, pb_type_Matrix_T);


STATIC mp_obj_t pb_type_Matrix_unary_op(mp_unary_op_t op, mp_obj_t o_in) {

    pb_type_Matrix_obj_t *self = MP_OBJ_TO_PTR(o_in);

    switch (op) {
        // len returns the size of the matrix: (m, n)
        case MP_UNARY_OP_LEN: {
            mp_obj_t shape[2];
            shape[0] = MP_OBJ_NEW_SMALL_INT(self->m);
            shape[1] = MP_OBJ_NEW_SMALL_INT(self->n);
            return mp_obj_new_tuple(2, shape);
        }
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
                return mp_obj_new_float(sqrtf(squares) * self->scale);
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

// dir(pybricks.robotics.Matrix)
STATIC const mp_rom_map_elem_t pb_type_Matrix_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_T),     MP_ROM_PTR(&pb_type_Matrix_T_obj)              },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_Matrix_locals_dict, pb_type_Matrix_locals_dict_table);

// type(pybricks.robotics.Matrix)
const mp_obj_type_t pb_type_Matrix_type = {
    { &mp_type_type },
    .name = MP_QSTR_Matrix,
    .print = pb_type_Matrix_print,
    .make_new = pb_type_Matrix_make_new,
    .unary_op = pb_type_Matrix_unary_op,
    .binary_op = pb_type_Matrix_binary_op,
    .locals_dict = (mp_obj_dict_t *)&pb_type_Matrix_locals_dict,
};

// pybricks.robotics._vector
STATIC mp_obj_t robotics__vector(size_t n_args, const mp_obj_t *args, bool normalize) {
    pb_type_Matrix_obj_t *mat = m_new_obj(pb_type_Matrix_obj_t);
    mat->base.type = &pb_type_Matrix_type;
    mat->data = m_new(float_t, n_args);
    mat->m = n_args;
    mat->n = 1;
    float_t squares = 0;
    for (size_t i = 0; i < n_args; i++) {
        mat->data[i] = mp_obj_get_float_to_f(args[i]);
        squares += mat->data[i] * mat->data[i];
    }
    mat->scale = normalize ? 1 / sqrtf(squares) : 1;

    return MP_OBJ_FROM_PTR(mat);
}

// pybricks.robotics.Vector
STATIC mp_obj_t robotics_Vector(size_t n_args, const mp_obj_t *args) {
    return robotics__vector(n_args, args, false);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(robotics_Vector_obj, 2, 4, robotics_Vector);

// pybricks.robotics.UnitVector
STATIC mp_obj_t robotics_UnitVector(size_t n_args, const mp_obj_t *args) {
    return robotics__vector(n_args, args, true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(robotics_UnitVector_obj, 2, 4, robotics_UnitVector);

#endif // MICROPY_PY_BUILTINS_FLOAT
