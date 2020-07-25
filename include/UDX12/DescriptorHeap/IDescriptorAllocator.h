// ref
// 1. http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/
// 2. https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngineD3D12/include/DescriptorHeap.hpp

#pragma once

#include "../Util.h"

namespace Ubpa::UDX12 {
    class DescriptorHeapAllocation;
    class DescriptorHeapAllocMngr;
    class RenderpDevice;

    class IDescriptorAllocator {
    public:
        // Allocate Count descriptors
        virtual DescriptorHeapAllocation Allocate(uint32_t Count) = 0;
        virtual void Free(DescriptorHeapAllocation&& Allocation) = 0;
        virtual uint32_t GetDescriptorSize() const = 0;
    };
}
