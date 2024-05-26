// EPOS Thread Implementation

#include <machine.h>
#include <system.h>
#include <process.h>
#include <time.h>

extern "C" { volatile unsigned long _running() __attribute__ ((alias ("_ZN4EPOS1S6Thread4selfEv"))); }

__BEGIN_SYS

bool Thread::_not_booting;
volatile unsigned int Thread::_thread_count;
Scheduler_Timer * Thread::_timer;
Scheduler<Thread> Thread::_scheduler;
Spin Thread::_lock;


unsigned long Thread::init_timestamp = 0;


void Thread::constructor_prologue(unsigned int stack_size)
{
    lock();

    if (profiler && _thread_count == 0)
        init_timestamp = CLINT::mtime();

    _thread_count++;
    _scheduler.insert(this);

    _stack = new (SYSTEM) char[stack_size];
}


void Thread::constructor_epilogue(Log_Addr entry, unsigned int stack_size)
{
    db<Thread>(TRC) << "Thread(entry=" << entry
                    << ",state=" << _state
                    << ",priority=" << _link.rank()
                    << ",stack={b=" << reinterpret_cast<void *>(_stack)
                    << ",s=" << stack_size
                    << "},context={b=" << _context
                    << "," << *_context << "}) => " << this << endl;

    assert((_state != WAITING) && (_state != FINISHING)); // invalid states

    if((_state != READY) && (_state != RUNNING))
        _scheduler.suspend(this);

    if(preemptive && (_state == READY) && (_link.rank() != IDLE)) {
        if (multicore && Criterion::global) {
            reschedule_all_cpus();
        } else {
            reschedule(criterion().queue());
        }
    }

    unlock();
}


Thread::~Thread()
{
    lock();

    db<Thread>(TRC) << "~Thread(this=" << this
                    << ",state=" << _state
                    << ",priority=" << _link.rank()
                    << ",stack={b=" << reinterpret_cast<void *>(_stack)
                    << ",context={b=" << _context
                    << "," << *_context << "})" << endl;

    // The running thread cannot delete itself!
    assert(_state != RUNNING);

    switch(_state) {
    case RUNNING:  // For switch completion only: the running thread would have deleted itself! Stack wouldn't have been released!
        exit(-1);
        break;
    case READY:
        _scheduler.remove(this);
        _thread_count--;
        break;
    case SUSPENDED:
        _scheduler.resume(this);
        _scheduler.remove(this);
        _thread_count--;
        break;
    case WAITING:
        _waiting->remove(this);
        _scheduler.resume(this);
        _scheduler.remove(this);
        _thread_count--;
        break;
    case FINISHING: // Already called exit()
        break;
    }

    if(_joining)
        _joining->resume();

    unlock();

    delete _stack;
}

Thread * volatile Thread::self() { 
    return _not_booting ? running() : reinterpret_cast<Thread * volatile>(CPU::id() + 1); 
}

void Thread::priority(const Criterion & c)
{
    lock();

    db<Thread>(TRC) << "Thread::priority(this=" << this << ",prio=" << c << ")" << endl;

    unsigned int current_cpu = criterion().queue();
    unsigned int next_cpu = c.queue();

    if(_state != RUNNING) { // reorder the scheduling queue
        _scheduler.remove(this);
        _link.rank(c);
        _scheduler.insert(this);
    } else
        _link.rank(c);

    if(preemptive) {

        if (multicore && Criterion::global) {
            reschedule_all_cpus();
        }
            
        if (multicore && Criterion::partitioned) {
            if (current_cpu != CPU::id()) {
                reschedule(current_cpu);
            }
            if (next_cpu != CPU::id() && current_cpu != next_cpu) {
                reschedule(next_cpu);
            }
            if (current_cpu == CPU::id() || next_cpu == CPU::id()) {
                reschedule();
            }
        } 

        if (!multicore) {
            reschedule();
        }
    }

    unlock();
}

void Thread::apply_new_priority(int new_priority)
{
    switch(_state) {
    case RUNNING:
        assert(multicore);
        criterion().apply_new_priority(new_priority);
        break;
    case READY:
        _scheduler.remove(this);
        criterion().apply_new_priority(new_priority);
        _scheduler.insert(this);
        break;
    case SUSPENDED:
        criterion().apply_new_priority(new_priority);
        break;
    case WAITING:
        _waiting->remove(&_link);
        criterion().apply_new_priority(new_priority);
        _waiting->insert(&_link);
        break;
    case FINISHING:
        break;
    }
}

void Thread::restore_priority()
{
    switch(_state) {
    case RUNNING:
        criterion().restore_priority();
        break;
    case READY:
        _scheduler.remove(this);
        criterion().restore_priority();
        _scheduler.insert(this);
        break;
    case SUSPENDED:
        criterion().restore_priority();
        break;
    case WAITING:
        _waiting->remove(&_link);
        criterion().restore_priority();
        _waiting->insert(&_link);
        break;
    case FINISHING:
        break;
    }
}

