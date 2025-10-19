#include "posix_sharedmemory.hpp"
#include "posix_memorymap.hpp"
#include "filesystem.hpp"
#include "logging.hpp"
#include <iostream>
#include <cstring>

using namespace ZeroCP;
using namespace ZeroCP::Details;


// AccessMode：控制当前进程如何访问文件描述符（读/写/读写）
// OpenMode：控制文件的生命周期管理（创建/打开/删除）
// FilePermissions：控制文件系统级别的访问控制（所有者/组/其他用户的权限）
// 三者协同工作：
// OpenMode 决定是否创建文件
// AccessMode 决定当前进程对文件的操作方式
// FilePermissions 决定文件在文件系统中的安全边界
// 这就像是：
// OpenMode：决定"这扇门是新装的还是已有的"
// AccessMode：决定"我手里拿的是哪种钥匙（只读/只写/读写）"
// FilePermissions：决定"这扇门的锁允许谁进入"

// 测试用例1: 创建新的共享内存
void testCase1_CreateNew()
{
    std::cout << "\n=== Test Case 1: Create new shared memory ===" << std::endl;
    
    auto result = PosixSharedMemoryBuilder()
        .name("test_shm_new")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!result)
    {
        std::cerr << "❌ Failed to create shared memory: Error code " 
                  << static_cast<int>(result.error()) << std::endl;
        return;
    }
    
    auto& shm = result.value();
    
    std::cout << "✅ Successfully created shared memory" << std::endl;
    std::cout << "   Handle: " << shm.getHandle() << std::endl;
    std::cout << "   Has ownership: " << (shm.hasOwnership() ? "Yes" : "No") << std::endl;
    std::cout << "   Memory size: " << shm.getMemorySize() << " bytes" << std::endl;
    
    if (shm.getHandle() == PosixSharedMemory::INVALID_HANDLE)
    {
        std::cerr << "❌ Invalid handle" << std::endl;
        return;
    }
    
    if (!shm.hasOwnership())
    {
        std::cerr << "❌ Should have ownership" << std::endl;
        return;
    }
    
    if (shm.getMemorySize() < 4096)
    {
        std::cerr << "❌ Memory size is less than expected (got " << shm.getMemorySize() << ", expected >= 4096)" << std::endl;
        return;
    }
}

// 测试用例2: 打开已存在的共享内存
void testCase2_OpenExisting()
{
    std::cout << "\n=== Test Case 2: Open existing shared memory ===" << std::endl;
    
    // 首先创建一个共享内存
    auto createResult = PosixSharedMemoryBuilder()
        .name("test_shm_existing")
        .memorySize(8192)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!createResult)
    {
        std::cerr << "❌ Failed to create shared memory for test" << std::endl;
        return;
    }
    
    std::cout << "✅ Created shared memory with 8192 bytes" << std::endl;
    
    // 现在尝试打开它
    auto openResult = PosixSharedMemoryBuilder()
        .name("test_shm_existing")
        .memorySize(8192)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::OpenExisting)
        .create();
    
    if (!openResult)
    {
        std::cerr << "❌ Failed to open existing shared memory: Error code " 
                  << static_cast<int>(openResult.error()) << std::endl;
        return;
    }
    
    auto& shm = openResult.value();
    
    std::cout << "✅ Successfully opened existing shared memory" << std::endl;
    std::cout << "   Handle: " << shm.getHandle() << std::endl;
    std::cout << "   Has ownership: " << (shm.hasOwnership() ? "Yes" : "No") << std::endl;
    std::cout << "   Memory size: " << shm.getMemorySize() << " bytes" << std::endl;
}

// 测试用例3: OpenOrCreate 模式
void testCase3_OpenOrCreate()
{
    std::cout << "\n=== Test Case 3: OpenOrCreate mode ===" << std::endl;
    
    // 第一次调用应该创建
    auto result1 = PosixSharedMemoryBuilder()
        .name("test_shm_or_create")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::OpenOrCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!result1)
    {
        std::cerr << "❌ Failed on first OpenOrCreate" << std::endl;
        return;
    }
    
    std::cout << "✅ First OpenOrCreate succeeded (created)" << std::endl;
    std::cout << "   Has ownership: " << (result1.value().hasOwnership() ? "Yes" : "No") << std::endl;
    
    // 第二次调用应该打开已存在的
    auto result2 = PosixSharedMemoryBuilder()
        .name("test_shm_or_create")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::OpenOrCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!result2)
    {
        std::cerr << "❌ Failed on second OpenOrCreate" << std::endl;
        return;
    }
    
    std::cout << "✅ Second OpenOrCreate succeeded (opened existing)" << std::endl;
    std::cout << "   Has ownership: " << (result2.value().hasOwnership() ? "Yes" : "No") << std::endl;
}

