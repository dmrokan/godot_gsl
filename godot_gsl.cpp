/* godot_gsl.cpp */

#include "godot_gsl.h"

void GodotGSL::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("matrix_new", "vn", "row_count", "col_count"), &GodotGSL::matrix_new);
    ClassDB::bind_method(D_METHOD("matrix_new_from_array", "vn", "a"), &GodotGSL::matrix_new_from_array);
    ClassDB::bind_method(D_METHOD("add", "vn1", "vn2"), &GodotGSL::add);
    ClassDB::bind_method(D_METHOD("to_array", "vn", "row_interval", "col_interval"), &GodotGSL::to_array, "", Array(), Array());
    ClassDB::bind_method(D_METHOD("set_zero", "vn"), &GodotGSL::set_zero);
    ClassDB::bind_method(D_METHOD("prod", "vn1", "vn2", "to"), &GodotGSL::prod);
    ClassDB::bind_method(D_METHOD("vector_new", "vn", "size", "is_row_vector"), &GodotGSL::vector_new, NULL, NULL, false);
    ClassDB::bind_method(D_METHOD("vector_new_from_array", "vn", "a", "is_row_vector"), &GodotGSL::vector_new_from_array, NULL, NULL, false);
    ClassDB::bind_method(D_METHOD("fx", "fn", "a", "to"), &GodotGSL::fx, "=", "", "");
    ClassDB::bind_method(D_METHOD("function", "fn", "args"), &GodotGSL::function, "", Array());
    ClassDB::bind_method(D_METHOD("instruction", "ins_name", "args"), &GodotGSL::instruction, "", Array());
    ClassDB::bind_method(D_METHOD("callf", "fn"), &GodotGSL::callf);
    ClassDB::bind_method(D_METHOD("ode", "on", "dim"), &GodotGSL::ode);
    ClassDB::bind_method(D_METHOD("ode_set_fx", "on", "fn"), &GodotGSL::ode_set_fx);
    ClassDB::bind_method(D_METHOD("ode_run", "on", "x0", "time_interval", "dt"), &GodotGSL::ode_run);
    ClassDB::bind_method(D_METHOD("ode_set_node_path", "on", "object", "property", "index"), &GodotGSL::ode_set_node_path);
    ClassDB::bind_method(D_METHOD("ode_set_node_path_as_input", "on", "object", "property", "vn", "index"), &GodotGSL::ode_set_node_path_as_input);
    ClassDB::bind_method(D_METHOD("ode_set_init_cond", "on", "x0", "t0"), &GodotGSL::ode_set_init_cond);
    ClassDB::bind_method(D_METHOD("ode_run_delta", "on", "dt"), &GodotGSL::ode_run_delta);
    ClassDB::bind_method(D_METHOD("matrix_set_identity", "vn"), &GodotGSL::matrix_set_identity);
    ClassDB::bind_method(D_METHOD("matrix_set", "vn", "row", "col", "value"), &GodotGSL::matrix_set);
    ClassDB::bind_method(D_METHOD("matrix_set_from_array", "vn", "row_interval", "col_interval", "value"), &GodotGSL::matrix_set_from_array);
    ClassDB::bind_method(D_METHOD("matrix_get", "vn", "row", "col"), &GodotGSL::matrix_get);
    ClassDB::bind_method(D_METHOD("ode_run_threaded", "tn", "on", "dt", "update_dt"), &GodotGSL::ode_run_threaded);
    ClassDB::bind_method(D_METHOD("ode_tick", "on", "dt"), &GodotGSL::ode_tick);
}

GodotGSL::GodotGSL()
{
    return;
}

GodotGSL::~GodotGSL()
{
    variables.clear();
    functions.clear();
    odes.clear();
    threads.clear();
}

void GodotGSL::_add_variable(String vn, GodotGSLMatrix* mtx)
{
    if (variables.has(vn))
    {
        variables.erase(vn);
        variables[vn] = mtx;
    }
    else
    {
        variables.insert(vn, mtx);
    }
}

void GodotGSL::matrix_new(String vn, const size_t row_count, const size_t col_count)
{
    GodotGSLMatrix *mtx = memnew(GodotGSLMatrix);
    mtx->init(row_count, col_count);

    _add_variable(vn, mtx);
}

void GodotGSL::matrix_new_from_array(String vn, const Array a)
{
    GodotGSLMatrix *mtx = memnew(GodotGSLMatrix);
    mtx->init(a);

    _add_variable(vn, mtx);
}

