// EPOS RISC-V Timer Mediator Initialization

#include <architecture/cpu.h>
#include <machine/timer.h>
#include <machine/ic.h>

__BEGIN_SYS

void Timer::init()
{
    db<Init, Timer>(TRC) << "Timer::init()" << endl;
    
    // See sifive_u_traits.h and Thread::idle().
    db<Timer>(WRN) << "Run your application as profiler to find the frequency limit you must respect when executing." << endl;
    
    assert(CPU::int_disabled());

    IC::int_vector(IC::INT_SYS_TIMER, int_handler);

    reset();
    IC::enable(IC::INT_SYS_TIMER);
}

__END_SYS
