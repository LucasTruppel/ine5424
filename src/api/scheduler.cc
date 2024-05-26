// EPOS CPU Scheduler Component Implementation

#include <process.h>
#include <time.h>

__BEGIN_SYS

Simple_Spin Scheduling_Criterion_Common::_lock;

// The following Scheduling Criteria depend on Alarm, which is not available at scheduler.h
template <typename ... Tn>
FCFS::FCFS(int p, Tn & ... an): Priority((p == IDLE) ? IDLE : Alarm::elapsed()) {}

EDF::EDF(const Microsecond & d, const Microsecond & p, const Microsecond & c, unsigned int): Real_Time_Scheduler_Common(Alarm::ticks(d), Alarm::ticks(d), p, c) {}

void EDF::update(Event event) {
    if (event & PERIOD_START) {
        if (_priority == CEILING) {
            if ((_frozen_priority >= PERIODIC) && (_frozen_priority < APERIODIC))
                _frozen_priority = Alarm::elapsed() + _deadline;
        } else if((_priority >= PERIODIC) && (_priority < APERIODIC))
            _priority = Alarm::elapsed() + _deadline;
    }
}

LLF::LLF(const Microsecond & d, const Microsecond & wcet, const Microsecond & p, const Microsecond & c, unsigned int): 
    Real_Time_Scheduler_Common(Alarm::ticks(d) - Alarm::ticks(wcet), Alarm::ticks(d), p, c),
    _wcet(Alarm::ticks(wcet)) {}

void LLF::update(Event event) {
    if (event & PERIOD_START) {
        db<Thread>(TRC) << "###" << endl;

        if (protocol_applied() && is_periodic(_frozen_priority))
            _frozen_priority = Alarm::elapsed() + _deadline - _wcet;
        else if(!protocol_applied() && is_periodic(_priority))
            _priority = Alarm::elapsed() + _deadline - _wcet;

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

// Since the definition of FCFS above is only known to this unit, forcing its instantiation here so it gets emitted in scheduler.o for subsequent linking with other units is necessary.
template FCFS::FCFS<>(int p);

__END_SYS
