#ifndef ZEROCP_RELATIVE_POINTER_INL
#define ZEROCP_RELATIVE_POINTER_INL

#include <cassert>
#include "zerocp_foundationLib/report/include/logging.hpp"

namespace ZeroCP
{

// ==================== SegmentRegistry 实现 ====================

inline SegmentRegistry& SegmentRegistry::instance() noexcept
{
    static SegmentRegistry registry;
    return registry;
}

inline void SegmentRegistry::registerSegment(segment_id_t id, void* baseAddress) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_segments[id] = baseAddress;
    
    ZEROCP_LOG(Debug, "Registered shared memory segment: ID=" << id 
                      << ", BaseAddress=" << baseAddress 
                      << ", TotalSegments=" << m_segments.size());
}

inline void SegmentRegistry::unregisterSegment(segment_id_t id) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_segments.find(id);
    
    if (it != m_segments.end())
    {
        ZEROCP_LOG(Debug, "Unregistering shared memory segment: ID=" << id 
                          << ", BaseAddress=" << it->second);
        m_segments.erase(it);
        ZEROCP_LOG(Debug, "Segment unregistered. Remaining segments: " << m_segments.size());
    }
    else
    {
        ZEROCP_LOG(Warn, "Attempted to unregister non-existent segment: ID=" << id);
    }
}

inline void* SegmentRegistry::getBaseAddress(segment_id_t id) const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_segments.find(id);
    
    if (it != m_segments.end())
    {
        ZEROCP_LOG(Trace, "Retrieved base address for segment ID=" << id 
                          << ", BaseAddress=" << it->second);
        return it->second;
    }
    else
    {
        ZEROCP_LOG(Error, "Segment not found in registry: ID=" << id 
                          << ". This may cause RelativePointer resolution to fail!");
        return nullptr;
    }
}

// ==================== RelativePointer 实现 ====================

template<typename T>
inline RelativePointer<T>::RelativePointer(ptr_t const ptr, uint64_t segment_id) noexcept
    : m_segment_id(segment_id)
    , m_offset(getOffset(segment_id, ptr))
{
}

template<typename T>
inline RelativePointer<T>::RelativePointer(const offset_t offset, const uint64_t segment_id) noexcept
    : m_segment_id(segment_id)
    , m_offset(offset)
{
}

template<typename T>
inline RelativePointer<T>::RelativePointer(RelativePointer&& other) noexcept
    : m_segment_id(other.m_segment_id)
    , m_offset(other.m_offset)
{
    // 移动后将源对象置为无效状态
    other.m_segment_id = 0;
    other.m_offset = 0;
}

template<typename T>
inline RelativePointer<T>& RelativePointer<T>::operator=(const RelativePointer& other) noexcept
{
    if (this != &other)
    {
        m_segment_id = other.m_segment_id;
        m_offset = other.m_offset;
    }
    return *this;
}

template<typename T>
inline RelativePointer<T>& RelativePointer<T>::operator=(RelativePointer&& other) noexcept
{
    if (this != &other)
    {
        m_segment_id = other.m_segment_id;
        m_offset = other.m_offset;
        
        // 移动后将源对象置为无效状态
        other.m_segment_id = 0;
        other.m_offset = 0;
    }
    return *this;
}

template<typename T>
inline T* RelativePointer<T>::get() const noexcept
{
    return getPtr(m_segment_id, m_offset);
}

template<typename T>
inline uint64_t RelativePointer<T>::get_segment_id() const noexcept
{
    return m_segment_id;
}

template<typename T>
inline typename RelativePointer<T>::offset_t RelativePointer<T>::get_offset() const noexcept
{
    return m_offset;
}

template<typename T>
inline typename RelativePointer<T>::offset_t RelativePointer<T>::getOffset(const segment_id_t id, ptr_t const ptr) noexcept
{
    if (ptr == nullptr)
    {
        ZEROCP_LOG(Trace, "Computing offset for nullptr, returning 0");
        return 0;
    }
    
    // 从注册表获取该段的基地址
    void* baseAddress = SegmentRegistry::instance().getBaseAddress(id);
    if (baseAddress == nullptr)
    {
        // 段未注册，返回0表示无效偏移
        ZEROCP_LOG(Error, "Cannot compute offset: Segment ID=" << id 
                          << " not found in registry. Ptr=" << ptr);
        return 0;
    }
    
    // 计算相对偏移量
    // ptr必须在baseAddress之后
    auto ptrValue = reinterpret_cast<uintptr_t>(ptr);
    auto baseValue = reinterpret_cast<uintptr_t>(baseAddress);
    
    if (ptrValue < baseValue)
    {
        // 指针不在段内，返回0
        ZEROCP_LOG(Error, "Invalid pointer: Ptr=" << ptr 
                          << " is before segment base=" << baseAddress 
                          << " (SegmentID=" << id << ")");
        return 0;
    }
    
    offset_t offset = static_cast<offset_t>(ptrValue - baseValue);
    ZEROCP_LOG(Trace, "Computed offset: SegmentID=" << id 
                      << ", Ptr=" << ptr 
                      << ", Base=" << baseAddress 
                      << ", Offset=" << offset);
    return offset;
}

template<typename T>
inline T* RelativePointer<T>::getPtr(const segment_id_t id, const offset_t offset) noexcept
{
    if (offset == 0)
    {
        ZEROCP_LOG(Trace, "Resolving RelativePointer with offset=0, returning nullptr");
        return nullptr;
    }
    
    // 从注册表获取该段的基地址
    void* baseAddress = SegmentRegistry::instance().getBaseAddress(id);
    if (baseAddress == nullptr)
    {
        // 段未注册，返回空指针
        ZEROCP_LOG(Error, "Cannot resolve RelativePointer: Segment ID=" << id 
                          << " not registered. Offset=" << offset 
                          << ". Returning nullptr.");
        return nullptr;
    }
    
    // 计算绝对地址：基地址 + 偏移量
    auto baseValue = reinterpret_cast<uintptr_t>(baseAddress);
    auto absoluteAddress = baseValue + offset;
    
    T* result = reinterpret_cast<T*>(absoluteAddress);
    ZEROCP_LOG(Trace, "Resolved RelativePointer: SegmentID=" << id 
                      << ", Offset=" << offset 
                      << ", Base=" << baseAddress 
                      << ", Result=" << result);
    return result;
}

template<typename T>
inline T& RelativePointer<T>::operator*() const noexcept
{
    T* ptr = get();
    assert(ptr != nullptr && "Dereferencing null RelativePointer");
    return *ptr;
}

template<typename T>
inline T* RelativePointer<T>::operator->() const noexcept
{
    T* ptr = get();
    assert(ptr != nullptr && "Dereferencing null RelativePointer");
    return ptr;
}

template<typename T>
inline RelativePointer<T>::operator bool() const noexcept
{
    return (m_offset != 0) && (get() != nullptr);
}

} // namespace ZeroCP

#endif // ZEROCP_RELATIVE_POINTER_INL