void GodotGSL::add(String vn1, String vn2)
{
    GodotGSLMatrix *mtx1 = variables[vn1];
    GodotGSLMatrix *mtx2 = variables[vn2];

    mtx1->add(mtx2, NULL);
}

void GodotGSL::set_zero(String vn)
{
    GodotGSLMatrix *mtx = variables[vn];

    mtx->set_zero();
}

Array GodotGSL::to_array(String vn, const Array row_interval, const Array col_interval)
{
    GodotGSLMatrix *mtx;
    GGSL_HAS_V(variables, vn, mtx, Array());

    GGSL_BOUNDS bounds;
    if (row_interval.empty())
    {
        bounds.r1 = 0;
        bounds.r2 = mtx->size[0];
    }
    else
    {
        bounds.r1 = row_interval[0];
        bounds.r2 = row_interval[1];

        if (bounds.r1 > bounds.r2)
        {
            GGSL_ERR_MESSAGE("GodotGSL::to_array: bounds.r1 > bounds.r2");
            return Array();
        }
    }

    if (col_interval.empty())
    {
        bounds.c1 = 0;
        bounds.c2 = mtx->size[1];
    }
    else
    {
        bounds.c1 = col_interval[0];
        bounds.c2 = col_interval[1];

        if (bounds.c1 > bounds.c2)
        {
            GGSL_ERR_MESSAGE("GodotGSL::to_array: bounds.c1 > bounds.c2");
            return Array();
        }
    }

    if (!mtx->is_bounds_included(bounds))
    {
        GGSL_ERR_MESSAGE("GodotGSL::to_array: !mtx->is_bounds_included(bounds)");
        return Array();
    }

    size_t row_count = bounds.r2 - bounds.r1;
    size_t col_count = bounds.c2 - bounds.c1;

    Array rows;
    for (size_t k = 0; k < row_count; k++)
    {
        Array row;
        for (size_t l = 0; l < col_count; l++)
        {
            row.append(mtx->get(k + bounds.r1, l + bounds.c1));
        }

        rows.append(row);
    }

    return rows;
}

void GodotGSL::prod(String vn1, String vn2, String to)
{
    GodotGSLMatrix *mtx1 = variables[vn1];
    GodotGSLMatrix *mtx2 = variables[vn2];

    GodotGSLMatrix *result = NULL;
    mtx2->prod(*mtx1, result, NULL);

    _add_variable(to, result);
}


void GodotGSL::vector_new(String vn, const size_t size, bool is_row_vector)
{
    GodotGSLMatrix *mtx = memnew(GodotGSLMatrix);

    size_t row_count = size;
    size_t col_count = 1;

    if (is_row_vector)
    {
        row_count = 1;
        col_count = size;
    }

    mtx->init(row_count, col_count);

    _add_variable(vn, mtx);
}

void GodotGSL::vector_new_from_array(String vn, const Array a, bool is_row_vector)
{
    GodotGSLMatrix *mtx = memnew(GodotGSLMatrix);

    Array b;

    if (is_row_vector)
    {
        b.append(a);
    }
    else
    {
        size_t size = a.size();
        for (size_t k = 0; k < size; k++)
        {
            STYPE value = a.get(k);
            Array c;
            c.append(value);
            b.append(c);
        }
    }

    mtx->init(b);

    _add_variable(vn, mtx);
}

void GodotGSL::fx(const String fn, const String a, String to)
{
    GodotGSLMatrix *mtx_a;
    GodotGSLMatrix *mtx_to;
    // GGSL_HAS(variables, a, mtx_a);
    // GGSL_HAS(variables, to, mtx_to);

    if (fn == FN_EQ)
    {
        if (to.empty())
        {
            return;
        }
    }
    else if (fn == FN_SIN)
    {
        if (!variables.has(a))
        {
            GGSL_MESSAGE("GodotGSL::fx: !variables.has(a)");
            return;
        }

        if (to.empty())
        {
            to = a;
        }
        else
        {
            if (!variables.has(to))
            {
                GGSL_MESSAGE("GodotGSL::fx: !variables.has(to)");
                return;
            }
        }

        mtx_a = variables[a];
        mtx_to = variables[to];
        /* TODO: find a way to supply default bounds
         * Possibly you are going to put this in fx
         * Cause you have all bound informations of matrices
         */
        mtx_a->fx(fn, mtx_to, NULL);
    }
    else if (fn == FN_INV)
    {
        mtx_a = variables[a];
        mtx_to = variables[to];
        printf("srsrwsreersew %s\n", fn.utf8().get_data());
        mtx_a->fx(fn, mtx_to, NULL);
    }

    /*
     * TODO: If there is no 'to' variable
     * It may be added automaticall
     * SOON

    _add_variable(to, mtx_to);

    */
}

