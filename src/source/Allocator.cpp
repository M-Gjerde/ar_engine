//
// Created by magnus on 9/20/20.
//

#include <malloc.h>
#include <stdexcept>
#include <iostream>
#include "Allocator.h"

void* Allocator::Allocation(
        size_t                                      size,
        size_t                                      alignment,
        VkSystemAllocationScope                     allocationScope)
{
    return memalign(alignment, size);
}

void* VKAPI_CALL Allocator::Allocation(
        void*                                       pUserData,
        size_t                                      size,
        size_t                                      alignment,
        VkSystemAllocationScope                     allocationScope)
{

    //printf("Allocating size: %zu, alignment: %zu, allocationScope: %i\n", size, alignment, allocationScope);

    return static_cast<Allocator*>(pUserData)->Allocation(size,
                                                          alignment,
                                                          allocationScope);
}

void* Allocator::Reallocation(
        void*                                       pOriginal,
        size_t                                      size,
        size_t                                      alignment,
        VkSystemAllocationScope                     allocationScope)
{
    realloc(pOriginal, size);
    //throw std::runtime_error("Attempted to reallocate\n");
}

void* VKAPI_CALL Allocator::Reallocation(
        void*                                       pUserData,
        void*                                       pOriginal,
        size_t                                      size,
        size_t                                      alignment,
        VkSystemAllocationScope                     allocationScope)
{
    return static_cast<Allocator*>(pUserData)->Reallocation(pOriginal,
                                                            size,
                                                            alignment,
                                                            allocationScope);
}

void Allocator::Free(
        void*                                       pMemory)
{

    free(pMemory);
}

void VKAPI_CALL Allocator::Free(
        void*                                       pUserData,
        void*                                       pMemory)
{
    //std::cout << typeid(pUserData).name()<< std::endl;

    return static_cast<Allocator*>(pUserData)->Free(pMemory);
}

void Allocator::InternalFree(
        void *pUserData,
        size_t size,
        VkInternalAllocationType allocationType,
        VkSystemAllocationScope allocationScope) {

    std::cout << "Freed memory: " << size << std::endl;

}

void Allocator::InternalAllocation(void *pUserData, size_t size, VkInternalAllocationType allocationType,
                                   VkSystemAllocationScope allocationScope) {

    std::cout << "More alloc: " << size << std::endl;

}

Allocator::~Allocator() = default;

Allocator::Allocator() = default;
