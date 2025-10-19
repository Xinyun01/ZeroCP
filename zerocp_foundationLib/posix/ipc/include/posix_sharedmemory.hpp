#ifndef ZeroCP_PosixSharedMemory_HPP
#define ZeroCP_PosixSharedMemory_HPP
#include "filesystem.hpp"
#include "builder.hpp"
#include <expected>
#include <utility>
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
    using Name_t = std::string;
    static constexpr int INVALID_HANDLE = -1;
    PosixSharedMemory(const PosixSharedMemory&) = delete;
    PosixSharedMemory& operator=(const PosixSharedMemory&) = delete;
    PosixSharedMemory(PosixSharedMemory&&) noexcept;
    PosixSharedMemory& operator=(PosixSharedMemory&&) noexcept;
    ~PosixSharedMemory();

    shm_handle_t getHandle() const;
    bool hasOwnership() const noexcept;

    // 获取共享内存大小
    uint64_t getMemorySize() const noexcept;

    friend class PosixSharedMemoryBuilder;
    
private:
    PosixSharedMemory(const Name_t& name, const shm_handle_t handle, const bool hasOwnership) noexcept;
    
    shm_handle_t m_handle{INVALID_HANDLE}; // 文件句柄
    Name_t m_name;
    bool m_hasOwnership{false}; // 是否拥有所有权
};

class PosixSharedMemoryBuilder
{
    using Name_t = std::string;
    
    /// 共享内存的有效名称
    ZeroCP_Builder_Implementation(Name_t, name, "");
    /// 共享内存的大小（字节）
    ZeroCP_Builder_Implementation(uint64_t, memorySize, 0U);
    /// 共享内存的访问权限
    ZeroCP_Builder_Implementation(AccessMode, accessMode, AccessMode::ReadOnly);
    /// 共享内存的打开模式
    ZeroCP_Builder_Implementation(OpenMode, openMode, OpenMode::OpenExisting);
    /// 共享内存的文件权限设置
    ZeroCP_Builder_Implementation(Perms, filePermissions, Perms::OwnerAll);

public:
    std::expected<PosixSharedMemory, PosixSharedMemoryError> create() noexcept;
};

} // namespace Details
} // namespace ZeroCP

#endif