void GodotGSL::_add_function(const String fn, GodotGSLFunction *fnc)
{
    if (functions.has(fn))
    {
        functions.erase(fn);
        functions[fn] = fnc;
    }
    else
    {
        functions.insert(fn, fnc);
    }
}

void GodotGSL::function(const String fn, const Array args)
{
    GodotGSLFunction *fnc = memnew(GodotGSLFunction(fn));
    _add_function(fn, fnc);

    int size = args.size();
    if (size > 0)
    {
        GodotGSLMatrix **argv;
        GGSL_ALLOC(argv, size);

        for (int k = 0; k < size; k++)
        {
            String vn = args[k];
            if (variables.has(vn))
            {
                argv[k] = variables[vn];
            }
            else
            {
                GGSL_MESSAGE("GodotGSL::function: !variables.has(vn)");
                GGSL_MESSAGE(vn.utf8().get_data());
                return;
            }
        }

        fnc->add_arguments(args, argv);
    }

    current = fnc;
}

void GodotGSL::instruction(const String in, const Array args)
{
    if (current == NULL)
    {
        GGSL_MESSAGE("GodotGSL::instruction: current == NULL");
        return;
    }

    current->add_instruction(in, args);
}

void GodotGSL::callf(const String fn)
{
    GGSL_DEBUG_MSG("GodotGSL::callf: procedure starts", 0);
    if (!functions.has(fn))
    {
        GGSL_MESSAGE("GodotGSL::call: !functions.has(fn)");
        return;
    }

    GodotGSLFunction *fnc = functions[fn];
    fnc->execute();
    GGSL_DEBUG_MSG("GodotGSL::callf: procedure ends", 0);
}

void GodotGSL::ode(const String on, const size_t dim)
{
    if (odes.has(on))
    {
        GodotGSLODE *to_rm = odes[on];
        odes[on] = memnew(GodotGSLODE(dim));
        delete to_rm;
    }
    else
    {
        odes[on] = memnew(GodotGSLODE(dim));
    }
}

void GodotGSL::ode_set_fx(const String on, const String fn)
{
    GodotGSLODE *ode;
    GodotGSLFunction *fnc;

    if (odes.has(on))
    {
        ode = odes[on];
    }
    else
    {
        GGSL_MESSAGE("GodotGSL::ode_set_fx: !odes.has(on)");
        return;
    }

    if (functions.has(fn))
    {
        fnc = functions[fn];
    }
    else
    {
        GGSL_MESSAGE("GodotGSL::ode_set_fx: !functions.has(on)");
        return;
    }

    ode->set_function(fnc);
}

void GodotGSL::ode_run(const String on, const Array x0, const Array time_interval, const double dt)
{
    if (!odes.has(on))
    {
        GGSL_MESSAGE("GodotGSL::ode_run: !odes.has(on)");
        return;
    }

    GodotGSLODE *ode = odes[on];
    ode->x->copy_vector_from_array(x0);

    double start_time, end_time;
    if (time_interval.size() == 1)
    {
        start_time = 0.0;
        end_time = time_interval[0];
    }
    else if (time_interval.size() == 2)
    {
        start_time = time_interval[0];
        end_time = time_interval[1];
    }

    ode->run(start_time, end_time, dt);
}

void GodotGSL::ode_set_init_cond(const String on, const Array x0, const double t0)
{
    if (!odes.has(on))
    {
        GGSL_MESSAGE("GodotGSL::ode_run: !odes.has(on)");
        return;
    }

    GodotGSLODE *ode = odes[on];
    ode->set_initial_conditions(x0, t0);
}

void GodotGSL::ode_run_delta(const String on, const double delta)
{
    if (!odes.has(on))
    {
        GGSL_MESSAGE("GodotGSL::ode_run: !odes.has(on)");
        return;
    }

    GodotGSLODE *ode = odes[on];

    ode->step(delta);
}

