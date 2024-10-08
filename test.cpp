#include "allocator.hpp"
#include <iostream>

const unsigned int max_size = 64;
const unsigned int chunks_per_block = 4;
const unsigned int n_obj = 5;
const unsigned int n_iter = 1;

class A
{  
public:
    A() { std::cout << "A construct at " << this << "\n"; }
    ~A() { std::cout << "A destructor\n"; }

    void* operator new(std::size_t n)
    {
        return pool.allocate(n);
    }

    void operator delete(void* p)
    {
        pool.deallocate(p);
    }

private:
    // using Pool = ObjectPool<A, MallocAllocator<A>>;
    // using Pool = ObjectPool<A, ArrayAllocator<A, max_size>>;
    // using Pool = ObjectPool<A, HeapAllocator<A, max_size>>;
    // using Pool = ObjectPool<A, StackAllocator<A, max_size>>;
    using Pool = ObjectPool<A, BlockAllocator<A, chunks_per_block>>;
    static Pool pool;
    int a;
    int b;
};


A::Pool A::pool;

void testA()
{
    for (int i = 0; i < n_iter; ++i)
    {
        A* a[n_obj];
        for (int j = 0; j < n_obj; ++j)
        {
            a[j] = new A;
        }
        for (int j = 0; j < n_obj; ++j)
        {
            delete a[j];
        }
    }
}


int main()
{
    testA();
}