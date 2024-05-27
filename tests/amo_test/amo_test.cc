#include <utility/ostream.h>

using namespace EPOS;

OStream cout;

bool tsl_test(int test_id, int lock) {
    cout << "\nCPU::tsl test " << test_id << endl;

    bool tsl_success = true;
    int previous_lock = lock;

    cout << "lock=" << lock << endl;
    cout << "int tsl_return = CPU::tsl(lock);" << endl;

    int tsl_return = CPU::tsl(lock);
    cout << "expected: tsl_return=" << previous_lock << " lock=1" << endl;
    if ((lock != 1) || (tsl_return != previous_lock)) {
        tsl_success = false;
    }   

    cout << "obtained: tsl_return=" << tsl_return << " lock=" << lock << endl;
    cout << "CPU::tsl test " << test_id << (tsl_success ? " passed" : " failed") << endl;

    return tsl_success;
}

bool finc_test(int test_id, int value) {
    cout << "\nCPU::finc test " << test_id << endl;

    bool finc_success = true;
    int previous_value = value;

    cout << "value=" << value << endl;
    cout << "int finc_return = CPU::finc(value);" << endl;

    int finc_return = CPU::finc(value);
    cout << "expected: finc_return=" << previous_value << " value=" << previous_value + 1 << endl;
    if ((value != previous_value + 1) || (finc_return != previous_value)) {
        finc_success = false;
    }

    cout << "obtained: finc_return=" << finc_return << " value=" << value << endl;
    cout << "CPU::finc test " << test_id << (finc_success ? " passed" : " failed") << endl;

    return finc_success;
}

bool fdec_test(int test_id, int value) {
    cout << "\nCPU::fdec test " << test_id << endl;

    bool fdec_success = true;
    int previous_value = value;

    cout << "value=" << value << endl;
    cout << "int fdec_return = CPU::fdec(value);" << endl;

    int fdec_return = CPU::fdec(value);
    cout << "expected: fdec_return=" << previous_value << " value=" << previous_value - 1 << endl;
    if ((value != previous_value - 1) || (fdec_return != previous_value)) {
        fdec_success = false;
    }

    cout << "obtained: fdec_return=" << fdec_return << " value=" << value << endl;
    cout << "CPU::fdec test " << test_id << (fdec_success ? " passed" : " failed") << endl;

    return fdec_success;
}

bool cas_test(int test_id, int value, int compare_value, int replacement) {
    cout << "\nCPU::cas test " << test_id << endl;

    bool cas_success = true;
    int previous_value = value;

    cout << "value=" << value << " compare_value=" << compare_value << " replacement=" << replacement << endl;
    cout << "int cas_return = CPU::cas(value, compare_value, replacement);" << endl;

    int cas_return = CPU::cas(value, compare_value, replacement);
    if (previous_value == compare_value) {
        cout << "expected: cas_return=" << previous_value << " value=" << replacement << endl;
        if ((value != replacement) || (cas_return != previous_value)) {
            cas_success = false; 
        }
    } else {
        cout << "expected: cas_return=" << previous_value << " value=" << previous_value << endl;
        if ((value != previous_value) || (cas_return != previous_value)) {
            cas_success = false;
        }
    }
    cout << "obtained: cas_return=" << cas_return << " value=" << value << endl;
    cout << "CPU::cas test " << test_id << (cas_success ? " passed" : " failed") << endl;

    return cas_success;
}

int main()
{
    cout << "AMO Operations Test" << endl;
    int tests_passed = 0;
    if (tsl_test(1, 0)) tests_passed++;
    if (tsl_test(2, 1)) tests_passed++;
    if (finc_test(1, 21)) tests_passed++;
    if (fdec_test(1, 13)) tests_passed++;
    if (cas_test(1, 0, 0, 10)) tests_passed++;
    if (cas_test(2, 10, 20, 30)) tests_passed++;
    cout << "\nAMO Operations Test " <<  tests_passed << "/6 tests passed\n" << endl;
    return 0;

}
