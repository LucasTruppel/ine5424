// EPOS CPU Scheduler Component Implementation

#include <process.h>
#include <time.h>

__BEGIN_SYS

Simple_Spin Scheduling_Criterion_Common::_lock;

Real_Time_Scheduler_Common::Tick Real_Time_Scheduler_Common::ticks(Microsecond time) {
    return Timer_Common::ticks(time, Alarm::frequency());
}

Microsecond Real_Time_Scheduler_Common::time(Tick ticks) {
    return Timer_Common::time(ticks, Alarm::frequency());
}

// The following Scheduling Criteria depend on Alarm, which is not available at scheduler.h
template <typename ... Tn>
FCFS::FCFS(int p, Tn & ... an): Priority((p == IDLE) ? IDLE : Alarm::elapsed()) {}

// Since the definition of FCFS above is only known to this unit, forcing its instantiation here so it gets emitted in scheduler.o for subsequent linking with other units is necessary.
template FCFS::FCFS<>(int p);

EDF::EDF(const Microsecond & d, const Microsecond & p, const Microsecond & c, unsigned int)
: Real_Time_Scheduler_Common(Alarm::ticks(d), d, p, c) {}

void EDF::update(Event event) {
    if (event & PERIOD_START) {
        if (_priority == CEILING) {
            if ((_frozen_priority >= PERIODIC) && (_frozen_priority < APERIODIC))
                _frozen_priority = Alarm::elapsed() + _deadline;
        } else if((_priority >= PERIODIC) && (_priority < APERIODIC))
            _priority = Alarm::elapsed() + _deadline;
    }
}

LLF::LLF(const Microsecond & d, const Microsecond & p, const Microsecond & c): 
    Real_Time_Scheduler_Common(Alarm::elapsed() + Alarm::ticks((d ? d : p) - c), d, p, c) {}

void LLF::update(Event event) {
    if (event & PERIOD_START) {
        if (protocol_applied() && is_periodic(_frozen_priority))
            _frozen_priority = Alarm::elapsed() + _deadline - _capacity;
        else if(!protocol_applied() && is_periodic(_priority))
            _priority = Alarm::elapsed() + _deadline - _capacity;

    } else if (event & LEAVE) {
        if (protocol_applied() && is_periodic(_frozen_priority)) 
            _frozen_priority += Alarm::elapsed() - _exec_start;
        else if(!protocol_applied() && is_periodic(_priority))
            _priority += Alarm::elapsed() - _exec_start;

    } else if (event & SCHEDULED) {
        _exec_start = Alarm::elapsed();
        
    }
}

volatile unsigned int PLLF::_next_cpu = 0;


__END_SYS
