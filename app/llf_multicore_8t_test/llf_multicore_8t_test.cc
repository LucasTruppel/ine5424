// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>
#include <scheduler.h>

using namespace EPOS;

const unsigned int iterations = 100;
const unsigned int period_a = 100; // ms
const unsigned int period_b = 80; // ms
const unsigned int period_c = 60; // ms
const unsigned int wcet_a = 90; // ms
const unsigned int wcet_b = 70; // ms
const unsigned int wcet_c = 50; // ms

int func_a();
int func_b();
int func_c();
int func_d();
int func_e();
int func_f();
int func_g();
int func_h();
long max(unsigned int a, unsigned int b, unsigned int c) { return ((a >= b) && (a >= c)) ? a : ((b >= a) && (b >= c) ? b : c); }

OStream cout;
Chronometer chrono;
Periodic_Thread * thread_a;
Periodic_Thread * thread_b;
Periodic_Thread * thread_c;
Periodic_Thread * thread_d;
Periodic_Thread * thread_e;
Periodic_Thread * thread_f;
Periodic_Thread * thread_g;
Periodic_Thread * thread_h;


volatile bool all_created = false;

typedef Traits<Thread>::Criterion Criterion;

inline void exec(char c, unsigned int time = 0) // in miliseconds
{
    // Delay was not used here to prevent scheduling interference due to blocking
    Microsecond elapsed = chrono.read() / 1000;

    cout << "\n" << elapsed << "\t" << c
         << "\t[p(A)=" << thread_a->priority()
         << ", p(B)=" << thread_b->priority()
         << ", p(C)=" << thread_c->priority()
         << ", p(D)=" << thread_d->priority()
         << ", p(E)=" << thread_e->priority()
         << ", p(F)=" << thread_f->priority()
         << ", p(G)=" << thread_g->priority()
         << ", p(H)=" << thread_h->priority()
         << "]";

    if(time) {
        for(Microsecond end = elapsed + time, last = end; end > elapsed; elapsed = chrono.read() / 1000)
            if(last != elapsed) {
                cout << "\n" << elapsed << "\t" << c
                    << "\t[p(A)=" << thread_a->priority()
                    << ", p(B)=" << thread_b->priority()
                    << ", p(C)=" << thread_c->priority()
                    << ", p(D)=" << thread_d->priority()
                    << ", p(E)=" << thread_e->priority()
                    << ", p(F)=" << thread_f->priority()
                    << ", p(G)=" << thread_g->priority()
                    << ", p(H)=" << thread_h->priority()
                    << "]";
                last = elapsed;
            }
    }
}


