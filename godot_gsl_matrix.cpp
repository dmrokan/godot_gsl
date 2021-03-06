#include "godot_gsl_matrix.h"

void gsl_no_blas_gemv(CBLAS_TRANSPOSE_t tr, DTYPE *mtx, VTYPE *vec, VTYPE *out,
                      GGSL_BOUNDS &bounds_mtx, GGSL_BOUNDS &bounds_vec, GGSL_BOUNDS &bounds_out)
{
    /*
     * TODO: Implement transposed version
     */
    size_t row_count, col_count;
    if (tr == CblasNoTrans)
    {
        if (bounds_mtx.c2 - bounds_mtx.c1 != bounds_vec.r2 - bounds_vec.r1)
        {
            GGSL_MESSAGE("gls_no_blas_gemv: bounds_mtx[3] - bounds_mtx[2] != bounds_vec[1] - bounds_vec[0]");
            return;
        }
        row_count = bounds_mtx.r2 - bounds_mtx.r1;
        col_count = bounds_mtx.c2 - bounds_mtx.c1;
    }
    else if (tr == CblasTrans)
    {
        if (bounds_mtx.r2 - bounds_mtx.r1 != bounds_vec.r2 - bounds_vec.r1)
        {
            GGSL_MESSAGE("gls_no_blas_gemv: bounds_mtx[1] - bounds_mtx[0] != bounds_vec[1] - bounds_vec[0]");
            return;
        }
        col_count = bounds_mtx.c2 - bounds_mtx.c1;
        row_count = bounds_mtx.r2 - bounds_mtx.r1;
    }

    for (size_t k = 0; k < row_count; k++)
    {
        double res = 0.0;

        for (size_t l = 0; l < col_count; l++)
        {
            double val_mtx, val_vec;

            if (tr == CblasNoTrans)
            {
                val_mtx = gsl_matrix_get(mtx, k + bounds_mtx.r1, l + bounds_mtx.c1);
                val_vec = gsl_vector_get(vec, l + bounds_vec.r1);
            }
            else if (tr == CblasTrans)
            {
                val_mtx = gsl_matrix_get(mtx, l, k);
                val_vec = gsl_vector_get(vec, l);
            }

            res += val_mtx * val_vec;
        }

        gsl_vector_set(out, k + bounds_out.r1, res);
    }
}

void gsl_no_blas_gemm(CBLAS_TRANSPOSE_t tr, CBLAS_TRANSPOSE_t tr2,
                      DTYPE *mtx1, DTYPE *mtx2, DTYPE *out,
                      GGSL_BOUNDS &bounds_mtx1, GGSL_BOUNDS &bounds_mtx2, GGSL_BOUNDS &bounds_out)
{
    /*
     * TODO: Implement transposed version
     */
    size_t row_count = 0;
    size_t col_count = 0;
    size_t prod_count = 0;

    if (tr == CblasNoTrans)
    {
        if (bounds_mtx1.c2 - bounds_mtx1.c1 != bounds_mtx2.r2 - bounds_mtx2.r1
            && bounds_mtx1.r2 - bounds_mtx1.r1 != bounds_mtx2.c2 - bounds_mtx2.c1)
        {
            GGSL_MESSAGE("gls_no_blas_gemm: bounds_mtx[3] - bounds_mtx[2] != bounds_vec[1] - bounds_vec[0]");
            return;
        }

        row_count = bounds_mtx1.r2 - bounds_mtx1.r1;
        col_count = bounds_mtx2.c2 - bounds_mtx2.c1;
        prod_count = bounds_mtx1.c2 - bounds_mtx1.c1;
    }
    else if (tr == CblasTrans)
    {

        if (bounds_mtx1.r2 - bounds_mtx1.r1 != bounds_mtx2.r2 - bounds_mtx2.r1
            && bounds_mtx1.c2 - bounds_mtx1.c1 != bounds_mtx2.c2 - bounds_mtx2.c1)
        {
            GGSL_MESSAGE("gls_no_blas_gemv: bounds_mtx[1] - bounds_mtx[0] != bounds_vec[1] - bounds_vec[0]");
            return;
        }

        row_count = bounds_mtx1.c2 - bounds_mtx1.c1;
        col_count = bounds_mtx2.c2 - bounds_mtx2.c1;
        prod_count = bounds_mtx1.r2 - bounds_mtx1.r1;
    }

    for (size_t k = 0; k < row_count; k++)
    {

        for (size_t l = 0; l < col_count; l++)
        {
            double res = 0.0;

            for (size_t m = 0; m < prod_count; m++)
            {
                size_t i_mtx1, j_mtx1, i_mtx2, j_mtx2;
                if (tr == CblasNoTrans)
                {
                    i_mtx1 = k + bounds_mtx1.r1;
                    j_mtx1 = m + bounds_mtx1.c1;
                    i_mtx2 = m + bounds_mtx2.r1;
                    j_mtx2 = l + bounds_mtx2.c1;
                }

                res += gsl_matrix_get(mtx1, i_mtx1, j_mtx1) * gsl_matrix_get(mtx2, i_mtx2, j_mtx2);
            }

            gsl_matrix_set(out, k + bounds_out.r1, l + bounds_out.c1, res);
        }
    }
}

