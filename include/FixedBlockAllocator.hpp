#pragma once

#include <cstddef>
#include <vector>
#include <utility>
#include <new>

template <typename T, size_t BlockSize = 4096>
class FixedBlockAllocator
{
private:
    union Node
    {
        Node* next;
        alignas(alignof(T)) char storage[sizeof(T)];
    };

    Node* freeList{nullptr};
    std::vector<void*> chunks;

    void allocateChunk()
    {
        Node* newChunk = static_cast<Node*>(::operator new[](BlockSize * sizeof(Node)));
        chunks.push_back(newChunk);

        for (size_t i = 0; i < BlockSize - 1; ++i)
        {
            newChunk[i].next = &newChunk[i + 1];
        }
        newChunk[BlockSize - 1].next = nullptr;
        freeList = newChunk;
    }

public:
    using value_type = T;

    FixedBlockAllocator() noexcept = default;

    template <typename U>
    FixedBlockAllocator(const FixedBlockAllocator<U, BlockSize>&) noexcept {}

    ~FixedBlockAllocator()
    {
        for (void* chunk : chunks)
        {
            ::operator delete[](chunk);
        }
    }

    T* allocate(size_t n)
    {
        if (n != 1)
        {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
        if (!freeList)
        {
            allocateChunk();
        }
        Node* node = freeList;
        freeList = freeList->next;
        return reinterpret_cast<T*>(node);
    }

    void deallocate(T* p, size_t n) noexcept
    {
        if (!p) return;
        if (n != 1)
        {
            ::operator delete(p);
            return;
        }
        Node* node = reinterpret_cast<Node*>(p);
        node->next = freeList;
        freeList = node;
    }

    template <typename U>
    struct rebind
    {
        using other = FixedBlockAllocator<U, BlockSize>;
    };

    bool operator==(const FixedBlockAllocator&) const noexcept { return true; }
    bool operator!=(const FixedBlockAllocator&) const noexcept { return false; }
};
