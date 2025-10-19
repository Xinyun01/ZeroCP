#include "posix_sharedmemory.hpp"
#include "posix_memorymap.hpp"
#include "filesystem.hpp"
#include "logging.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
#include <thread>

using namespace ZeroCP;
using namespace ZeroCP::Details;

// 共享内存中的数据结构
struct SharedData
{
    int processId;
    int counter;
    char message[256];
    bool ready;
};

// 测试用例：父子进程间共享内存通信
void testCrossProcessCommunication()
{
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         Cross-Process Shared Memory Communication Test      ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    const char* shmName = "test_cross_process_shm";
    
    // 父进程创建共享内存
    auto shmResult = PosixSharedMemoryBuilder()
        .name(shmName)
        .memorySize(sizeof(SharedData))
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!shmResult)
    {
        std::cerr << "❌ Parent: Failed to create shared memory" << std::endl;
        return;
    }
    
    std::cout << "✅ Parent: Created shared memory" << std::endl;
    
    auto& shm = shmResult.value();
    
    // 创建内存映射
    auto mapResult = PosixMemoryMapBuilder()
        .fileDescriptor(shm.getHandle())
        .memoryLength(shm.getMemorySize())
        .prot(PROT_READ | PROT_WRITE)
        .flags(MAP_SHARED)
        .offset_(0)
        .create();
    
    if (!mapResult)
    {
        std::cerr << "❌ Parent: Failed to create memory map" << std::endl;
        return;
    }
    
    auto& memMap = mapResult.value();
    SharedData* data = static_cast<SharedData*>(memMap.getBaseAddress());
    
    std::cout << "✅ Parent: Created memory map at address " << memMap.getBaseAddress() << std::endl;
    
    // 初始化共享数据
    data->processId = getpid();
    data->counter = 0;
    std::strcpy(data->message, "Hello from parent");
    data->ready = false;
    
    std::cout << "✅ Parent: Initialized shared data" << std::endl;
    std::cout << "   Parent PID: " << getpid() << std::endl;
    std::cout << "   Initial message: \"" << data->message << "\"" << std::endl;
    
    // 创建子进程
    pid_t childPid = fork();
    
    if (childPid < 0)
    {
        std::cerr << "❌ Failed to fork process" << std::endl;
        return;
    }
    
    if (childPid == 0)
    {
        // 子进程
        std::cout << "\n--- Child Process Started ---" << std::endl;
        std::cout << "   Child PID: " << getpid() << std::endl;
        
        // 子进程打开已存在的共享内存
        auto childShmResult = PosixSharedMemoryBuilder()
            .name(shmName)
            .memorySize(sizeof(SharedData))
            .accessMode(AccessMode::ReadWrite)
            .openMode(OpenMode::OpenExisting)
            .create();
        
        if (!childShmResult)
        {
            std::cerr << "❌ Child: Failed to open shared memory" << std::endl;
            exit(1);
        }
        
        std::cout << "✅ Child: Opened shared memory" << std::endl;
        
        auto& childShm = childShmResult.value();
        
        // 子进程创建内存映射
        auto childMapResult = PosixMemoryMapBuilder()
            .fileDescriptor(childShm.getHandle())
            .memoryLength(childShm.getMemorySize())
            .prot(PROT_READ | PROT_WRITE)
            .flags(MAP_SHARED)
            .offset_(0)
            .create();
        
        if (!childMapResult)
        {
            std::cerr << "❌ Child: Failed to create memory map" << std::endl;
            exit(1);
        }
        
        auto& childMemMap = childMapResult.value();
        SharedData* childData = static_cast<SharedData*>(childMemMap.getBaseAddress());
        
        std::cout << "✅ Child: Created memory map at address " << childMemMap.getBaseAddress() << std::endl;
        
        // 读取父进程写入的数据
        std::cout << "✅ Child: Read data from parent" << std::endl;
        std::cout << "   Parent PID (from shared memory): " << childData->processId << std::endl;
        std::cout << "   Parent message: \"" << childData->message << "\"" << std::endl;
        
        // 子进程修改共享数据
        childData->counter = 42;
        std::strcpy(childData->message, "Hello from child");
        childData->ready = true;
        
        std::cout << "✅ Child: Modified shared data" << std::endl;
        std::cout << "   New counter: " << childData->counter << std::endl;
        std::cout << "   New message: \"" << childData->message << "\"" << std::endl;
        std::cout << "--- Child Process Exiting ---\n" << std::endl;
        
        exit(0);
    }
    else
    {
        // 父进程等待子进程完成
        std::cout << "\n--- Parent: Waiting for child process ---" << std::endl;
        
        int status;
        waitpid(childPid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            std::cout << "✅ Parent: Child process completed successfully" << std::endl;
        }
        else
        {
            std::cerr << "❌ Parent: Child process failed" << std::endl;
            return;
        }
        
        // 读取子进程修改的数据
        std::cout << "\n--- Parent: Reading modified data from child ---" << std::endl;
        std::cout << "   Counter (modified by child): " << data->counter << std::endl;
        std::cout << "   Message (modified by child): \"" << data->message << "\"" << std::endl;
        std::cout << "   Ready flag: " << (data->ready ? "true" : "false") << std::endl;
        
        // 验证数据
        if (data->counter == 42 && 
            std::strcmp(data->message, "Hello from child") == 0 &&
            data->ready == true)
        {
            std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
            std::cout << "║         ✅ Cross-Process Communication Test PASSED          ║" << std::endl;
            std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
        }
        else
        {
            std::cerr << "\n❌ Data verification failed!" << std::endl;
        }
    }
}

