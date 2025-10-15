#ifndef ZeroCP_PosixSharedMemory_HPP
#define ZeroCP_PosixSharedMemory_HPP

namespace ZeroCP
{
    /// @brief 共享内存文件描述符类型
using shm_handle_t = int;
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

private:
    shm_handle_t m_handle;
    Name_t m_name;
    shm_handle_t m_handle{INVALID_HANDLE}; // 文件句柄
};

}
}

#endif