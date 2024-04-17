// EPOS Ceiling Test Program

#include <time.h>
#include <real-time.h>
#include <scheduler.h>

using namespace EPOS;

int func_a();
int func_b();
int func_c();

OStream cout;
Chronometer chrono;
Thread * thread_a;
Thread * thread_b;
Thread * thread_c;
Semaphore* semaphore;
bool end = false;

typedef Traits<Thread>::Criterion Criterion;
typedef Thread::Configuration Configuration;

inline void wait(Thread* thread, char c, unsigned int time) // in miliseconds
{
    Microsecond elapsed = chrono.read() / 1000;
    if(time) {
        for(Microsecond end = elapsed + time, last = elapsed; end > elapsed; elapsed = chrono.read() / 1000) {
            if (elapsed - last >= time / 5) {
                cout << "\n" << "waiting: " << c
                    << "\t[p(A)=" << thread_a->priority()
                    << ", p(B)=" << thread_b->priority()
                    << ", p(C)=" << thread_c->priority() << "]";
                last = elapsed;
                thread->yield();
            }
        }
    }
}

inline void exec_a() 
{
    cout << "\n" << "a"
        << "\t[p(A)=" << thread_a->priority()
        << ", p(B)=" << thread_b->priority()
        << ", p(C)=" << thread_c->priority() << "]";
    while (thread_a->priority() != Criterion::CEILING && !end) {
       wait(thread_a, 'a', 1000);
    }
    cout << "\n" << "done waiting " << "a";
    cout << "\n" << "a"
    << "\t[p(A)=" << thread_a->priority()
    << ", p(B)=" << thread_b->priority()
    << ", p(C)=" << thread_c->priority() << "]";
}

inline void exec_b() 
{
    cout << "\n" << "b"
        << "\t[p(A)=" << thread_a->priority()
        << ", p(B)=" << thread_b->priority()
        << ", p(C)=" << thread_c->priority() << "]";
    while (thread_b->priority() != Criterion::CEILING && !end) {
       wait(thread_b, 'b', 1000);
    }
    cout << "\n" << "done waiting " << "b";
    cout << "\n" << "b"
    << "\t[p(A)=" << thread_a->priority()
    << ", p(B)=" << thread_b->priority()
    << ", p(C)=" << thread_c->priority() << "]";
}

inline void exec_c() 
{
    cout << "\n" << "c"
        << "\t[p(A)=" << thread_a->priority()
        << ", p(B)=" << thread_b->priority()
        << ", p(C)=" << thread_c->priority() << "]";
    cout << "\n" << "finally! " << "c";
    end = true;
}

int main() {

    semaphore = new Semaphore(2);

    thread_a = new Thread(Configuration(Thread::READY, Thread::NORMAL), &func_a);
    thread_b = new Thread(Configuration(Thread::READY, Thread::NORMAL), &func_b);
    thread_c = new Thread(Configuration(Thread::READY, Thread::HIGH), &func_c);

    cout << "\n\n" << "inicio: "
    << "\t[p(A)=" << thread_a->priority()
    << ", p(B)=" << thread_b->priority()
    << ", p(C)=" << thread_c->priority() << "]";

    chrono.start();

    int status_a = thread_a->join();
    int status_b = thread_b->join();
    int status_c = thread_c->join();

    chrono.stop();

    cout << "\n\n" << "final: "
    << "\t[p(A)=" << thread_a->priority()
    << ", p(B)=" << thread_b->priority()
    << ", p(C)=" << thread_c->priority() << "]";

    cout << "\n... done!" << endl;
    cout << "\n\nThread A exited with status \"" << char(status_a)
         << "\", thread B exited with status \"" << char(status_b)
         << "\" and thread C exited with status \"" << char(status_c) << "." << endl;
    cout << "I'm also done, bye!" << endl;

    delete semaphore;

    return 0;
}

int func_a()
{
    semaphore->p();
    exec_a();
    semaphore->v();
    return 'A';
}

int func_b()
{
    semaphore->p();
    exec_b();
    semaphore->v();
    return 'B';
}

int func_c()
{
    wait(thread_c, 'c', 3000);
    semaphore->p();
    exec_c();
    semaphore->v();
    return 'C';
}
