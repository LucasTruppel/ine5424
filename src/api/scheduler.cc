// EPOS CPU Scheduler Component Implementation

#include <process.h>
#include <time.h>

__BEGIN_SYS

// The following Scheduling Criteria depend on Alarm, which is not available at scheduler.h
template <typename ... Tn>
FCFS::FCFS(int p, Tn & ... an): Priority((p == IDLE) ? IDLE : Alarm::elapsed()) {}

EDF::EDF(const Microsecond & d, const Microsecond & p, const Microsecond & c, unsigned int): Real_Time_Scheduler_Common(Alarm::ticks(d), Alarm::ticks(d), p, c) {}

void EDF::update() {
    if (_priority == CEILING) {
        if ((_frozen_priority >= PERIODIC) && (_frozen_priority < APERIODIC))
            _frozen_priority = Alarm::elapsed() + _deadline;
    } else if((_priority >= PERIODIC) && (_priority < APERIODIC))
        _priority = Alarm::elapsed() + _deadline;
}

LLF::LLF(const Microsecond & d, const Microsecond & wcet, const Microsecond & p, const Microsecond & c, unsigned int): 
    Real_Time_Scheduler_Common(Alarm::ticks(d) - Alarm::ticks(wcet), Alarm::ticks(d), p, c),
    _wcet(Alarm::ticks(wcet)) {}

void LLF::update() {
    if (_priority == CEILING) {
        if ((_frozen_priority >= PERIODIC) && (_frozen_priority < APERIODIC))
            _frozen_priority = Alarm::elapsed() + _deadline - _wcet;
    } else if((_priority >= PERIODIC) && (_priority < APERIODIC))
        _priority = Alarm::elapsed() + _deadline - _wcet;
}

void LLF::update_on_reschedule(const Microsecond & exec_start) {
    if (_priority == CEILING) {
        if ((_frozen_priority >= PERIODIC) && (_frozen_priority < APERIODIC))
            _frozen_priority += Alarm::elapsed() - exec_start;
    } else if((_priority >= PERIODIC) && (_priority < APERIODIC))
        _priority += Alarm::elapsed() - exec_start;
}

// Since the definition of FCFS above is only known to this unit, forcing its instantiation here so it gets emitted in scheduler.o for subsequent linking with other units is necessary.
template FCFS::FCFS<>(int p);

__END_SYS
