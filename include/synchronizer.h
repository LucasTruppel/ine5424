// EPOS Synchronizer Components

#ifndef __synchronizer_h
#define __synchronizer_h

#include <architecture.h>
#include <utility/handler.h>
#include <process.h>

__BEGIN_SYS

class Synchronizer_Common
{
protected:
    typedef Thread::Queue Queue;
    typedef Thread::Criterion Criterion;


protected:
    Synchronizer_Common(long size): _size(size) {
        _thread_array = new Thread*[size];
        for (int i = 0; i < size; i++) {
            _thread_array[i] = nullptr;
        }
    }
    
    ~Synchronizer_Common() { 
        begin_atomic(); 
        wakeup_all(); 
        end_atomic();
        delete[] _thread_array;
    }

    // Atomic operations
    bool tsl(volatile bool & lock) { return CPU::tsl(lock); }
    long finc(volatile long & number) { return CPU::finc(number); }
    long fdec(volatile long & number) { return CPU::fdec(number); }

    // Thread operations
    void begin_atomic() { Thread::lock(); }
    void end_atomic() { Thread::unlock(); }

    void sleep() { Thread::sleep(&_queue); }
    void wakeup() { Thread::wakeup(&_queue); }
    void wakeup_all() { Thread::wakeup_all(&_queue); }

    void insert() {
        if (priority_inversion_protocol == Priority_Inversion_Protocol::NONE)
            return;

        Thread* current_thread = Thread::running();
        for (int i = 0; i <_size; i++) {
            if (_thread_array[i] == nullptr) {
                _thread_array[i] = current_thread;
                break;
            }
        }

    }

    void apply_new_priority() {
        if (priority_inversion_protocol == Priority_Inversion_Protocol::NONE)
            return;

        Thread* current_thread = Thread::running();
        for (int i = 0; i < _size; i++) {
            if (current_thread->priority() < _thread_array[i]->priority()) {
                _thread_array[i]->apply_new_priority(get_new_priority(current_thread));
                db<Synchronizer>(INF) << "\nPriority inversion protocol applied!";
                return;
            }
        }
    }

    void restore_priority() {
        if (priority_inversion_protocol == Priority_Inversion_Protocol::NONE)
            return;

        Thread* current_thread = Thread::running();
        if (current_thread->criterion().protocol_applied()) {
            for (int i = 0; i < _size; i++) {
                if (_thread_array[i] == current_thread) {
                    _thread_array[i]->restore_priority();
                    db<Synchronizer>(INF) << "\nPriority inversion protocol restored!";
                    return;
                }
            }
        }
    }

private:
    int get_new_priority(Thread* current_thread) {
        switch (priority_inversion_protocol)
        {
        case Priority_Inversion_Protocol::NONE:  // Never runs.
            return 0;
        case Priority_Inversion_Protocol::CEILING:
            return Criterion::CEILING;
        case Priority_Inversion_Protocol::INHERITANCE:
            return current_thread->priority(); 
        }
    }

protected:
    Queue _queue;
    Thread** _thread_array;
    long _size;
    static const int priority_inversion_protocol = Traits<Synchronizer>::priority_inversion_protocol;
};


class Mutex: protected Synchronizer_Common
{
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

private:
    volatile bool _locked;
};


class Semaphore: protected Synchronizer_Common
{
public:
    Semaphore(long v = 1);
    ~Semaphore();

    void p();
    void v();

private:
    volatile long _value;
};


// This is actually no Condition Variable
// check http://www.cs.duke.edu/courses/spring01/cps110/slides/sem/sld002.htm
class Condition: protected Synchronizer_Common
{
public:
    Condition();
    ~Condition();

    void wait();
    void signal();
    void broadcast();
};


// An event handler that triggers a mutex (see handler.h)
class Mutex_Handler: public Handler
{
public:
    Mutex_Handler(Mutex * h) : _handler(h) {}
    ~Mutex_Handler() {}

    void operator()() { _handler->unlock(); }

private:
    Mutex * _handler;
};

// An event handler that triggers a semaphore (see handler.h)
class Semaphore_Handler: public Handler
{
public:
    Semaphore_Handler(Semaphore * h) : _handler(h) {}
    ~Semaphore_Handler() {}

    void operator()() { _handler->v(); }

private:
    Semaphore * _handler;
};

// An event handler that triggers a condition variable (see handler.h)
class Condition_Handler: public Handler
{
public:
    Condition_Handler(Condition * h) : _handler(h) {}
    ~Condition_Handler() {}

    void operator()() { _handler->signal(); }

private:
    Condition * _handler;
};

__END_SYS

#endif
