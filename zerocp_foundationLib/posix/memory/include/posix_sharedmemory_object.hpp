#ifndef PosixSharedMemoryObject_HPP
#define PosixSharedMemoryObject_HPP

#include <expected>
#include <optional>
#include <utility>
#include <string>
#include "builder.hpp"
#include "../../filesystem/include/filesystem.hpp"
#include "posix_sharedmemory.hpp"
#include "posix_memorymap.hpp"

namespace ZeroCP
{

namespace Details
{

enum class PosixSharedMemoryObjectError
{
    INVALID_NAME,                  // 名称非法（为空 / 含有不允许字符）
    INVALID_SIZE,                  // 请求的共享内存大小无效（0 或超出上限）
    PERMISSION_DENIED,             // 权限不足（比如没权限打开已有的 SHM）
    ALREADY_EXISTS,                // shm_open(O_CREAT | O_EXCL) 时，已存在
    DOES_NOT_EXIST,                // shm_open 仅打开，但目标不存在
    SHM_OPEN_FAILED,
    UNABLE_TO_VERIFY_MEMORY_SIZE,
    REQUESTED_SIZE_EXCEEDS_ACTUAL_SIZE,
    UNKNOWN_ERROR
};



class PosixSharedMemoryObjectBuilder;

class PosixSharedMemoryObject
{
public:
    PosixSharedMemoryObject(const PosixSharedMemoryObject&) = delete;
    PosixSharedMemoryObject& operator=(const PosixSharedMemoryObject&) = delete;

    PosixSharedMemoryObject(PosixSharedMemoryObject&&) noexcept = default;
    PosixSharedMemoryObject& operator=(PosixSharedMemoryObject&&) noexcept = default;
    ~PosixSharedMemoryObject() noexcept = default;
    // 获取共享内存的基地址
    const void* getBaseAddress() const noexcept;
    void* getBaseAddress() noexcept;
    
    // 获取文件句柄
    shm_handle_t getFileHandle() const noexcept;
    
    // 检查是否拥有共享内存的所有权
    bool hasOwnership() const noexcept;
    
    friend class PosixSharedMemoryObjectBuilder;

private:
    PosixSharedMemoryObject(Details::PosixSharedMemory&& sharedMemory,
                           Details::PosixMemoryMap&& memoryMap) noexcept;
    
    Details::PosixSharedMemory m_sharedMemory;
    Details::PosixMemoryMap m_memoryMap;
};

/*
* @param type          成员变量类型
* @param name          setter 函数名，同时形成成员变量 m_name
* @param defaultvalue  成员变量默认值
*/
class PosixSharedMemoryObjectBuilder
{
    using Name_t = std::string;
    
    /// 共享内存的有效名称，不允许以点号开头（部分文件系统不兼容）。
    ZeroCP_Builder_Implementation(Name_t, name, "");
    /// 共享内存的大小（字节）。
    ZeroCP_Builder_Implementation(uint64_t, memorySize, 0U);
    /// 共享内存的访问权限：只读或可写。只读内存区域写入会导致段错误。
    ZeroCP_Builder_Implementation(AccessMode, accessMode, AccessMode::ReadOnly);
    /// 共享内存的打开模式：创建、删除并重新创建、创建或打开已存在的、打开已存在的。
    ZeroCP_Builder_Implementation(OpenMode, openMode, OpenMode::OpenExisting);
    /// 共享内存的权限设置（如读、写、无权限等）
    ZeroCP_Builder_Implementation(Perms, permissions, Perms::None);
    /// 内存映射的基地址提示
    ZeroCP_Builder_Implementation(std::optional<void*>, baseAddressHint, std::nullopt);

public:
    std::expected<PosixSharedMemoryObject, PosixSharedMemoryObjectError> create() noexcept;
};

} // namespace Details
} // namespace ZeroCP

#endif



