#ifndef GODOT_GSL_THREAD_H
#define GODOT_GSL_THREAD_H

#include "core/os/thread.h"

template <typename T>
class GodotGSLThread {
public:
    GodotGSLThread(const String tn) { name = tn; }
    ~GodotGSLThread() {
        Thread::wait_to_finish(_thread);
    }
    void start() {
        if (_terminate)
        {
            _terminate = false;
            _thread = Thread::create((ThreadCreateCallback) _main_ptr, this);
        }
    }
    void set_loop_object(T obj) {
        _loop_object = obj;
    }
    void terminate() { _terminate = true; }

private:
    String name;
    Thread *_thread;
    T _loop_object;
    static void _main_ptr(GodotGSLThread *t) {
        t->_main();
    }
    void _main() {
        while(!_terminate)
        {
            _loop_object->loop();
        }
    }
    bool _terminate = true;
};

#endif
