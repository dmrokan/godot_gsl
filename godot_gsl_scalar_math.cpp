#include "godot_gsl_scalar_math.h"

MTYPE ggsl_math_eq(MTYPE in)
{
    return in;
}

MTYPE ggsl_math_sub(MTYPE in1, MTYPE in2)
{
    return in1 - in2;
}

MTYPE ggsl_math_mul_el(MTYPE in1, MTYPE in2)
{
    return in1 * in2;
}

MTYPE ggsl_math_div_el(MTYPE in1, MTYPE in2)
{
    return in1 / in2;
}

MTYPE ggsl_math_pow_el(MTYPE in1, MTYPE in2)
{
    return pow(in1, in2);
}
