#ifndef PosixSharedMemoryObject_HPP
#define PosixSharedMemoryObject_HPP

#include "builder.hpp"
namespace ZeroCP
{

namespace Details
{

enum class PosixSharedMemoryObjectError
{
    InvalidName,                  // 名称非法（为空 / 含有不允许字符）
    InvalidSize,                  // 请求的共享内存大小无效（0 或超出上限）
    PermissionDenied,             // 权限不足（比如没权限打开已有的 SHM）
    AlreadyExists,                // shm_open(O_CREAT | O_EXCL) 时，已存在
    DoesNotExist,                 // shm_open 仅打开，但目标不存在
    ShmOpenFailed   
}



class PosixSharedMemoryObject
{
public:
    PosixSharedMemoryObject();

};
class PosixSharedMemoryObjectBuilder;

class PosixSharedMemoryObject
{
public:
    PosixSharedMemoryObject(const PosixSharedMemoryObject&) = delete;
    PosixSharedMemoryObject& operator=(const PosixSharedMemoryObject&) = delete;

    PosixSharedMemoryObject(PosixSharedMemoryObject&&) noexcept = default;
    PosixSharedMemoryObject& operator=(PosixSharedMemoryObject&&) noexcept = default;
    //获取共享内存的基地址
    *void getBaseMemory();
    friend class PosixSharedMemoryObjectBuilder;

private:
};

/*
* @param type          成员变量类型
* @param name          setter 函数名，同时形成成员变量 m_name
* @param defaultvalue  成员变量默认值
*/
class PosixSharedMemoryObjectBuilder
{
    /// 共享内存的有效名称，不允许以点号开头（部分文件系统不兼容）。
    ZeroCP_Builder_Implementation(Name_t,name,"");
    /// 共享内存的大小（字节）。
    ZeroCP_Builder_Implementation(uint64_t,memorySize,0U);
    /// 共享内存的访问权限：只读或可写。只读内存区域写入会导致段错误。
    ZeroCP_Builder_Implementation(AccessMode, accessMode, AccessMode::ReadOnly)
    /// 共享内存的打开模式：创建、删除并重新创建、创建或打开已存在的、打开已存在的。
    ZeroCP_Builder_Implementation(OpenMode, openMode, OpenMode::OpenExisting)
    /// 共享内存的权限设置（如读、写、无权限等）
    ZeroCP_Builder_Implementation(uint8_t,permissions,0U);

public:
    expected<PosixSharedMemoryObject,PosixSharedMemoryObjectError> create() const;
}

}    

};



