#ifndef _ALLOCATOR_HPP_
#define _ALLOCATOR_HPP_

#include <iostream>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <array>

template <typename T>
class Allocator
{
public:
    virtual ~Allocator() = default;
    virtual T* allocate() = 0;
    virtual void deallocate(T*) = 0;
};

template<typename T, typename Allocator>
class ObjectPool
{
public:
    ObjectPool() = default;
    ~ObjectPool() = default;

    void* allocate(std::size_t n)
    {
        if (sizeof(T) != n)
            throw std::bad_alloc();
        return m_allocator.allocate();
    }

    void deallocate(void* p)
    {
        m_allocator.deallocate(static_cast<T*>(p));
    }

private:
    Allocator m_allocator;
};

template<typename T>
class MallocAllocator : public Allocator<T>
{
public:
    MallocAllocator() = default;
    ~MallocAllocator() = default;

    T* allocate() override
    {
        std::cout << "malloc allocator\n";
        return reinterpret_cast<T*>(std::malloc(sizeof(T)));
    }

    void deallocate(T* p) override
    {
        std::cout << "malloc deallocator\n";
        std::free(p);
    }
};

template<typename T, std::size_t N>
class ArrayAllocator : public Allocator<T>
{
public:
    ArrayAllocator()
    {
        for (int i = 0; i < N; ++i)
        {
            m_used[i] = false;
        }
    }

    ~ArrayAllocator() = default;

    T* allocate() override
    {
        std::cout << "array allocator\n";
        for (int i = 0; i < N; ++i)
        {
            if (!m_used[i])
            {
                m_used[i] = true;
                return reinterpret_cast<T*>(&m_data[sizeof(T) * i]);
            }
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p) override
    {
        std::cout << "array deallocator\n";
        auto i = ((unsigned char*)p - m_data) / sizeof(T);
        m_used[i] = false;
    }

private:
    unsigned char m_data[sizeof(T) * N];
    bool m_used[N];
};

template<typename T, std::size_t N>
class HeapAllocator : public Allocator<T>
{
public:
    enum State
    {
        FREE = 1,
        USED = 0
    };

    struct Entry
    {
        State state;
        T* p;

        bool operator < (const Entry& other) const 
        {
            return state < other.state;
        }
    };

    HeapAllocator()
    {
        m_available = N;
        for (int i = 0; i < N; ++i)
        {
            m_entry[i].state = FREE;
            m_entry[i].p = reinterpret_cast<T*>(&m_data[sizeof(T) * i]);
        }

        std::make_heap(m_entry, m_entry + N);
    }

    ~HeapAllocator() = default;

    T* allocate() override
    {
        std::cout << "heap allocator\n";
        if (m_available <= 0)
        {
            throw std::bad_alloc();
        }
        Entry e = m_entry[0];
        std::pop_heap(m_entry, m_entry + N);

        m_available--;
        m_entry[m_available].state = USED;
        m_entry[m_available].p = nullptr;

        return e.p;
    }

    void deallocate(T* p) override
    {
        std::cout << "heap deallocator\n";
        if (p == nullptr || m_available >=N)
            return;
        m_entry[m_available].state = FREE;
        m_entry[m_available].p = reinterpret_cast<T*>(p);
        m_available++;

        std::push_heap(m_entry, m_entry + N);
    }

private:
    unsigned char m_data[sizeof(T) * N];
    Entry m_entry[N];
    int m_available;
};

template<typename T, std::size_t N>
class StackAllocator : public Allocator<T>
{
public:
    StackAllocator() : m_allocated(0), m_available(0) {}
    ~StackAllocator() = default;

    T* allocate() override
    {

        std::cout << "stack allocator\n";
        if (m_available <= 0 && m_allocated >= N)
            throw std::bad_alloc();
        
        if (m_available > 0)
        {
            return m_stack[--m_available];
        }
        else
        {
            auto p = m_data + sizeof(T) * m_allocated;
            m_allocated++;
            return reinterpret_cast<T*> (p);
        }    
    }

    void deallocate(T* p) override
    {
        std::cout << "stack deallocator\n";
        m_stack[m_available++] = p;
    }

private:
    unsigned char m_data[sizeof(T) * N];
    std::array<T*, N> m_stack;
    int m_allocated;
    int m_available;
};

template <typename T, std::size_t ChunksPerBlock>
class BlockAllocator : public Allocator<T>
{
public:
    BlockAllocator() : m_head(nullptr) {}
    ~BlockAllocator() = default;

    T* allocate() override
    {
        std::cout << "block allocator\n";
        if (m_head == nullptr)
        {
            if (sizeof(T) < sizeof(T*))
            {
                std::cerr << "object size less than pointer size\n";
                throw std::bad_alloc();
            }
            m_head = allocate_block(sizeof(T));
        }
        Chuck* p = m_head;
        m_head = m_head->next;
        return reinterpret_cast<T*>(p);
    }

    void deallocate(T* p) override
    {
        std::cout << "block deallocator\n";
        auto chunk = reinterpret_cast<Chuck*>(p);
        chunk->next = m_head;
        m_head = chunk;
    }

private:
    struct Chunk
    {
        Chunk* next;
    };

    Chuck* allocate_block(std::size_t chunk_size)
    {
        std::size_t block_size = chunk_size * ChunksPerBlock;
        Chuck* head = reinterpret_cast<Chuck*>(std::malloc(block_size));
        Chuck* chunk = head;
        for (int i = 0; i < ChunksPerBlock; ++i)
        {
            chunk->next = reinterpret_cast<Chuck*>(reinterpret_cast<unsigned char*>(chunk) + chunk_size);
            chunk = chunk->next;
        }
        chunk->next = nullptr;
        return head;
    }
    
    Chunk* m_head;
};



#endif