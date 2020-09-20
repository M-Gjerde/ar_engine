//
// Created by magnus on 9/20/20.
//

#ifndef AR_ENGINE_ALLOCATOR_H
#define AR_ENGINE_ALLOCATOR_H


#include <vulkan/vulkan.h>

class Allocator {
public:

    Allocator();
    virtual ~Allocator();

    // Operator that allows an instance of this class to be used as a
    // VkAllocationCallbacks structure
    inline explicit operator VkAllocationCallbacks() const
    {
        VkAllocationCallbacks result;

        result.pUserData = (void*)this;
        result.pfnAllocation = &Allocation;
        result.pfnReallocation = &Reallocation;
        result.pfnFree = &Free;
        result.pfnInternalAllocation = &InternalAllocation;
        result.pfnInternalFree = &InternalFree;

        return result;
    };

private:

    int totalAllocated;

    // Declare the allocator callbacks as static member functions.
    static void* VKAPI_CALL Allocation(
            void*                                       pUserData,
            size_t                                      size,
            size_t                                      alignment,
            VkSystemAllocationScope                     allocationScope);

    static void* VKAPI_CALL Reallocation(
            void*                                       pUserData,
            void*                                       pOriginal,
            size_t                                      size,
            size_t                                      alignment,
            VkSystemAllocationScope                     allocationScope);

    static void VKAPI_CALL Free(
            void*                                       pUserData,
            void*                                       pMemory);

    // Now declare the nonstatic member functions that will actually perform
    // the allocations.
    void* Allocation(
            size_t                                      size,
            size_t                                      alignment,
            VkSystemAllocationScope                     allocationScope);

    void* Reallocation(
            void*                                      pOriginal,
            size_t                                     size,
            size_t                                     alignment,
            VkSystemAllocationScope                    allocationScope);

    void Free(
            void*                                       pMemory);

    static void VKAPI_CALL InternalFree(
            void*                                       pUserData,
            size_t                                      size,
            VkInternalAllocationType                    allocationType,
            VkSystemAllocationScope                     allocationScope);

    static void VKAPI_CALL InternalAllocation(
            void*                                       pUserData,
            size_t                                      size,
            VkInternalAllocationType                    allocationType,
            VkSystemAllocationScope                     allocationScope
            );
};


#endif //AR_ENGINE_ALLOCATOR_H