int main()
{
    cout << "LLF Periodic Thread Component Test" << endl;

    cout << "\nThis test consists in creating three periodic threads as follows:" << endl;
    cout << "- Every " << period_a << "ms, thread A execs \"a\", waits for " << wcet_a << "ms and then execs another \"a\";" << endl;
    cout << "- Every " << period_b << "ms, thread B execs \"b\", waits for " << wcet_b << "ms and then execs another \"b\";" << endl;
    cout << "- Every " << period_c << "ms, thread C execs \"c\", waits for " << wcet_c << "ms and then execs another \"c\";" << endl;

    cout << "Threads will now be created and I'll wait for them to finish..." << endl;

    // p,d,c,act,t
    thread_a = new Periodic_Thread(RTConf(period_a * 1000, 0, 0, 0, iterations, Thread::READY, Criterion(period_a * 1000, wcet_a * 1000)), &func_a);
    thread_b = new Periodic_Thread(RTConf(period_b * 1000, 0, 0, 0, iterations, Thread::READY, Criterion(period_b * 1000, wcet_b * 1000)), &func_b);
    thread_c = new Periodic_Thread(RTConf(period_c * 1000, 0, 0, 0, iterations, Thread::READY, Criterion(period_c * 1000, wcet_c * 1000)), &func_c);
    thread_d = new Periodic_Thread(RTConf(period_a * 1000, 0, 0, 0, iterations, Thread::READY, Criterion(period_a * 1000, wcet_a * 1000)), &func_d);
    thread_e = new Periodic_Thread(RTConf(period_b * 1000, 0, 0, 0, iterations, Thread::READY, Criterion(period_b * 1000, wcet_b * 1000)), &func_e);
    thread_f = new Periodic_Thread(RTConf(period_c * 1000, 0, 0, 0, iterations, Thread::READY, Criterion(period_c * 1000, wcet_c * 1000)), &func_f);
    thread_g = new Periodic_Thread(RTConf(period_a * 1000, 0, 0, 0, iterations, Thread::READY, Criterion(period_a * 1000, wcet_a * 1000)), &func_g);
    thread_h = new Periodic_Thread(RTConf(period_b * 1000, 0, 0, 0, iterations, Thread::READY, Criterion(period_b * 1000, wcet_b * 1000)), &func_h);

    exec('M');

    chrono.start();

    all_created = true;

    int status_a = thread_a->join();
    int status_b = thread_b->join();
    int status_c = thread_c->join();
    int status_d = thread_d->join();
    int status_e = thread_e->join();
    int status_f = thread_f->join();
    int status_g = thread_g->join();
    int status_h = thread_h->join();

    chrono.stop();

    exec('M');

    cout << "\n... done!" << endl;
    cout << "\n\nThread A exited with status \"" << char(status_a)
         << "\" \nThread B exited with status \"" << char(status_b)
         << "\" \nThread C exited with status \"" << char(status_c)
         << "\" \nThread D exited with status \"" << char(status_d)
         << "\" \nThread E exited with status \"" << char(status_e)
         << "\" \nThread F exited with status \"" << char(status_f)
         << "\" \nThread G exited with status \"" << char(status_g)
         << "\" \nThread H exited with status \"" << char(status_h)
         << "." << endl;

    cout << "\nThe estimated time to run the test was "
         << max(period_a, period_b, period_c) * iterations
         << " ms. The measured time was " << chrono.read() / 1000 <<" ms!" << endl;

    cout << "I'm also done, bye!" << endl;

    return 0;
}

int func_a()
{
    while (!all_created) {}

    exec('A');

    do {
        exec('a', wcet_a);
        cout << "Thread A is waiting for the next period" << endl;

    } while (Periodic_Thread::wait_next());

    exec('A');

    return 'A';
}

int func_b()
{
    while (!all_created) {}


    exec('B');

    do {
        exec('b', wcet_b);
        cout << "Thread B is waiting for the next period" << endl;
    } while (Periodic_Thread::wait_next());

    exec('B');

    return 'B';
}

int func_c()
{
    while (!all_created) {}

    exec('C');

    do {
        exec('c', wcet_c);
        cout << "Thread C is waiting for the next period" << endl;
    } while (Periodic_Thread::wait_next());

    exec('C');

    return 'C';
}

int func_d()
{
    while (!all_created) {}

    exec('D');

    do {
        exec('d', wcet_a);
        cout << "Thread D is waiting for the next period" << endl;
    } while (Periodic_Thread::wait_next());

    exec('D');

    return 'D';
}

int func_e()
{
    while (!all_created) {}

    exec('E');

    do {
        exec('e', wcet_b);
        cout << "Thread E is waiting for the next period" << endl;
    } while (Periodic_Thread::wait_next());

    exec('E');

    return 'E';
}

int func_f()
{
    while (!all_created) {}

    exec('F');

    do {
        exec('f', wcet_c);
        cout << "Thread F is waiting for the next period" << endl;
    } while (Periodic_Thread::wait_next());

    exec('F');

    return 'F';
}

int func_g()
{
    while (!all_created) {}

    exec('G');

    do {
        exec('g', wcet_a);
        cout << "Thread G is waiting for the next period" << endl;
    } while (Periodic_Thread::wait_next());

    exec('G');

    return 'G';
}

int func_h()
{
    while (!all_created) {}

    exec('H');

    do {
        exec('h', wcet_b);
        cout << "Thread H is waiting for the next period" << endl;
    } while (Periodic_Thread::wait_next());

    exec('H');

    return 'H';
}