/*
 * GodotGSLMatrix class method definitions
 */

void GodotGSLMatrix::init(const size_t row_count, const size_t col_count)
{
    gsl_mtx = _alloc(row_count, col_count);

    size[0] = row_count;
    size[1] = col_count;
    bounds.r1 = 0;
    bounds.r2 = row_count;
    bounds.c1 = 0;
    bounds.c2 = col_count;

    if ((size[0] == 1 || size[1] == 1) && !(size[0] == 1 && size[1] == 1))
    {
        type = GSL_VECTOR;

        if (size[0] == 1)
        {
            is_row_vector = true;
        }

        _vector_alloc((size[0] == 1 ? size[1] : size[0]));
    }
    else if (size[0] == 1 && size[1] == 1)
    {
        type = GSL_SCALER;
        gsl_scalar = gsl_mtx->data;
    }
}

void GodotGSLMatrix::init(const Array a)
{
    size_t row_count = a.size();
    if (row_count == 0)
    {
        ERR_FAIL_COND("GodotGSLMatrix::GodotGSLMatrix: row_count == 0");
    }

    Array b = (Array) a.get(0);
    size_t col_count = b.size();
    if (col_count == 0)
    {
        ERR_FAIL_COND("GodotGSLMatrix::GodotGSLMatrix: col_count == 0");
    }

    gsl_mtx = _alloc(row_count, col_count);

    for (int k = 0; k < row_count; k++)
    {
        for (int l = 0; l < col_count; l++)
        {
            Array row = (Array) a.get(k);
            if (row.size() <= l)
            {
                ERR_FAIL_COND("GodotGSLMatrix::GodotGSLMatrix: row->size() <= l");
            }

            STYPE val = (STYPE) row.get(l);

            gsl_matrix_set(gsl_mtx, k, l, val);
        }
    }

    size[0] = row_count;
    size[1] = col_count;
    bounds.r1 = 0;
    bounds.r2 = row_count;
    bounds.c1 = 0;
    bounds.c2 = col_count;

    if ((size[0] == 1 || size[1] == 1) && !(size[0] == 1 && size[1] == 1))
    {
        type = GSL_VECTOR;

        if (size[0] == 1)
        {
            is_row_vector = true;
        }

        _vector_alloc((size[0] == 1 ? size[1] : size[0]));
    }
    else if (size[0] == 1 && size[1] == 1)
    {
        type = GSL_SCALER;
        gsl_scalar = gsl_mtx->data;
    }
}

GodotGSLMatrix::~GodotGSLMatrix()
{
    memdelete_arr((uint8_t*) gsl_mtx);
    GGSL_FREE(gsl_mtx);
    GGSL_FREE(lu_fact);
    GGSL_FREE(perm);
    memdelete(gsl_vec);
    GGSL_FREE(objects);
}


DTYPE *GodotGSLMatrix::_alloc(const size_t row_count, const size_t col_count)
{
    size_t data_size = row_count * col_count;
    size_t total_bytes = MATRIX_SIZE;
    total_bytes += BLOCK_SIZE;
    total_bytes += MULTIPLICITY * data_size * ATOMIC_SIZE;
    uint8_t *mtx_raw = memnew_arr(uint8_t, total_bytes);

    BTYPE block;
    DTYPE mtx;
    memcpy(mtx_raw, &mtx, sizeof(mtx));
    memcpy(mtx_raw + MATRIX_BLOCK_ADDR_SKIP, &block, sizeof(block));

    BTYPE *block_ptr = (BTYPE*) (mtx_raw + MATRIX_BLOCK_ADDR_SKIP);
    DTYPE *mtx_ptr = (DTYPE*) mtx_raw;

    mtx_ptr->data = (STYPE*) (mtx_raw + MATRIX_DATA_ADDR_SKIP);
    mtx_ptr->block = block_ptr;
    block_ptr->data = ((DTYPE*) mtx_raw)->data;
    block_ptr->size = data_size;

    mtx_ptr->size1 = row_count;
    mtx_ptr->size2 = col_count;
    mtx_ptr->tda = col_count;
    mtx_ptr->owner = 1;

    return mtx_ptr;
}