int Thread::join()
{
    lock();
    db<Thread>(TRC) << "Thread::join(this=" << this << ",state=" << _state << ")" << endl;

    // Precondition: no Thread::self()->join()
    assert(running() != this);

    // Precondition: a single joiner
    assert(!_joining);

    if(_state != FINISHING) {
        Thread * prev = running();

        _joining = prev;
        prev->_state = SUSPENDED;
        _scheduler.suspend(prev); // implicitly choose() if suspending chosen()

        Thread * next = _scheduler.chosen();

        dispatch(prev, next);
    }

    unlock();

    return *reinterpret_cast<int *>(_stack);
}


void Thread::pass()
{
    lock();
    db<Thread>(TRC) << "Thread::pass(this=" << this << ")" << endl;

    Thread * prev = running();
    Thread * next = _scheduler.choose(this);

    if(next)
        dispatch(prev, next, false);
    else
        db<Thread>(WRN) << "Thread::pass => thread (" << this << ") not ready!" << endl;

    unlock();
}


void Thread::suspend()
{
    lock();
    db<Thread>(TRC) << "Thread::suspend(this=" << this << ")" << endl;

    Thread * prev = running();

    _state = SUSPENDED;
    _scheduler.suspend(this);

    Thread * next = _scheduler.chosen();

    dispatch(prev, next);

    unlock();
}


void Thread::resume()
{
    lock();
    db<Thread>(TRC) << "Thread::resume(this=" << this << ")" << endl;

    if(_state == SUSPENDED) {
        _state = READY;
        _scheduler.resume(this);

        if (preemptive) {
            if (multicore && Criterion::global) {
                reschedule_all_cpus();
            } else {
                reschedule(criterion().queue());
            }
        }  

    } else
        db<Thread>(WRN) << "Resume called for unsuspended object!" << endl;

    unlock();
}


void Thread::yield()
{
    lock();
    db<Thread>(TRC) << "Thread::yield(running=" << running() << ")" << endl;

    Thread * prev = running();
    Thread * next = _scheduler.choose_another();

    dispatch(prev, next);

    unlock();
}


void Thread::exit(int status)
{
    lock();

    db<Thread>(TRC) << "Thread::exit(status=" << status << ") [running=" << running() << "]" << endl;

    Thread * prev = running();
    _scheduler.remove(prev);
    prev->_state = FINISHING;
    *reinterpret_cast<int *>(prev->_stack) = status;

    _thread_count--;

    if(prev->_joining) {
        prev->_joining->_state = READY;
        _scheduler.resume(prev->_joining);
        prev->_joining = 0;
    }

    Thread * next = _scheduler.choose(); // at least idle will always be there

    dispatch(prev, next);

    unlock();
}


void Thread::sleep(Queue * q)
{
    db<Thread>(TRC) << "Thread::sleep(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    Thread * prev = running();
    _scheduler.suspend(prev);
    prev->_state = WAITING;
    prev->_waiting = q;
    q->insert(&prev->_link);

    Thread * next = _scheduler.chosen();

    dispatch(prev, next);
}


void Thread::wakeup(Queue * q)
{
    db<Thread>(TRC) << "Thread::wakeup(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    if(!q->empty()) {
        Thread * t = q->remove()->object();
        t->_state = READY;
        t->_waiting = 0;
        _scheduler.resume(t);

        if (preemptive) {
            if (multicore && Criterion::global) {
                reschedule_all_cpus();
            } else {
                reschedule(t->criterion().queue());
            }
        }
    }
}


void Thread::wakeup_all(Queue * q)
{
    db<Thread>(TRC) << "Thread::wakeup_all(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    bool reschedule_cpu[CPU::cores()];
    for(unsigned int i = 0; i < CPU::cores(); i++) {
        reschedule_cpu[i] = false;
    }

    if(!q->empty()) {
        while(!q->empty()) {
            Thread * t = q->remove()->object();
            t->_state = READY;
            t->_waiting = 0;
            _scheduler.resume(t);

            reschedule_cpu[t->criterion().queue()] = true;
        }

        if (preemptive) {
            if (multicore && Criterion::global) {
                reschedule_all_cpus();
                return;
            } 

            if (multicore && Criterion::partitioned) {
                for(unsigned int i = 0; i < CPU::cores(); i++) {
                    if (reschedule_cpu[i] && i != CPU::id()) {
                        reschedule(i);
                    }
                }
                if (reschedule_cpu[CPU::id()]) {
                    reschedule();
                }
                return; 
            } 
            
            if (!multicore) {
                reschedule();
            }
        }
    }
}

void Thread::reschedule(unsigned int cpu_id)
{
    if(!Criterion::timed || Traits<Thread>::hysterically_debugged)
        db<Thread>(TRC) << "Thread::reschedule(int cpu_id)" << endl;

    assert(locked()); // locking handled by caller

    if (!multicore || (cpu_id == CPU::id())) {
        reschedule();
    } else {
        db<Thread>(TRC) << "INT_RESCHEDULER sent from " << CPU::id() << " to " << cpu_id << endl;
        IC::ipi(cpu_id, IC::INT_RESCHEDULER);
    }
}

