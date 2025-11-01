/**
 * @file relative_pointer_example.cpp
 * @brief RelativePointer 完整使用示例
 * 
 * 演示如何在共享内存中使用 RelativePointer 实现跨进程的数据结构
 */

#include "zerocp_foundationLib/posix/memory/include/relative_pointer.hpp"
#include "zerocp_daemon/sharememory/include/posixshm_provider.hpp"
#include <iostream>
#include <cstring>

using namespace ZeroCP;
using namespace ZeroCP::Daemon;

// ==================== 数据结构定义 ====================

/**
 * @brief 消息节点（链表结构）
 * 
 * 使用 RelativePointer 而不是原生指针，确保跨进程安全
 */
struct Message
{
    int id;
    char content[256];
    RelativePointer<Message> next;  // 相对指针，指向下一个消息
    
    Message() : id(0), next(0, 0) 
    {
        content[0] = '\0';
    }
};

/**
 * @brief 共享内存头部
 * 
 * 存储元数据和第一个消息的指针
 */
struct SharedMemoryHeader
{
    uint64_t messageCount;
    RelativePointer<Message> firstMessage;
    
    SharedMemoryHeader() : messageCount(0), firstMessage(0, 0) {}
};

// ==================== 示例函数 ====================

/**
 * @brief 写入进程：创建共享内存并写入链表
 */
void writerProcess()
{
    std::cout << "\n=== Writer Process ===" << std::endl;
    
    // 1. 创建共享内存提供者
    PosixShmProvider shmProvider(
        "/zerocopy_messages",
        64 * 1024,  // 64KB
        AccessMode::ReadWrite,
        OpenMode::CreateOrOpen,
        Perms::OwnerReadWrite | Perms::GroupRead | Perms::OthersRead
    );
    
    // 2. 创建共享内存
    auto result = shmProvider.createMemory();
    if (!result.has_value())
    {
        std::cerr << "Failed to create shared memory!" << std::endl;
        return;
    }
    
    void* baseAddress = result.value();
    uint64_t poolId = shmProvider.getPoolId();
    
    std::cout << "Shared Memory Created:" << std::endl;
    std::cout << "  - Pool ID: " << poolId << std::endl;
    std::cout << "  - Base Address: " << baseAddress << std::endl;
    
    // 3. 初始化共享内存头部
    auto* header = static_cast<SharedMemoryHeader*>(baseAddress);
    new (header) SharedMemoryHeader();  // placement new
    
    // 4. 计算消息存储区域
    auto* messageArea = reinterpret_cast<Message*>(
        static_cast<char*>(baseAddress) + sizeof(SharedMemoryHeader)
    );
    
    // 5. 创建消息链表
    const int MESSAGE_COUNT = 5;
    Message* prevMsg = nullptr;
    
    for (int i = 0; i < MESSAGE_COUNT; ++i)
    {
        Message* currentMsg = &messageArea[i];
        new (currentMsg) Message();  // placement new
        
        currentMsg->id = i + 1;
        snprintf(currentMsg->content, sizeof(currentMsg->content), 
                 "Message #%d from writer process", i + 1);
        
        // 设置链表连接
        if (i == 0)
        {
            // 第一个消息
            header->firstMessage = RelativePointer<Message>(currentMsg, poolId);
        }
        else
        {
            // 连接前一个消息
            prevMsg->next = RelativePointer<Message>(currentMsg, poolId);
        }
        
        prevMsg = currentMsg;
    }
    
    // 最后一个消息的 next 指向 nullptr
    prevMsg->next = RelativePointer<Message>(nullptr, poolId);
    header->messageCount = MESSAGE_COUNT;
    
    std::cout << "Written " << MESSAGE_COUNT << " messages to shared memory" << std::endl;
    
    // 6. 验证写入（通过 RelativePointer 遍历）
    std::cout << "\nVerifying written data:" << std::endl;
    RelativePointer<Message> current = header->firstMessage;
    int count = 0;
    
    while (current)
    {
        std::cout << "  Message " << current->id << ": " << current->content << std::endl;
        current = current->next;
        count++;
    }
    
    std::cout << "Verified " << count << " messages" << std::endl;
    
    // 7. 通知内存可用
    shmProvider.announceMemoryAvailable();
    
    std::cout << "\nPress Enter to destroy shared memory..." << std::endl;
    std::cin.get();
    
    // 8. 清理（析构函数会自动取消注册段）
}

/**
 * @brief 读取进程：打开现有共享内存并读取链表
 */
