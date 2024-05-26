// EPOS Heap Utility Declarations

#ifndef __heap_h
#define __heap_h

#include <utility/debug.h>
#include <utility/list.h>
#include <utility/spin.h>

__BEGIN_UTIL

extern "C" {
    void _lock_heap();
    void _unlock_heap();
}

// Heap
class Heap: private Grouping_List<char>
{
protected:
    static const bool typed = Traits<System>::multiheap;

public:
    using Grouping_List<char>::empty;
    using Grouping_List<char>::size;
    using Grouping_List<char>::grouped_size;

    Heap() {
        db<Init, Heaps>(TRC) << "Heap() => " << this << endl;
    }

    Heap(void * addr, unsigned long bytes) {
        db<Init, Heaps>(TRC) << "Heap(addr=" << addr << ",bytes=" << bytes << ") => " << this << endl;

        free(addr, bytes);
    }

    void * alloc(unsigned long bytes) {
        lock();
        void* address = _alloc(bytes);
        unlock();
        return address;
    }

    void free(void * ptr, unsigned long bytes) {
        lock();
        db<Heaps>(TRC) << "Heap::free(this=" << this << ",ptr=" << ptr << ",bytes=" << bytes << ")" << endl;

        if(ptr && (bytes >= sizeof(Element))) {
            Element * e = new (ptr) Element(reinterpret_cast<char *>(ptr), bytes);
            Element * m1, * m2;
            insert_merging(e, &m1, &m2);
        }
        unlock();
    }

    static void typed_free(void * ptr) {
        lock();
        long * addr = reinterpret_cast<long *>(ptr);
        unsigned long bytes = *--addr;
        Heap * heap = reinterpret_cast<Heap *>(*--addr);
        heap->free(addr, bytes);
        unlock();
    }

    static void untyped_free(Heap * heap, void * ptr) {
        lock();
        long * addr = reinterpret_cast<long *>(ptr);
        unsigned long bytes = *--addr;
        heap->free(addr, bytes);
        unlock();
    }

private:
    static void lock() {  
        _lock_heap(); 
    }

    static void unlock() { 
        _unlock_heap(); 
    }

    void out_of_memory(unsigned long bytes);

    void * _alloc(unsigned long bytes) {
        db<Heaps>(TRC) << "Heap::alloc(this=" << this << ",bytes=" << bytes;

        if(!bytes)
            return 0;

        if(!Traits<CPU>::unaligned_memory_access)
            while((bytes % sizeof(void *)))
                ++bytes;

        if(typed)
            bytes += sizeof(void *);  // add room for heap pointer
        bytes += sizeof(long);        // add room for size
        if(bytes < sizeof(Element))
            bytes = sizeof(Element);

        Element * e = search_decrementing(bytes);
        if(!e) {
            out_of_memory(bytes);
            return 0;
        }

        long * addr = reinterpret_cast<long *>(e->object() + e->size());

        if(typed)
            *addr++ = reinterpret_cast<long>(this);
        *addr++ = bytes;

        db<Heaps>(TRC) << ") => " << reinterpret_cast<void *>(addr) << endl;

        return addr;
    }
};

__END_UTIL

#endif
