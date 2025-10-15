namespace ZeroCP
{

enum class AccessMode : uint8_t
{
    ReadOnly = 0,
    WriteOnly = 1,
    ReadWrite = 2
};

enum class OpenMode : uint8_t
{
    /// @brief 创建共享内存，如果已存在则构造失败
    ExclusiveCreate = 0U,
    /// @brief 创建共享内存，如果已存在则删除并重新创建
    PurgeAndCreate = 1U,
    /// @brief 创建共享内存，如果不存在则创建，否则打开已存在的
    OpenOrCreate = 2U,
    /// @brief 打开已存在的共享内存，如果不存在则失败
    OpenExisting = 3U
};

enum class Permissions : uint8_t
{
    None = 0;
    Read = 1;
    Write = 2;
    ReadWrite = 3;

};

}