void readerProcess()
{
    std::cout << "\n=== Reader Process ===" << std::endl;
    
    // 1. 打开现有共享内存
    PosixShmProvider shmProvider(
        "/zerocopy_messages",
        64 * 1024,
        AccessMode::ReadOnly,
        OpenMode::OpenExisting,  // 只打开已存在的
        Perms::OwnerReadWrite
    );
    
    // 2. 映射共享内存
    auto result = shmProvider.createMemory();
    if (!result.has_value())
    {
        std::cerr << "Failed to open shared memory! Is the writer running?" << std::endl;
        return;
    }
    
    void* baseAddress = result.value();
    uint64_t poolId = shmProvider.getPoolId();
    
    std::cout << "Shared Memory Opened:" << std::endl;
    std::cout << "  - Pool ID: " << poolId << std::endl;
    std::cout << "  - Base Address: " << baseAddress << std::endl;
    
    // 3. 读取头部
    auto* header = static_cast<SharedMemoryHeader*>(baseAddress);
    
    std::cout << "Message count: " << header->messageCount << std::endl;
    
    // 4. 遍历消息链表
    std::cout << "\nReading messages:" << std::endl;
    RelativePointer<Message> current = header->firstMessage;
    int count = 0;
    
    while (current)
    {
        std::cout << "  [" << count + 1 << "] ID=" << current->id 
                  << ", Content: " << current->content << std::endl;
        
        // 关键点：RelativePointer 自动将偏移量转换为当前进程的虚拟地址
        current = current->next;
        count++;
    }
    
    std::cout << "\nRead " << count << " messages successfully" << std::endl;
    
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
}

/**
 * @brief 高级示例：跨池引用
 */
void crossPoolExample()
{
    std::cout << "\n=== Cross-Pool Reference Example ===" << std::endl;
    
    // 创建两个共享内存池
    PosixShmProvider metadataProvider(
        "/zerocopy_metadata",
        4096,
        AccessMode::ReadWrite,
        OpenMode::CreateOrOpen,
        Perms::OwnerReadWrite
    );
    
    PosixShmProvider dataProvider(
        "/zerocopy_data",
        64 * 1024,
        AccessMode::ReadWrite,
        OpenMode::CreateOrOpen,
        Perms::OwnerReadWrite
    );
    
    // 创建内存
    auto metaResult = metadataProvider.createMemory();
    auto dataResult = dataProvider.createMemory();
    
    if (!metaResult.has_value() || !dataResult.has_value())
    {
        std::cerr << "Failed to create shared memory pools!" << std::endl;
        return;
    }
    
    void* metaBase = metaResult.value();
    void* dataBase = dataResult.value();
    
    uint64_t metaPoolId = metadataProvider.getPoolId();
    uint64_t dataPoolId = dataProvider.getPoolId();
    
    std::cout << "Created two pools:" << std::endl;
    std::cout << "  - Metadata Pool: ID=" << metaPoolId << ", Base=" << metaBase << std::endl;
    std::cout << "  - Data Pool: ID=" << dataPoolId << ", Base=" << dataBase << std::endl;
    
    // 在元数据池中存储指向数据池的指针
    struct Metadata
    {
        RelativePointer<char> dataPtr;  // 指向另一个池
        uint64_t dataSize;
    };
    
    auto* meta = static_cast<Metadata*>(metaBase);
    char* data = static_cast<char*>(dataBase);
    
    // 写入数据
    const char* message = "Hello from data pool!";
    strcpy(data, message);
    
    // 创建跨池相对指针
    meta->dataPtr = RelativePointer<char>(data, dataPoolId);
    meta->dataSize = strlen(message);
    
    // 读取验证
    std::cout << "\nCross-pool access:" << std::endl;
    std::cout << "  Data: " << meta->dataPtr.get() << std::endl;
    std::cout << "  Size: " << meta->dataSize << std::endl;
    
    std::cout << "\nPress Enter to clean up..." << std::endl;
    std::cin.get();
}

// ==================== 主函数 ====================

int main(int argc, char* argv[])
{
    std::cout << "RelativePointer Example" << std::endl;
    std::cout << "======================" << std::endl;
    
    if (argc < 2)
    {
        std::cout << "\nUsage:" << std::endl;
        std::cout << "  " << argv[0] << " writer    - Run as writer process" << std::endl;
        std::cout << "  " << argv[0] << " reader    - Run as reader process" << std::endl;
        std::cout << "  " << argv[0] << " cross     - Run cross-pool example" << std::endl;
        return 0;
    }
    
    std::string mode(argv[1]);
    
    if (mode == "writer")
    {
        writerProcess();
    }
    else if (mode == "reader")
    {
        readerProcess();
    }
    else if (mode == "cross")
    {
        crossPoolExample();
    }
    else
    {
        std::cerr << "Unknown mode: " << mode << std::endl;
        return 1;
    }
    
    return 0;
}

