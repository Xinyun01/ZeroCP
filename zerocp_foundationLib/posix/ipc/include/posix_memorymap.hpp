#ifndef ZEROCP_POSIX_MEMORYMAP_HPP
#define ZEROCP_POSIX_MEMORYMAP_HPP
#include "builder.hpp"
// void *mmap(void *addr,           // 建议的映射地址
//     size_t length,        // 映射长度
//     int prot,             // 保护模式
//     int flags,            // 映射标志
//     int fd,               // 文件描述符
//     off_t offset);        // 文件偏移
namespace ZeroCP
{
namespace Details
{

/// @brief POSIX 内存映射相关错误类型
enum class PosixMemoryMapError : uint8_t
{
    ACCESS_FAILED,                            // 访问失败
    UNABLE_TO_LOCK,                           // 无法加锁
    INVALID_FILE_DESCRIPTOR,                  // 文件描述符无效
    MAP_OVERLAP,                              // 映射区域重叠
    INVALID_PARAMETERS,                       // 参数无效
    OPEN_FILES_SYSTEM_LIMIT_EXCEEDED,         // 已打开的文件数量超出系统限制
    FILESYSTEM_DOES_NOT_SUPPORT_MEMORY_MAPPING, // 文件系统不支持内存映射
    NOT_ENOUGH_MEMORY_AVAILABLE,              // 可用内存不足
    OVERFLOWING_PARAMETERS,                   // 参数溢出
    PERMISSION_FAILURE,                       // 权限失败
    NO_WRITE_PERMISSION,                      // 没有写权限
    UNKNOWN_ERROR                             // 未知错误
};

/// @brief POSIX 内存映射标志位
enum class PosixMemoryMapFlags : int32_t
{
    /// @brief 变更为共享（MAP_SHARED）
    SHARE_CHANGES = MAP_SHARED,

    /// @brief 变更为私有（MAP_PRIVATE）
    PRIVATE_CHANGES = MAP_PRIVATE,

};

// POSIX 内存映射保护位
enum class PosixMemoryMapProt : int8_t
{
    PROT_NONE = PROT_NONE,                 // 无访问权限
    PROT_READ = PROT_READ,                 // 只读权限
    PROT_WRITE = PROT_WRITE,               // 可写权限
    PROT_EXEC = PROT_EXEC,                 // 可执行权限
    PROT_READ_WRITE = PROT_READ | PROT_WRITE, // 读写权限
    PROT_READ_EXEC = PROT_READ | PROT_EXEC,   // 读执行权限
};

class PosixSharedMemoryBuilder
{
    // 内存映射的起始地址，默认为nullptr
    ZeroCP_Builder_Implementation(void*, baseMemory, nullptr);

    // 内存映射的大小，默认为0
    ZeroCP_Builder_Implementation(uint64_t, memoryLength, 0);

    // 保护标志，默认为只读（PROT_READ）
    ZeroCP_Builder_Implementation(uint8_t, prot, PosixMemoryMapProt::PROT_READ);

    // 映射标志，默认为共享（MAP_SHARED）
    ZeroCP_Builder_Implementation(int32_t, flags, PosixMemoryMapFlags::SHARE_CHANGES);

    // 文件描述符，默认为-1（无效）
    ZeroCP_Builder_Implementation(shm_handle_t,fileDescriptor, -1);

    // 映射的偏移量，默认为0
    ZeroCP_Builder_Implementation(uint64_t, offset_, 0);

    // 创建并返回PosixMemoryMap对象
    expected<PosixMemoryMap, PosixMemoryMapError> create() noexcept;

}

class PosixMemoryMap
{
public:
    PosixMemoryMap(const PosixMemoryMap&) = delete;
    PosixMemoryMap& operator=(const PosixMemoryMap&) = delete;
    PosixMemoryMap(PosixMemoryMap&&) noexcept = default;
    PosixMemoryMap& operator=(PosixMemoryMap&&) noexcept = default;
    ~PosixMemoryMap();
    void* getBaseMemory() const noexcept;
    size_t getMemorySize() const noexcept;
private:
    void* m_baseAddress{nullptr};
    uint64_t m_length{0U};
}

}

}


#endif // ZEROCP_POSIX_MEMORYMAP_HPP