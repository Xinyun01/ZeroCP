#ifndef ZEROCP_RELATIVE_POINTER_HPP
#define ZEROCP_RELATIVE_POINTER_HPP

#include <cstdint>
#include <unordered_map>
#include <mutex>

namespace ZeroCP
{

// 段ID类型定义
using segment_id_t = std::uint64_t;

// 共享内存段注册表：存储每个segment_id对应的基地址
class SegmentRegistry
{
public:
    static SegmentRegistry& instance() noexcept;
    
    void registerSegment(segment_id_t id, void* baseAddress) noexcept;
    void unregisterSegment(segment_id_t id) noexcept;
    void* getBaseAddress(segment_id_t id) const noexcept;
    
private:
    SegmentRegistry() = default;
    mutable std::mutex m_mutex;
    std::unordered_map<segment_id_t, void*> m_segments;
};

template<typename T>
class RelativePointer
{
public:
        using ptr_t = T*;
        using offset_t = std::uint64_t;

        RelativePointer() noexcept = default;
        ~RelativePointer() noexcept = default;

        RelativePointer(ptr_t const ptr, uint64_t segment_id) noexcept;

        RelativePointer(const offset_t offset, const uint64_t segment_id) noexcept;
        
        RelativePointer(const RelativePointer& other) noexcept = default;
        RelativePointer(RelativePointer&& other) noexcept;
        RelativePointer& operator=(const RelativePointer& other) noexcept;
        // AXIVION Next Line AutosarC++19_03-A3.1.6 : False positive, this is not a trivial accessor/ mutator function but the move c'tor
        RelativePointer& operator=(RelativePointer&& other) noexcept;

        T* get() const noexcept;
        uint64_t get_segment_id() const noexcept;
        offset_t get_offset() const noexcept;
        
        /// @brief 计算ptr对base的偏移
        static offset_t getOffset(const segment_id_t id, ptr_t const ptr) noexcept;
        /// @brief 由id及offset获取原生指针
        static T* getPtr(const segment_id_t id, const offset_t offset) noexcept;
        
        /// @brief 解引用操作符
        T& operator*() const noexcept;
        /// @brief 成员访问操作符
        T* operator->() const noexcept;
        /// @brief 布尔转换操作符（检查是否为空）
        explicit operator bool() const noexcept;
        
private:
    segment_id_t m_segment_id{0};
    offset_t m_offset{0};
};

} // namespace ZeroCP

#include "zerocp_foundationLib/posix/memory/deital/relative_pointer.inl"

#endif