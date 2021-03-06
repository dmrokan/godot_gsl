#ifndef GODOT_GSL_ODE_H
#define GODOT_GSL_ODE_H

#include "godot_gsl_function.h"
#include "ode-initval2/gsl_odeiv2.h"

#define PARAM_COUNT_STEP 5
#define ODE_DELTA_T 1e-3
#define DEFAULT_HSTART 1e-2
#define DEFAULT_EPSABS 1e-6

int __function(double t, const double y[], double f[], void *params);
int __jacobian(double t, const double y[], double *dfdy, double dfdt[], void *params);

class GodotGSLODE
{
public:
    GodotGSLODE() { }
    GodotGSLODE(size_t dim) { init(dim); };
    GodotGSLODE(size_t dim, const double hstart, const double epsabs) { init(dim, hstart, epsabs); };
    GodotGSLODE(size_t dim, bool threaded) { init(dim); _settings.threaded = threaded; };
    GodotGSLODE(size_t dim, const double hstart, const double epsabs, bool threaded) { init(dim, hstart, epsabs); _settings.threaded = threaded; };
    ~GodotGSLODE();
    void init(size_t dim);
    void init(size_t dim, const double hstart, const double epsabs);
    void set_function(GodotGSLFunction *fnc);
    void set_jacobian(GodotGSLFunction *fnc);
    void run(double t0, double t1, double _dt);
    int step(double dt);
    void func_execute() { function->execute(); }
    void jac_execute() { jacobian->execute(); }
    void set_node_path(Object *obj, const String subpath, const int index);
    void set_node_path_as_input(Object *obj, const String subpath, const String vn, const int index);
    void set_node_path_as_output(Object *obj, const String subpath, const String vn, const int index);
    void update_node_properties();
    void set_initial_conditions(const Array &x0_arr, const double t0);
    void settings(const double dt_arg, const double update_dt, const bool threaded_arg) {
        dt = dt_arg;
        _settings.update_dt = update_dt;
        _settings.threaded = threaded_arg;
    }
    void loop();
    void tick(const double _dt);
    GodotGSLMatrix *x = NULL;
    GodotGSLMatrix *xdot = NULL;
    size_t dimension = 0;

private:
    GodotGSLFunction *function = NULL;
    GodotGSLFunction *jacobian = NULL;
    static int _function_static(double t, const double y[], double f[], void *params);
    static int _jacobian_static(double t, const double y[], double *dfdy, double dfdt[], void *params);
    int _function(double t, const double y[], double f[], void *params);
    int _jacobian(double t, const double y[], double *dfdy, double dfdt[], void *params);
    gsl_odeiv2_system sys;
    gsl_odeiv2_driver *driver;
    double t = 0.0;
    double dt = 0.0;
    double property_refresh_rate;
    unsigned int t_property = 0;

    struct Settings {
        bool threaded = false;
        double next_update_t = 0.0;
        double update_dt = 0.0;
    };
    Settings _settings;
    bool _ticker = false;
    bool _lock_tick = false;
};

#endif