void GodotGSLMatrix::_vector_alloc(size_t s)
{
    gsl_vec = (VTYPE*) memnew(VTYPE);
    gsl_vec->size = s;
    gsl_vec->stride = STRIDE;
    gsl_vec->data = gsl_mtx->data;
    gsl_vec->block = gsl_mtx->block;
    gsl_vec->owner = gsl_mtx->owner;
}

void GodotGSLMatrix::set_zero()
{
    if (gsl_mtx == NULL)
    {
        ERR_FAIL_COND("GodotGSLMatrix::set_zero: gsl_mtx == NULL");
    }

    gsl_matrix_set_zero(gsl_mtx);
}

void GodotGSLMatrix::set_identity()
{
    if (gsl_mtx == NULL)
    {
        ERR_FAIL_COND("GodotGSLMatrix::set_identity: gsl_mtx == NULL");
    }

    gsl_matrix_set_identity(gsl_mtx);
}

int GodotGSLMatrix::add(const GodotGSLMatrix *a, GGSL_BOUNDS *bounds)
{
    if (gsl_mtx == NULL)
    {
        ERR_FAIL_COND_V("GodotGSLMatrix::add: gsl_mtx == NULL", ERR_NULL_VALUE);
    }

    if (size[0] != a->size[0] || size[1] != a->size[1])
    {
        ERR_FAIL_COND_V("GodotGSLMatrix::add: size[0] != a.size[0] || size[1] != a.size[1]", ERR_DIMENSION_MISMATCH);
    }

    if (a->type == GSL_SCALER && type == GSL_SCALER)
    {
        *gsl_scalar += *(a->gsl_scalar);
    }
    else
    {
        size_t row_count = bounds[0].r2 - bounds[0].r1;
        size_t col_count = bounds[0].c2 - bounds[0].c1;

        for (size_t k = 0; k < row_count; k++)
        {
            for (size_t l = 0; l < col_count; l++)
            {
                STYPE val = gsl_matrix_get(gsl_mtx, k + bounds[0].r1, l + bounds[0].c1)
                    + gsl_matrix_get(a->get_ptr(), k + bounds[1].r1, l + bounds[1].c1);

                gsl_matrix_set(gsl_mtx, k + bounds[0].r1, l + bounds[0].c1, val);
            }
        }
    }

    return 0;
}

STYPE GodotGSLMatrix::get(const size_t row, const size_t col)
{
    if (row >= size[0] || col >= size[1])
    {
        ERR_FAIL_COND_V("GodotGSLMatrix::get: row >= size[0] || col >= size[1]", 0.0);
    }

    STYPE ret = gsl_matrix_get(gsl_mtx, row, col);

    return ret;
}

void GodotGSLMatrix::set(const size_t row, const size_t col, const STYPE a)
{
    if (row >= size[0] || col >= size[1])
    {
        ERR_FAIL_COND("GodotGSLMatrix::set: row >= size[0] || col >= size[1]");
    }

    gsl_matrix_set(gsl_mtx, row, col, a);
}

DTYPE *GodotGSLMatrix::get_ptr() const
{
    return gsl_mtx;
}

STYPE *GodotGSLMatrix::get_data() const
{
    return gsl_mtx->data;
}

VTYPE *GodotGSLMatrix::get_vec_ptr() const
{
    return gsl_vec;
}

int GodotGSLMatrix::sub(const GodotGSLMatrix &a)
{
    if (gsl_mtx == NULL)
    {
        ERR_FAIL_COND_V("GodotGSLMatrix::sub: gsl_mtx == NULL", ERR_NULL_VALUE);
    }

    if (size[0] != a.size[0] || size[1] != a.size[1])
    {
        ERR_FAIL_COND_V("GodotGSLMatrix::sub: size[0] != a.size[0] || size[1] != a.size[1]", ERR_DIMENSION_MISMATCH);
    }

    int ret = 0;

    if (a.type == GSL_SCALER && type == GSL_SCALER)
    {
        *gsl_scalar -= *(a.gsl_scalar);
    }
    else
    {
        ret = gsl_matrix_sub(gsl_mtx, a.get_ptr());
    }

    return ret;
}

