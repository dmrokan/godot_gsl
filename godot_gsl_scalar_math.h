#ifndef GODOT_GSL_SCALAR_MATH_H
#define GODOT_GSL_SCALAR_MATH_H

#include <math.h>

#define MTYPE double

MTYPE ggsl_math_eq(MTYPE in);
MTYPE ggsl_math_add(MTYPE in1, MTYPE in2);
MTYPE ggsl_math_sub(MTYPE in1, MTYPE in2);
MTYPE ggsl_math_mul_el(MTYPE in1, MTYPE in2);
MTYPE ggsl_math_div_el(MTYPE in1, MTYPE in2);
MTYPE ggsl_math_pow_el(MTYPE in1, MTYPE in2);

#endif
