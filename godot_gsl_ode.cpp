#include "godot_gsl_ode.h"

GodotGSLODE::~GodotGSLODE()
{
    if (x != NULL)
    {
        memdelete(x);
    }
    if (xdot != NULL)
    {
        memdelete(xdot);
    }
    if (function != NULL)
    {
        memdelete(function);
    }
    if (jacobian != NULL)
    {
        memdelete(jacobian);
    }

    gsl_odeiv2_driver_free(driver);
}

void GodotGSLODE::init(size_t dim)
{
    init(dim, 1e-2, 1e-6);
}

void GodotGSLODE::init(size_t dim, const double hstart, const double epsabs)
{
    x = memnew(GodotGSLMatrix(dim));
    xdot = memnew(GodotGSLMatrix(dim));
    dimension = dim;

    sys.function = (int (*) (double t, const double y[], double dydt[], void *params)) &_function_static;
    sys.jacobian = NULL;
    sys.dimension = dim;
    sys.params = this;
    driver = gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, hstart, epsabs, 0.0);
}

void GodotGSLODE::set_function(GodotGSLFunction *fnc)
{
    function = fnc;
    function->add_argument("x", x);
    function->add_argument("xdot", xdot);
}

void GodotGSLODE::set_jacobian(GodotGSLFunction *fnc)
{
    jacobian = fnc;
}

void GodotGSLODE::run(double t0, double t1, double _dt)
{
    if (_settings.threaded)
    {
        GGSL_ERR_MESSAGE("GodotGSLODE::run: _threaded");
        return;
    }

    dt = _dt;
    size_t step_count = (t1 - t0) / dt;
    t = t0;

    update_node_properties();

    for (size_t k = 0; k < step_count; k++)
    {
        step(dt);
    }
}

int GodotGSLODE::step(double dt)
{
    if (_settings.threaded)
    {
        GGSL_ERR_MESSAGE("GodotGSLODE::run: _threaded");
        return -1;
    }

    double tt = t + dt;
    double *y = x->get_data();
    int status = gsl_odeiv2_driver_apply(driver, &t, tt, y);

    if (status != GSL_SUCCESS)
    {
        GGSL_MESSAGE("GodotGSLODE::step: status != GSL_SUCCESS");
    }
    else
    {
        update_node_properties();
    }

    return status;
}

void GodotGSLODE::set_node_path(Object *obj, const String subpath, const int index)
{
    NodePath property = NodePath(subpath).get_as_property_path();
    Vector<StringName> key = property.get_subnames();

    x->add_node_path(obj, key, index, NODE_P_OUTPUT);
}

void GodotGSLODE::set_node_path_as_input(Object *obj, const String subpath, const String vn, const int index)
{
    NodePath property = NodePath(subpath).get_as_property_path();
    Vector<StringName> key = property.get_subnames();

    GodotGSLMatrix *mtx = function->get_arg(vn);
    if (mtx != NULL)
    {
        mtx->add_node_path(obj, key, index, NODE_P_INPUT);
    }
    else
    {
        GGSL_ERR_MESSAGE("GodotGSLODE::set_node_path_as_input: mtx == NULL");
    }
}

void GodotGSLODE::set_node_path_as_output(Object *obj, const String subpath, const String vn, const int index)
{
    NodePath property = NodePath(subpath).get_as_property_path();
    Vector<StringName> key = property.get_subnames();

    GodotGSLMatrix *mtx = function->get_arg(vn);
    if (mtx != NULL)
    {
        x->add_node_path(obj, key, index, NODE_P_OUTPUT);
    }
}

void GodotGSLODE::update_node_properties()
{
    double p_time = t_property * property_refresh_rate;

    if (t < p_time)
    {
        return;
    }

    for (size_t k = 0; k < function->get_argc(); k++)
    {
        GodotGSLMatrix *mtx = function->get_arg(k);
        mtx->update_node_properties();
    }

    x->update_node_properties();
}

void GodotGSLODE::set_initial_conditions(const Array &x0_arr, const double t0)
{
    x->copy_vector_from_array(x0_arr);
    t = t0;
}

int GodotGSLODE::_function_static(double t, const double y[], double f[], void *params)
{
    GodotGSLODE *ode = (GodotGSLODE*) params;
    return ode->_function(t, y, f, params);
}

int GodotGSLODE::_function(double t, const double y[], double f[], void *params)
{
    (void)(t);

    function->execute();

    size_t size_in_bytes = dimension * sizeof(double);
    memcpy(f, xdot->get_data(), size_in_bytes);

    return GSL_SUCCESS;
}

int __function(double t, const double y[], double f[], void *params)
{
    (void)(t);

    GodotGSLODE *ode = (GodotGSLODE*) params;
    ode->func_execute();

    size_t size_in_bytes = ode->dimension * sizeof(double);
    memcpy(f, ode->xdot->get_data(), size_in_bytes);

    return GSL_SUCCESS;
}

int __jacobian(double t, const double y[], double f[], void *params)
{
    ((GodotGSLODE*) params)->jac_execute();

    return GSL_SUCCESS;
}

void GodotGSLODE::loop()
{
    if (!_ticker)
    {
        return;
    }
    else
    {
        _ticker = false;
    }

    if (!_settings.threaded)
    {
        GGSL_ERR_MESSAGE("GodotGSLODE::run: !_threaded");
        return;
    }

    _lock_tick = true;
    double *y = x->get_data();

    for (double tt = t + dt; t < _settings.next_update_t; tt = t + dt)
    {
        int status = gsl_odeiv2_driver_apply(driver, &t, tt, y);

        if (status != GSL_SUCCESS)
        {
            GGSL_MESSAGE("GodotGSLODE::step: status != GSL_SUCCESS");
            return;
        }
    }

    update_node_properties();
    _lock_tick = false;
}

void GodotGSLODE::tick(const double _dt)
{
    if (_lock_tick)
    {
        return;
    }

    _settings.next_update_t = t + _dt;
    _ticker = true;
}
