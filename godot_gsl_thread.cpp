#include "godot_gsl_thread.h"

// template <typename T>
// GodotGSLThread<T>::~GodotGSLThread()
// {
//     Thread::wait_to_finish(_thread);
// }

// template <typename T>
// void GodotGSLThread<T>::start()
// {
//     if (this->_terminate)
//     {
//         this->_terminate = false;
//         this->_thread = Thread::create(_main, this);
//     }
// }

// template <typename T>
// void GodotGSLThread<T>::set_loop_object(T obj)
// {
//     _loop_object = obj;
// }

// template <typename T>
// void GodotGSLThread<T>::_main_ptr(GodotGSLThread *t)
// {

// }

// template <typename T>
// void GodotGSLThread<T>::_main()
// {
//     while(!terminate)
//     {
//         _loop_object->loop();
//     }
// }