void Thread::reschedule()
{
    if(!Criterion::timed || Traits<Thread>::hysterically_debugged)
        db<Thread>(TRC) << "Thread::reschedule()" << endl;

    assert(locked()); // locking handled by caller

    Thread * prev = running();
    Thread * next = _scheduler.choose();
    dispatch(prev, next);
}

void Thread::reschedule_all_cpus(const bool reschedule_after_ipi)
{
    if(!Criterion::timed || Traits<Thread>::hysterically_debugged)
        db<Thread>(TRC) << "Thread::reschedule_all_cpus()" << endl;

    assert(locked()); // locking handled by caller

    for(unsigned int i = 0; i < Traits<Machine>::CPUS; i++) {
        if (i != CPU::id())
            reschedule(i);
    }
    if (reschedule_after_ipi)
        reschedule();
}

void Thread::rescheduler(IC::Interrupt_Id i)
{
    lock();
    db<Thread>(TRC) << "Thread::rescheduler" << endl;
    reschedule();
    unlock();
}


void Thread::time_slicer(IC::Interrupt_Id i)
{
    lock();
    db<Thread>(TRC) << "Thread::time_slicer" << endl;
    reschedule();
    unlock();
}


void Thread::dispatch(Thread * prev, Thread * next, bool charge)
{
    // "next" is not in the scheduler's queue anymore. It's already "chosen"

    if(charge) {
        if(Criterion::timed)
            _timer->restart();
    }

    if(prev != next) {
        if(prev->_state == RUNNING)
            prev->_state = READY;
        next->_state = RUNNING;

        if (Criterion::dynamic) {
            prev->update(Criterion::LEAVE);
            next->update(Criterion::SCHEDULED);
        }

        db<Thread>(TRC) << "Thread::dispatch(prev=" << prev << ",next=" << next << ")" << endl;
        if(Traits<Thread>::debugged && Traits<Debug>::info) {
            CPU::Context tmp;
            tmp.save();
            db<Thread>(INF) << "Thread::dispatch:prev={" << prev << ",ctx=" << tmp << "}" << endl;
        }
        db<Thread>(INF) << "Thread::dispatch:next={" << next << ",ctx=" << *next->_context << "}" << endl;

        // assert(_lock.level() == 1);
        if (multicore) {
            db<Thread>(TRC) << "locked released at dispatch" << endl;
            _lock.release();
        }

        db<Thread>(INF) << "BEFORE SWITCH CONTEXT" << endl;
        db<Thread>(INF) << "SP=" << CPU::sp() << endl;
        // The non-volatile pointer to volatile pointer to a non-volatile context is correct
        // and necessary because of context switches, but here, we are locked() and
        // passing the volatile to switch_constext forces it to push prev onto the stack,
        // disrupting the context (it doesn't make a difference for Intel, which already saves
        // parameters on the stack anyway).
        CPU::switch_context(const_cast<Context **>(&prev->_context), next->_context);
        db<Thread>(INF) << "AFTER SWITCH CONTEXT" << endl;
        db<Thread>(INF) << "SP=" << CPU::sp() << endl;

        if (multicore) {
            CPU::int_disable();
            _lock.acquire();
            db<Thread>(TRC) << "locked acquired at dispatch" << endl;
        }

    }
}

void Thread::update(Event event)
{
    switch(_state) {
    case RUNNING:
        criterion().update(event);
        break;
    case READY:
        _scheduler.remove(this);
        criterion().update(event);
        _scheduler.insert(this);
        break;
    case SUSPENDED:
        criterion().update(event);
        break;
    case WAITING:
        _waiting->remove(&_link);
        criterion().update(event);
        _waiting->insert(&_link);
        break;
    case FINISHING:
        criterion().update(event);
        break;
    }
}

int Thread::idle()
{
    db<Thread>(TRC) << "Thread::idle(this=" << running() << ")" << endl;

    while(_thread_count > CPU::cores()) { // someone else besides idles  
        if(Traits<Thread>::trace_idle)
            db<Thread>(TRC) << "Thread::idle(this=" << running() << ")" << endl;

        CPU::int_enable();
        CPU::halt();

        if(_scheduler.schedulables() > 0)
            yield();
    }

    CPU::int_disable();
    db<Thread>(WRN) << "The last thread has exited!" << endl;

    if (profiler && (CPU::id() == 0)) {
        double limit_interrupt_percent = 10;
        long current_timestamp = CLINT::mtime();
        double actual_interrupt_percent = (((double) (IC::interrupt_time)) / (current_timestamp - init_timestamp))*100;
        int frequency_limit = (int) (Traits<Timer>::FREQUENCY * (limit_interrupt_percent / actual_interrupt_percent));
        db<Thread>(INF) << "Time interreputions time="<< IC::interrupt_time << endl;
        db<Thread>(WRN) << "Frequency limit= " << frequency_limit << endl;
    }

    if (CPU::id() == 0) {
        db<Thread>(WRN) << "Rebooting the machine ..." << endl;
        Machine::reboot();
    }

    // Some machines will need a little time to actually reboot
    for(;;);

    return 0;
}

__END_SYS
