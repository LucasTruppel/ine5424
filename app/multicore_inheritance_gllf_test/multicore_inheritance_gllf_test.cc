// EPOS Periodic Inheritance Test Program

#include <time.h>
#include <real-time.h>
#include <scheduler.h>

using namespace EPOS;

const unsigned int iterations = 10;
const unsigned int period_a = 100; // ms
const unsigned int period_c = 90; // ms
const unsigned int wcet_a = 50; // ms
const unsigned int wcet_c = 80; // ms

int func_a();
int func_b();
int func_c();

OStream cout;
Chronometer chrono;
Thread * thread_a;
Thread * thread_c;
Semaphore* semaphore;
Simple_Spin* spin;
volatile static bool resource_taken = false;
volatile static bool all_created = false;

typedef Traits<Thread>::Criterion Criterion;

int main() {

    semaphore = new Semaphore(1);
    spin = new Simple_Spin();

    thread_a = new Periodic_Thread(RTConf(period_a * 1000, period_a * 1000, wcet_a * 1000, 0, iterations), &func_a);
    thread_c = new Periodic_Thread(RTConf(period_c * 1000, period_c * 1000, wcet_c * 1000, 0, iterations), &func_c);

    all_created = true;

    int status_a = thread_a->join();
    int status_c = thread_c->join();

    cout << "\n... done!" << endl;
    cout << "\n\nThread A exited with status \"" << char(status_a)
         << "\" and thread C exited with status \"" << char(status_c) << "." << endl;
    cout << "I'm also done, bye!" << endl;

    delete semaphore;

    return 0;
}

int func_a()
{
    while (!all_created) {}
    spin->acquire();
    cout << "\n" << "A" << "\t[p(A)=" << thread_a->priority() << ", p(C)=" << thread_c->priority() << "]";
    spin->release();
    do {
        semaphore->p();
        resource_taken = true;
        while (!const_cast<Criterion &>(thread_a->priority()).protocol_applied()) {}
        cout << "\n" << "a" << "\t[p(A)=" << thread_a->priority() << ", p(C)=" << thread_c->priority() << "] Inheritance applied";
        semaphore->v();
        cout << "\n" << "a" << "\t[p(A)=" << thread_a->priority() << ", p(C)=" << thread_c->priority() << "] Restored priority";
        
    } while (Periodic_Thread::wait_next());
    return 'A';
}

int func_c()
{
    while (!all_created) {}
    spin->acquire();
    cout << "\n" << "C" << "\t[p(A)=" << thread_a->priority() << ", p(C)=" << thread_c->priority() << "]";
    spin->release();
    do {
        while (!resource_taken) {}
        semaphore->p();
        resource_taken = false;
        semaphore->v();
    } while (Periodic_Thread::wait_next());
    return 'C';
}