// 测试用例4: 内存映射和数据读写
void testCase4_MemoryMapAndReadWrite()
{
    std::cout << "\n=== Test Case 4: Memory mapping and data read/write ===" << std::endl;
    
    // 创建共享内存
    auto shmResult = PosixSharedMemoryBuilder()
        .name("test_shm_readwrite")
        .memorySize(4096)
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
    std::cout << "✅ Created shared memory" << std::endl;
    
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
        std::cerr << "❌ Failed to create memory map: Error code " 
                  << static_cast<int>(mapResult.error()) << std::endl;
        return;
    }
    
    auto& memMap = mapResult.value();
    void* baseAddr = memMap.getBaseAddress();
    
    std::cout << "✅ Created memory map" << std::endl;
    std::cout << "   Base address: " << baseAddr << std::endl;
    std::cout << "   Length: " << memMap.getLength() << " bytes" << std::endl;
    
    // 写入测试数据
    const char* testData = "Hello, ZeroCopy Framework! This is a test message.";
    size_t dataLen = std::strlen(testData) + 1;
    std::memcpy(baseAddr, testData, dataLen);
    
    std::cout << "✅ Wrote data to shared memory: \"" << testData << "\"" << std::endl;
    
    // 读取数据验证
    char readBuffer[256];
    std::memcpy(readBuffer, baseAddr, dataLen);
    
    if (std::strcmp(testData, readBuffer) == 0)
    {
        std::cout << "✅ Data verification successful: \"" << readBuffer << "\"" << std::endl;
    }
    else
    {
        std::cerr << "❌ Data mismatch!" << std::endl;
    }
}

// 测试用例5: 不同访问模式
void testCase5_AccessModes()
{
    std::cout << "\n=== Test Case 5: Different access modes ===" << std::endl;
    
    // 只读模式 - 先创建，再以只读模式打开
    auto createForReadOnly = PosixSharedMemoryBuilder()
        .name("test_shm_readonly")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!createForReadOnly)
    {
        std::cerr << "❌ Failed to create shared memory for read-only test" << std::endl;
    }
    else
    {
        // 现在以只读模式打开
        auto readOnlyResult = PosixSharedMemoryBuilder()
            .name("test_shm_readonly")
            .memorySize(4096)
            .accessMode(AccessMode::ReadOnly)
            .openMode(OpenMode::OpenExisting)
            .create();
        
        if (readOnlyResult)
        {
            std::cout << "✅ Opened shared memory in read-only mode" << std::endl;
        }
        else
        {
            std::cerr << "❌ Failed to open shared memory in read-only mode" << std::endl;
        }
    }
    
    // 只写模式
    auto writeOnlyResult = PosixSharedMemoryBuilder()
        .name("test_shm_writeonly")
        .memorySize(4096)
        .accessMode(AccessMode::WriteOnly)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (writeOnlyResult)
    {
        std::cout << "✅ Created write-only shared memory" << std::endl;
    }
    else
    {
        std::cerr << "❌ Failed to create write-only shared memory" << std::endl;
    }
    
    // 读写模式
    auto readWriteResult = PosixSharedMemoryBuilder()
        .name("test_shm_readwrite_mode")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (readWriteResult)
    {
        std::cout << "✅ Created read-write shared memory" << std::endl;
    }
    else
    {
        std::cerr << "❌ Failed to create read-write shared memory" << std::endl;
    }
}

// 测试用例6: 不同大小的共享内存
void testCase6_DifferentSizes()
{
    std::cout << "\n=== Test Case 6: Different memory sizes ===" << std::endl;
    
    const uint64_t sizes[] = {1024, 4096, 16384, 65536, 1048576};  // 1KB to 1MB
    const char* sizeNames[] = {"1KB", "4KB", "16KB", "64KB", "1MB"};
    
    for (size_t i = 0; i < 5; ++i)
    {
        std::string shmName = "test_shm_size_" + std::to_string(i);
        
        auto result = PosixSharedMemoryBuilder()
            .name(shmName)
            .memorySize(sizes[i])
            .accessMode(AccessMode::ReadWrite)
            .openMode(OpenMode::PurgeAndCreate)
            .filePermissions(Perms::OwnerAll)
            .create();
        
        if (result)
        {
            std::cout << "✅ Created " << sizeNames[i] << " shared memory (actual: " 
                      << result.value().getMemorySize() << " bytes)" << std::endl;
        }
        else
        {
            std::cerr << "❌ Failed to create " << sizeNames[i] << " shared memory" << std::endl;
        }
    }
}