int GodotGSLMatrix::mul_elements(const GodotGSLMatrix &a)
{
    if (gsl_mtx == NULL)
    {
        ERR_FAIL_COND_V("GodotGSLMatrix::mul: gsl_mtx == NULL", ERR_NULL_VALUE);
    }

    if (size[0] != a.size[0] || size[1] != a.size[1])
    {
        ERR_FAIL_COND_V("GodotGSLMatrix::mul: size[0] != a.size[0] || size[1] != a.size[1]", ERR_DIMENSION_MISMATCH);
    }

    int ret = 0;

    if (a.type == GSL_SCALER && type == GSL_SCALER)
    {
        *gsl_scalar *= *(a.gsl_scalar);
    }
    else
    {
        ret = gsl_matrix_mul_elements(gsl_mtx, a.get_ptr());
    }

    return ret;
}

int GodotGSLMatrix::scale(const STYPE a)
{
    if (gsl_mtx == NULL)
    {
        ERR_FAIL_COND_V("GodotGSLMatrix::mul: gsl_mtx == NULL", ERR_NULL_VALUE);
    }

    int ret = 0;

    if (type == GSL_SCALER)
    {
        *gsl_scalar *= a;
    }
    else
    {
        ret = gsl_matrix_scale(gsl_mtx, a);
    }

    return ret;
}


void GodotGSLMatrix::prod(GodotGSLMatrix &a, GodotGSLMatrix *to, GGSL_BOUNDS *bounds)
{
    int prod_type = -1;

    if ((type == GSL_MATRIX && a.type == GSL_MATRIX)
        || (type == GSL_VECTOR && a.type == GSL_VECTOR))
    {
        prod_type = MATRIX_MATRIX_PROD;
    }
    else
    {
        prod_type = MATRIX_VECTOR_PROD;
    }

    if (prod_type == MATRIX_VECTOR_PROD)
    {
        if (type == GSL_VECTOR)
        {
            if (!is_row_vector)
            {
                gsl_no_blas_gemv(CblasNoTrans, a.get_ptr(), gsl_vec, to->get_vec_ptr(),
                                 bounds[0], bounds[1], bounds[2]);
            }
            else
            {
                gsl_no_blas_gemv(CblasTrans, a.get_ptr(), gsl_vec, to->get_vec_ptr(),
                                 bounds[0], bounds[1], bounds[2]);
            }
        }
        else if (a.type == GSL_VECTOR)
        {
            if (!a.is_row_vector)
            {
                gsl_no_blas_gemv(CblasNoTrans, gsl_mtx, a.get_vec_ptr(), to->get_vec_ptr(),
                                 bounds[0], bounds[1], bounds[2]);
            }
            else
            {
                gsl_no_blas_gemv(CblasTrans, gsl_mtx, a.get_vec_ptr(), to->get_vec_ptr(),
                                 bounds[0], bounds[1], bounds[2]);
            }
        }
    }
    else if (prod_type == MATRIX_MATRIX_PROD || prod_type == VECTOR_VECTOR_PROD)
    {
        gsl_no_blas_gemm(CblasNoTrans, CblasNoTrans, a.get_ptr(), gsl_mtx, to->get_ptr(),
                         bounds[0], bounds[1], bounds[2]);
    }
    else
    {
        WARN_PRINT("GodotGSLMatrix::prod: For scalar products use matrix_scale function.");
    }
}

