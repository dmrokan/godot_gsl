extends Spatial

const LIM = 40
var coord_limits = { 'x': [-LIM, LIM], 'y': [-LIM, LIM], 'z': [-LIM, LIM] }
var obj_count = 8
var lorenz_obj
var obj_list = []
var r_vec = []
var d_vec = []
var b_vec = []
var tmp_vec = []
var gsl_obj

enum {
    L_CHAOS, # From Dmitri Kartofelev presentation
    L_BUTTERFLY, # From Md. Shakhawat Alam [2017]
    }

var rho = 0

func rand_coord():
    var _x = randf() * (coord_limits['x'][1] - coord_limits['x'][0]) + coord_limits['x'][0]
    var _y = randf() * (coord_limits['y'][1] - coord_limits['y'][0]) + coord_limits['y'][0]
    var _z = randf() * (coord_limits['z'][1] - coord_limits['z'][0]) + coord_limits['z'][0]
    
    return Vector3(_x, _y, _z)

func init_param_vectors():
    var type = L_CHAOS
    for k in range(0, obj_count * 3):
        if type == L_CHAOS:
            r_vec.append(28.0)
            d_vec.append(10.0)
            b_vec.append(8.0 / 3.0)
        elif type == L_BUTTERFLY:
            r_vec.append(28.0)
            d_vec.append(10.0)
            b_vec.append(8.0 / 3.0)
        tmp_vec.append(0.0)

func init_objects():
    var tr_vec = Vector3()
    lorenz_obj.visible = true
    # lorenz_obj.translate(rand_coord())
    var beta = 8.0 / 3.0
    var rho = 28.0
    var eta = sqrt(beta * (rho - 1.0))
    var yc = [rho - 1.0, eta, eta]
    lorenz_obj.translate(Vector3(yc[0], yc[1], 3.0 + yc[2]))
    lorenz_obj.scale *= 5.0
    var mat = lorenz_obj.get_surface_material(0).duplicate()
    mat.albedo_color = Color(1.0, 0.0, 0.0)
    lorenz_obj.set_surface_material(0, mat)
    add_child(lorenz_obj)
    obj_list.append(lorenz_obj)
    for k in range(1, obj_count):
        var new_obj = $LorenzObj.duplicate()
        new_obj.visible = true
        new_obj.translate(rand_coord())
        new_obj.scale *= 5.0
        mat = mat.duplicate()
        mat.albedo_color = Color(0.0, 1.0, 0.0)
        new_obj.set_surface_material(0, mat)
        add_child(new_obj)
        obj_list.append(new_obj)
    
func init_ode():
    var c = obj_list.size()
    if c != obj_count:
        assert false
    
    init_param_vectors()
    
    gsl_obj = GodotGSL.new()
    gsl_obj.vector_new_from_array("b", b_vec)
    gsl_obj.vector_new_from_array("d", d_vec)
    gsl_obj.vector_new_from_array("r", r_vec)
    gsl_obj.vector_new_from_array("tmp", tmp_vec)
    gsl_obj.vector_new_from_array("tmp2", tmp_vec)
    
    gsl_obj.function("fx", ["b", "d", "r", "tmp", "tmp2"])
    gsl_obj.ode("lorenz", obj_count * 3)
    gsl_obj.ode_set_fx("lorenz", "fx")
    
    var x0 = []
    
    var k = 0
    for obj in obj_list:
        var sub_mtx_str1 = "[" + str(k) + ":" + str(k + 1) + "]"
        var sub_mtx_str2 = "[" + str(k + 1) + ":" + str(k + 2) + "]"
        var sub_mtx_str3 = "[" + str(k + 2) + ":" + str(k + 3) + "]"
        
        gsl_obj.instruction("-", [ "x" + sub_mtx_str2, "x" + sub_mtx_str1, "tmp" + sub_mtx_str1 ])
        gsl_obj.instruction(".*", [ "tmp" + sub_mtx_str1, "d" + sub_mtx_str1, "xdot" + sub_mtx_str1 ])
        
        gsl_obj.instruction(".*", [ "x" + sub_mtx_str1, "x" + sub_mtx_str3, "tmp" + sub_mtx_str2 ])
        gsl_obj.instruction("+", [ "x" + sub_mtx_str2, "tmp" + sub_mtx_str2, "tmp" + sub_mtx_str2 ])
        gsl_obj.instruction(".*", [ "x" + sub_mtx_str1, "r" + sub_mtx_str1, "tmp2" + sub_mtx_str2 ])
        gsl_obj.instruction("-", [ "tmp2" + sub_mtx_str2, "tmp" + sub_mtx_str2, "xdot" + sub_mtx_str2 ])
        
        gsl_obj.instruction(".*", [ "x" + sub_mtx_str1, "x" + sub_mtx_str2, "tmp" + sub_mtx_str3 ])
        gsl_obj.instruction(".*", [ "x" + sub_mtx_str3, "b" + sub_mtx_str1, "tmp2" + sub_mtx_str3 ])
        gsl_obj.instruction("-", [ "tmp" + sub_mtx_str3, "tmp2" + sub_mtx_str3, "xdot" + sub_mtx_str3 ])
        
        gsl_obj.ode_set_node_path("lorenz", obj, "translation:x", k)
        gsl_obj.ode_set_node_path("lorenz", obj, "translation:y", k + 1)
        gsl_obj.ode_set_node_path("lorenz", obj, "translation:z", k + 2)
        
        x0.append(obj.translation.x)
        x0.append(obj.translation.y)
        x0.append(obj.translation.z)
        
        k += 3
    
    gsl_obj.ode_set_init_cond("lorenz", x0, 0.0)
#    var xdot = gsl_obj.to_array("tmp2")
#    print(xdot)    
#    gsl_obj.callf("fx")
#    xdot = gsl_obj.to_array("tmp2")
#    print(xdot)

func _ready():
    lorenz_obj = $LorenzObj.duplicate()
    init_objects()
    init_ode()
    gsl_obj.ode_run_threaded("lorenz_t", "lorenz", 1e-3, 0.0)

var time_t = 2.0
func _process(delta):
    # return
    if gsl_obj == null:
        return
    # gsl_obj.ode_run_delta("lorenz", delta / 1e1)
    gsl_obj.ode_tick("lorenz", delta)
    if time_t > 1.0:
        print(obj_list[0].translation)
        time_t = 0.0
    time_t += delta
