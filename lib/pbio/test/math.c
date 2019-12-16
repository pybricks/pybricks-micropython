
#include <stdio.h>

#include <pbio/fixmath.h>

#include <tinytest.h>
#include <tinytest_macros.h>

void pbio_math_test_sqrt(void *env) {
    tt_want(int_sqrt(4) == 2);
    tt_want(int_sqrt(400) == 20);
    tt_want(int_sqrt(40000) == 200);
    tt_want(int_sqrt(400000000) == 20000);
}