void GodotGSLMatrix::fx(const String fn, GodotGSLMatrix *a, GodotGSLMatrix *to, GGSL_BOUNDS *bounds)
{
    if (fn == FN_SIN)
    {
        math_func1 = sin;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_COS)
    {
        math_func1 = cos;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_TAN)
    {
        math_func1 = tan;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_EXP)
    {
        math_func1 = exp;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_LOG)
    {
        math_func1 = log;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_LOG10)
    {
        math_func1 = log10;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_SQRT)
    {
        math_func1 = sqrt;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_COSH)
    {
        math_func1 = cosh;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_SINH)
    {
        math_func1 = sinh;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_FLOOR)
    {
        math_func1 = floor;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_ROUND)
    {
        math_func1 = round;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_CEIL)
    {
        math_func1 = ceil;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_EQ)
    {
        math_func1 = ggsl_math_eq;
        _fx_elements1(to, bounds);
        return;
    }
    else if (fn == FN_ADD)
    {
        math_func2 = ggsl_math_add;
        _fx_elements2(a, to, bounds);
        return;
    }
    else if (fn == FN_SUB)
    {
        math_func2 = ggsl_math_sub;
        _fx_elements2(a, to, bounds);
        return;
    }
    else if (fn == FN_MUL_EL)
    {
        math_func2 = ggsl_math_mul_el;
        _fx_elements2(a, to, bounds);
        return;
    }
    else if (fn == FN_DIV_EL)
    {
        math_func2 = ggsl_math_div_el;
        _fx_elements2(a, to, bounds);
        return;
    }
    else if (fn == FN_POW_EL)
    {
        math_func2 = ggsl_math_pow_el;
        _fx_elements2(a, to, bounds);
        return;
    }
    else if (fn == FN_MUL)
    {
        prod(*a, to, bounds);
        return;
    }
    else if (fn == FN_INV)
    {
        invert(a);
        return;
    }
}

void GodotGSLMatrix::fx(const String fn, GodotGSLMatrix *a, GGSL_BOUNDS *bounds)
{
    bounds[2] = bounds[1];
    fx(fn, a, a, bounds);
}

void GodotGSLMatrix::fx(const String fn, GGSL_BOUNDS *bounds)
{
    bounds[2] = bounds[1] = bounds[0];
    fx(fn, this, bounds);
}

void GodotGSLMatrix::_fx_elements1(GodotGSLMatrix *out, GGSL_BOUNDS *bounds)
{
    if (math_func1 == NULL)
    {
        ERR_FAIL_COND("GodotGSLMatrix::_fx_elements1: math_func1 == NULL");
    }

    size_t row_count = bounds[0].r2 - bounds[0].r1;
    size_t col_count = bounds[0].c2 - bounds[0].c1;

    for (size_t k = 0; k < row_count; k++)
    {
        for (size_t l = 0; l < col_count; l++)
        {
            STYPE val = get(k + bounds[0].r1, l + bounds[0].c1);
            val = math_func1(val);
            out->set(k + bounds[1].r1, l + bounds[1].c1, val);
        }
    }

    math_func1 = NULL;
}

void GodotGSLMatrix::_fx_elements2(GodotGSLMatrix *a, GodotGSLMatrix *out, GGSL_BOUNDS *bounds)
{
    if (math_func2 == NULL)
    {
        ERR_FAIL_COND("GodotGSLMatrix::_fx_elements1: math_func1 == NULL");
    }

    size_t row_count = bounds[0].r2 - bounds[0].r1;
    size_t col_count = bounds[0].c2 - bounds[0].c1;

    for (size_t k = 0; k < row_count; k++)
    {
        for (size_t l = 0; l < col_count; l++)
        {
            STYPE val1 = get(k + bounds[0].r1, l + bounds[0].c1);
            STYPE val2 = a->get(k + bounds[1].r1, l + bounds[1].c1);
            val1 = math_func2(val1, val2);
            out->set(k + bounds[2].r1, l + bounds[2].c1, val1);
        }
    }

    math_func2 = NULL;
}

bool GodotGSLMatrix::_condition(const cond_type cond)
{
    return _condition(cond, NULL);
}

bool GodotGSLMatrix::_condition(const cond_type cond, const GodotGSLMatrix *a)
{
    if (cond == EQUAL_SIZE)
    {
        if (size[0] == a->size[0] && size[1] == a->size[1])
        {
            return true;
        }
    }
    else if (cond == NONZERO_SIZE)
    {
        if (size[0] > 0 && size[1] > 0)
        {
            return true;
        }
    }

    return false;
}

void GodotGSLMatrix::copy(GodotGSLMatrix* to)
{
    if (to == NULL)
    {
        GGSL_MESSAGE("GodotGSLMatrix::copy: to == NULL");
        return;
    }

    gsl_matrix_memcpy(to->gsl_mtx, this->gsl_mtx);
}

