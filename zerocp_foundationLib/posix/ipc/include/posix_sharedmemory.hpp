#ifndef ZeroCP_PosixSharedMemory_HPP
#define ZeroCP_PosixSharedMemory_HPP
#include "filesystemInterface.hpp"
#include <string>
#include <array>
#include <cstdint>
namespace ZeroCP
{
    /// @brief 共享内存文件描述符类型
using shm_handle_t = int;

enum class PosixSharedMemoryError : uint8_t
{
    EMPTY_NAME,                         // 名称为空
    INVALID_NAME,                       // 名称无效
    INVALID_FILE_NAME,                  // 文件名无效
    INSUFFICIENT_PERMISSIONS,           // 权限不足
    DOES_EXIST,                         // 已存在
    INCOMPATIBLE_OPEN_AND_ACCESS_MODE,  // 打开模式和访问模式不兼容
    UNKNOWN_ERROR
};

namespace Details
{

class PosixSharedMemory 
{
public:
    using Name_t = std::string<256>;
    static constexpr int INVALID_HANDLE = -1;
    PosixSharedMemory(const PosixSharedMemory&) = delete;
    PosixSharedMemory& operator=(const PosixSharedMemory&) = delete;
    PosixSharedMemory(PosixSharedMemory&&) noexcept = default;
    PosixSharedMemory& operator=(PosixSharedMemory&&) noexcept = default;
    ~PosixSharedMemory();

    shm_handle_t getHandle() const;

    // 实现 IMemorySize 接口
    uint64_t getMemorySize() const noexcept override;
    
    // 实现 ISharedMemory 接口
    void* getBaseMemory() const noexcept override;
    bool isValid() const noexcept override;

    expected<bool,PosixSharedMemoryError> create(Name_t name) noexcept;
    string<PosixSharedMemory::Name_t::capacity() + 1> addLeadingSlash(const PosixSharedMemory::Name_t& name) noexcept;

    friend class PosixSharedMemoryBuilder;
private:
    shm_handle_t m_handle{INVALID_HANDLE}; // 文件句柄
    Name_t m_name;
    uint64_t m_memorySize{0}; // 内存大小
    void* m_baseMemory{nullptr}; // 内存基地址
    bool m_hasOwnership{false}; // 是否拥有所有权
};

}
}

#endif