void GodotGSL::ode_set_node_path(const String on, Variant obj_var, const String subpath, const int index)
{
    if (!odes.has(on))
    {
        GGSL_ERR_MESSAGE("GodotGSL::ode_set_node_path: !odes.has(on)");
        return;
    }

    Object *obj = (Object*) obj_var;

    GodotGSLODE *ode = odes[on];
    /* TODO: Use weak ptr */
    ode->set_node_path(obj, subpath, index);
}

void GodotGSL::ode_set_node_path_as_input(const String on, Variant obj_var, const String subpath, const String vn, const int index)
{
    if (!odes.has(on))
    {
        GGSL_ERR_MESSAGE("GodotGSL::ode_set_node_path: !odes.has(on)");
        return;
    }

    Object *obj = (Object*) obj_var;

    GodotGSLODE *ode = odes[on];
    /* TODO: Use weak ptr */
    ode->set_node_path_as_input(obj, subpath, vn, index);
}

void GodotGSL::matrix_set_identity(const String vn)
{
    GodotGSLMatrix *mtx;
    GGSL_HAS(variables, vn, mtx);

    mtx->set_identity();
}

void GodotGSL::matrix_set(const String vn, int row, int col, STYPE value)
{
    GodotGSLMatrix *mtx;
    GGSL_HAS(variables, vn, mtx);

    mtx->set(row, col, value);
}

void GodotGSL::matrix_set_from_array(const String vn, const Array row_interval, const Array col_interval, const Array arr)
{
    GodotGSLMatrix *mtx;
    GGSL_HAS(variables, vn, mtx);

    if (row_interval.size() != 2 || col_interval.size() != 2)
    {
        GGSL_ERR_MESSAGE("GodotGSL::matrix_set_from_array: row_interval.size() != 2 || col_interval.size() != 2");
        return;
    }

    int row1 = row_interval[0];
    int row2 = row_interval[1];
    int col1 = col_interval[0];
    int col2 = col_interval[1];

    if (row1 > row2 || col1 > col2)
    {
        GGSL_ERR_MESSAGE("GodotGSL::matrix_set_from_array: row1 > row2 || col1 > col2");
        return;
    }

    if (arr.size() == 0 || arr[0].get_type() != Variant::ARRAY)
    {
        GGSL_ERR_MESSAGE("GodotGSL::matrix_set_from_array: arr.size() == 0 || arr[0].get_type() != Variant::ARRAY");
        return;
    }

    GGSL_BOUNDS bounds;
    bounds.r1 = row_interval[0];
    bounds.r2 = row_interval[1];
    bounds.c1 = col_interval[0];
    bounds.c2 = col_interval[1];

    mtx->set_from_array(bounds, arr);
}

void GodotGSL::vector_set_from_array(const String vn, const Array row_interval, const Array arr)
{
    Array mtx_arr;

    for (int k = 0; k < arr.size(); k++)
    {
        Array row;
        row.append(arr[k]);
        mtx_arr.append(row);
    }

    Array col_interval;
    col_interval.append(0);
    col_interval.append(1);

    matrix_set_from_array(vn, row_interval, col_interval, mtx_arr);
}

STYPE GodotGSL::matrix_get(const String vn, int row, int col)
{
    GodotGSLMatrix *mtx;
    GGSL_HAS_V(variables, vn, mtx, 0.0);

    return mtx->get(row, col);
}

void GodotGSL::ode_run_threaded(const String tn, const String on, const double dt, const double update_dt)
{
    GodotGSLThread<GodotGSLODE*> *thread = _add_thread(tn);

    GodotGSLODE *ode;
    GGSL_HAS(odes, on, ode);

    /* 3rd param 'true' is assigned to var 'threaded' */
    ode->settings(dt, update_dt, true);

    thread->set_loop_object(ode);
    thread->start();
}

GodotGSLThread<GodotGSLODE*> *GodotGSL::_add_thread(const String tn)
{
    if (threads.has(tn))
    {
        GGSL_MESSAGE("GodotGSL::_add_thread: threads.has(tn)");

        return threads[tn];
    }
    else
    {
        GodotGSLThread<GodotGSLODE*> *thread = memnew(GodotGSLThread<GodotGSLODE*>(tn));

        return thread;
    }
}

void GodotGSL::ode_tick(const String on, const double dt)
{
    GodotGSLODE *ode;
    GGSL_HAS(odes, on, ode);

    ode->tick(dt);
}