void GodotGSLMatrix::copy_vector_from_array(const Array a)
{
    if (type != GSL_VECTOR)
    {
        GGSL_MESSAGE("GodotGSLMatrix::copy_vector_from_array: type != GSL_VECTOR");
        return;
    }

    size_t _size;
    if (!is_row_vector)
    {
        _size = size[0];
    }
    else
    {
        _size = size[1];
    }

    for (size_t k = 0; k < _size; k++)
    {
        if (!is_row_vector)
        {
            set(k, 0, a[k]);
        }
        else
        {
            set(0, k, a[k]);
        }
    }
}

void GodotGSLMatrix::invert(GodotGSLMatrix *to)
{
    if (size[0] != size[1])
    {
        GGSL_MESSAGE("GodotGSLMatrix::invert: size[0] != size[1]");
        return;
    }

    _alloc_matrix_inv();

    gsl_matrix_memcpy(lu_fact, gsl_mtx);

    int s;

    gsl_linalg_LU_decomp(lu_fact, perm, &s);
    gsl_linalg_LU_invert(lu_fact, perm, to->get_ptr());
}

void GodotGSLMatrix::_alloc_matrix_inv()
{
    if (perm == NULL)
    {
        size_t size_in_bytes = sizeof(gsl_permutation);
        size_in_bytes += size[0] * sizeof(size_t);
        uint8_t *perm_ptr = memnew_arr(uint8_t, size_in_bytes);
        perm = (gsl_permutation*) perm_ptr;
        perm_ptr += sizeof(gsl_permutation);
        perm->data = (size_t*) perm_ptr;
        perm->size = size[0];
    }

    if (lu_fact == NULL)
    {
        lu_fact = _alloc(size[0], size[1]);
    }
}

void GodotGSLMatrix::add_node_path(Object *obj, const Vector<StringName> key, const int index, const int dir)
{
    if (objects != NULL)
    {
        Object **tmp;
        tmp = objects;
        GGSL_ALLOC_G(objects, ++object_count, Object);
        memcpy(objects, tmp, (object_count - 1) * sizeof(Object*));
        GGSL_FREE(tmp);
    }
    else
    {
        GGSL_ALLOC_G(objects, ++object_count, Object);
    }

    objects[object_count - 1] = obj;

    keys.append(key);
    indices.append(index);
    directions.append(dir);
}

void GodotGSLMatrix::update_node_properties()
{
    for (int k = 0; k < indices.size(); k++)
    {
        int index = indices[k];
        Vector<StringName> key = keys[k];
        Object *obj = objects[k];
        int dir = directions[k];

        if (dir == NODE_P_OUTPUT)
        {
            double value = get(index, 0);
            bool valid = false;
            obj->set_indexed(key, value, &valid);
        }
        else if (dir == NODE_P_INPUT)
        {
            bool valid = false;
            double value = obj->get_indexed(key, &valid);
            set(index, 0, value);
        }
    }
}

void GodotGSLMatrix::set_from_array(const GGSL_BOUNDS &bounds, const Array &arr)
{
    if (bounds.r1 < 0 || bounds.r2 >= size[0]
        || bounds.c1 < 0 || bounds.c2 >= size[1])
    {
        GGSL_ERR_MESSAGE("GodotGSLMatrix::set_from_array: bounds.r1 < 0 || bounds.r2 >= size[0] || bounds.c1 < 0 || bounds.c2 >= size[1]");
        return;
    }

    int row_count = arr.size();

    if (row_count != bounds.r2 - bounds.r1)
    {
        GGSL_ERR_MESSAGE("GodotGSLMatrix::set_from_array: row_count >= bounds.r2 - bounds.r1");
        return;
    }

    for (int k = 0; k < row_count; k++)
    {
        Array row = arr[0];
        int col_count = row.size();
        if (col_count != bounds.c2 - bounds.c1)
        {
            GGSL_ERR_MESSAGE("GodotGSLMatrix::set_from_array: col_count >= bounds.c2 - bounds.c1");
            return;
        }

        for (int l = 0; l < col_count; l++)
        {
            int k_mtx = k + bounds.r1;
            int l_mtx = l + bounds.c1;
            set(k_mtx, l_mtx, row[l]);
        }
    }
}

bool GodotGSLMatrix::is_bounds_included(GGSL_BOUNDS &bounds)
{
    if ((bounds.r1 >= 0 && bounds.r1 < size[0])
        && (bounds.r2 >= 0 && bounds.r2 <= size[0])
        && (bounds.c1 >= 0 && bounds.c1 < size[1])
        && (bounds.c2 >= 0 && bounds.c2 <= size[1]))
    {
        return true;
    }
    else
    {
        return false;
    }
}
