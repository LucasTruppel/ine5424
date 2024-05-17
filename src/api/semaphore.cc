// EPOS Semaphore Implementation

#include <synchronizer.h>

__BEGIN_SYS

Semaphore::Semaphore(long v) : Synchronizer_Common(v), _value(v)
{
    db<Synchronizer>(TRC) << "Semaphore(value=" << _value << ") => " << this << endl;
}


Semaphore::~Semaphore()
{
    db<Synchronizer>(TRC) << "~Semaphore(this=" << this << ")" << endl;
}


void Semaphore::p()
{
    db<Synchronizer>(TRC) << "Semaphore::p(this=" << this << ",value=" << _value << ")" << endl;

    begin_atomic();
    db<Synchronizer>(TRC) << "Semaphore::p lock" << endl;
    if(fdec(_value) < 1){
        apply_new_priority();
        sleep();
    }
    insert();
     db<Synchronizer>(TRC) << "Semaphore::p unlock" << endl;
    end_atomic();
    
}


void Semaphore::v()
{
    db<Synchronizer>(TRC) << "Semaphore::v(this=" << this << ",value=" << _value << ")" << endl;

    begin_atomic();
    db<Synchronizer>(TRC) << "Semaphore::v lock" << endl;
    restore_priority();
    if(finc(_value) < 0)
        wakeup();
    db<Synchronizer>(TRC) << "Semaphore::v unlock" << endl;
    end_atomic();

}

__END_SYS