// 测试用例7: 错误处理 - 空名称
void testCase7_ErrorHandling_EmptyName()
{
    std::cout << "\n=== Test Case 7: Error handling - empty name ===" << std::endl;
    
    auto result = PosixSharedMemoryBuilder()
        .name("")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!result)
    {
        if (result.error() == PosixSharedMemoryError::EMPTY_NAME)
        {
            std::cout << "✅ Correctly detected empty name error" << std::endl;
        }
        else
        {
            std::cerr << "⚠️  Failed with different error: " 
                      << static_cast<int>(result.error()) << std::endl;
        }
    }
    else
    {
        std::cerr << "❌ Should have failed due to empty name!" << std::endl;
    }
}

// 测试用例8: 错误处理 - ExclusiveCreate 模式下已存在
void testCase8_ErrorHandling_AlreadyExists()
{
    std::cout << "\n=== Test Case 8: Error handling - already exists ===" << std::endl;
    
    // 首先创建一个共享内存
    auto createResult = PosixSharedMemoryBuilder()
        .name("test_shm_exclusive")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!createResult)
    {
        std::cerr << "❌ Failed to create initial shared memory" << std::endl;
        return;
    }
    
    std::cout << "✅ Created initial shared memory" << std::endl;
    
    // 尝试用 ExclusiveCreate 模式再次创建（应该失败）
    auto exclusiveResult = PosixSharedMemoryBuilder()
        .name("test_shm_exclusive")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::ExclusiveCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!exclusiveResult)
    {
        if (exclusiveResult.error() == PosixSharedMemoryError::DOES_EXIST)
        {
            std::cout << "✅ Correctly detected 'already exists' error" << std::endl;
        }
        else
        {
            std::cerr << "⚠️  Failed with different error: " 
                      << static_cast<int>(exclusiveResult.error()) << std::endl;
        }
    }
    else
    {
        std::cerr << "❌ Should have failed due to existing shared memory!" << std::endl;
    }
}

// 测试用例9: 移动语义
void testCase9_MoveSemantics()
{
    std::cout << "\n=== Test Case 9: Move semantics ===" << std::endl;
    
    auto result = PosixSharedMemoryBuilder()
        .name("test_shm_move")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)//进程访问模式
        .openMode(OpenMode::PurgeAndCreate)//文件打开模式
        .filePermissions(Perms::OwnerAll)//系统级权限
        .create();
    
    if (!result)
    {
        std::cerr << "❌ Failed to create shared memory" << std::endl;
        return;
    }
    
    auto shm1 = std::move(result.value());
    int handle1 = shm1.getHandle();
    
    std::cout << "✅ Created shm1 with handle: " << handle1 << std::endl;
    
    // 移动构造
    auto shm2 = std::move(shm1);
    int handle2 = shm2.getHandle();
    
    std::cout << "✅ Moved to shm2 with handle: " << handle2 << std::endl;
    
    if (handle1 == handle2)
    {
        std::cout << "✅ Move semantics working correctly" << std::endl;
    }
    else
    {
        std::cerr << "❌ Move semantics failed: handles don't match" << std::endl;
    }
}

// 测试用例10: 不同文件权限
void testCase10_FilePermissions()
{
    std::cout << "\n=== Test Case 10: Different file permissions ===" << std::endl;
    
    // 所有者全部权限
    auto ownerAllResult = PosixSharedMemoryBuilder()
        .name("test_shm_perm_owner_all")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (ownerAllResult)
    {
        std::cout << "✅ Created with OwnerAll permissions (0700)" << std::endl;
    }
    
    // 所有者读写
    auto ownerRWResult = PosixSharedMemoryBuilder()
        .name("test_shm_perm_owner_rw")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerRead | Perms::OwnerWrite)
        .create();
    
    if (ownerRWResult)
    {
        std::cout << "✅ Created with OwnerRead|OwnerWrite permissions (0600)" << std::endl;
    }
    
    // 全部权限
    auto allPermsResult = PosixSharedMemoryBuilder()
        .name("test_shm_perm_all")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::All)
        .create();
    
    if (allPermsResult)
    {
        std::cout << "✅ Created with All permissions (0777)" << std::endl;
    }
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  PosixSharedMemory Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try
    {
        testCase1_CreateNew();
        testCase2_OpenExisting();
        testCase3_OpenOrCreate();
        testCase4_MemoryMapAndReadWrite();
        testCase5_AccessModes();
        testCase6_DifferentSizes();
        testCase7_ErrorHandling_EmptyName();
        testCase8_ErrorHandling_AlreadyExists();
        testCase9_MoveSemantics();
        testCase10_FilePermissions();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  All tests completed!" << std::endl;
        std::cout << "========================================" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ Exception caught: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