// 测试用例：验证内存映射的零拷贝特性
void testZeroCopyMapping()
{
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              Zero-Copy Memory Mapping Test                  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    const char* shmName = "test_zero_copy_mapping";
    const size_t bufferSize = 4096;
    
    // 创建共享内存
    auto shmResult = PosixSharedMemoryBuilder()
        .name(shmName)
        .memorySize(bufferSize)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!shmResult)
    {
        std::cerr << "❌ Failed to create shared memory" << std::endl;
        return;
    }
    
    auto& shm = shmResult.value();
    std::cout << "✅ Created shared memory of " << bufferSize << " bytes" << std::endl;
    
    // 创建第一个内存映射
    auto map1Result = PosixMemoryMapBuilder()
        .fileDescriptor(shm.getHandle())
        .memoryLength(shm.getMemorySize())
        .prot(PROT_READ | PROT_WRITE)
        .flags(MAP_SHARED)
        .offset_(0)
        .create();
    
    if (!map1Result)
    {
        std::cerr << "❌ Failed to create first memory map" << std::endl;
        return;
    }
    
    auto& map1 = map1Result.value();
    char* addr1 = static_cast<char*>(map1.getBaseAddress());
    std::cout << "✅ Created first memory map at " << static_cast<void*>(addr1) << std::endl;
    
    // 创建第二个内存映射（映射到同一个共享内存）
    auto map2Result = PosixMemoryMapBuilder()
        .fileDescriptor(shm.getHandle())
        .memoryLength(shm.getMemorySize())
        .prot(PROT_READ | PROT_WRITE)
        .flags(MAP_SHARED)
        .offset_(0)
        .create();
    
    if (!map2Result)
    {
        std::cerr << "❌ Failed to create second memory map" << std::endl;
        return;
    }
    
    auto& map2 = map2Result.value();
    char* addr2 = static_cast<char*>(map2.getBaseAddress());
    std::cout << "✅ Created second memory map at " << static_cast<void*>(addr2) << std::endl;
    
    // 通过第一个映射写入数据
    const char* testData = "Zero-Copy Test: This data should be visible through both mappings!";
    std::strcpy(addr1, testData);
    
    std::cout << "\n--- Testing Zero-Copy Property ---" << std::endl;
    std::cout << "✅ Wrote through map1: \"" << addr1 << "\"" << std::endl;
    std::cout << "✅ Read through map2:  \"" << addr2 << "\"" << std::endl;
    
    // 验证两个映射看到的是同一份数据
    if (std::strcmp(addr1, addr2) == 0)
    {
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║         ✅ Zero-Copy Property Verified Successfully!        ║" << std::endl;
        std::cout << "║   Both mappings see the same physical memory (no copy)     ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    }
    else
    {
        std::cerr << "\n❌ Zero-copy verification failed!" << std::endl;
    }
}

// 测试用例：大数据传输性能
void testLargeDataTransfer()
{
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║            Large Data Transfer Performance Test             ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    const char* shmName = "test_large_transfer";
    const size_t dataSize = 10 * 1024 * 1024;  // 10 MB
    
    // 创建共享内存
    auto shmResult = PosixSharedMemoryBuilder()
        .name(shmName)
        .memorySize(dataSize)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!shmResult)
    {
        std::cerr << "❌ Failed to create shared memory" << std::endl;
        return;
    }
    
    auto& shm = shmResult.value();
    std::cout << "✅ Created shared memory of " << (dataSize / 1024 / 1024) << " MB" << std::endl;
    
    // 创建内存映射
    auto mapResult = PosixMemoryMapBuilder()
        .fileDescriptor(shm.getHandle())
        .memoryLength(shm.getMemorySize())
        .prot(PROT_READ | PROT_WRITE)
        .flags(MAP_SHARED)
        .offset_(0)
        .create();
    
    if (!mapResult)
    {
        std::cerr << "❌ Failed to create memory map" << std::endl;
        return;
    }
    
    auto& memMap = mapResult.value();
    char* data = static_cast<char*>(memMap.getBaseAddress());
    
    // 测试写入性能
    std::cout << "\n--- Testing write performance ---" << std::endl;
    auto startWrite = std::chrono::high_resolution_clock::now();
    
    // 填充数据
    for (size_t i = 0; i < dataSize; ++i)
    {
        data[i] = static_cast<char>(i % 256);
    }
    
    auto endWrite = std::chrono::high_resolution_clock::now();
    auto writeTime = std::chrono::duration_cast<std::chrono::milliseconds>(endWrite - startWrite);
    
    std::cout << "✅ Wrote " << (dataSize / 1024 / 1024) << " MB in " << writeTime.count() << " ms" << std::endl;
    std::cout << "   Throughput: " << (dataSize / 1024.0 / 1024.0) / (writeTime.count() / 1000.0) << " MB/s" << std::endl;
    
    // 测试读取性能
    std::cout << "\n--- Testing read performance ---" << std::endl;
    auto startRead = std::chrono::high_resolution_clock::now();
    
    // 读取并验证数据
    size_t errorCount = 0;
    for (size_t i = 0; i < dataSize && errorCount < 10; ++i)
    {
        if (data[i] != static_cast<char>(i % 256))
        {
            errorCount++;
        }
    }
    
    auto endRead = std::chrono::high_resolution_clock::now();
    auto readTime = std::chrono::duration_cast<std::chrono::milliseconds>(endRead - startRead);
    
    std::cout << "✅ Read and verified " << (dataSize / 1024 / 1024) << " MB in " << readTime.count() << " ms" << std::endl;
    std::cout << "   Throughput: " << (dataSize / 1024.0 / 1024.0) / (readTime.count() / 1000.0) << " MB/s" << std::endl;
    
    if (errorCount == 0)
    {
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║         ✅ Large Data Transfer Test PASSED                  ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    }
    else
    {
        std::cerr << "\n❌ Data verification failed with " << errorCount << " errors!" << std::endl;
    }
}

int main()
{
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║      Advanced Shared Memory & Memory Mapping Tests         ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    try
    {
        // 测试跨进程通信
        testCrossProcessCommunication();
        
        // 测试零拷贝特性
        testZeroCopyMapping();
        
        // 测试大数据传输
        testLargeDataTransfer();
        
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║              ✅ All Advanced Tests Completed!               ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ Exception caught: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

