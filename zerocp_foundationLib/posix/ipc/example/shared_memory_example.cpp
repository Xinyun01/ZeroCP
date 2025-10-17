#include "posix_sharedmemory_object.hpp"
#include "filesystemInterface.hpp"
#include <iostream>
#include <cstring>

using namespace ZeroCP::Details;

int main() {
    try {
        // 创建共享内存对象
        PosixSharedMemoryObjectBuilder builder;
        
        // 使用Builder模式设置参数
        auto result = std::move(builder)
            .name("test_shared_memory")
            .memorySize(4096)  // 4KB
            .accessMode(AccessMode::ReadWrite)
            .openMode(OpenMode::OpenOrCreate)
            .permissions(Perms::OwnerRead | Perms::OwnerWrite)
            .create();
        
        if (!result) {
            std::cerr << "Failed to create shared memory object: " << static_cast<int>(result.error()) << std::endl;
            return 1;
        }
        
        auto sharedMemoryObject = std::move(result.value());
        
        // 使用接口方法
        std::cout << "Memory size: " << sharedMemoryObject.getMemorySize() << " bytes" << std::endl;
        
        void* baseMemory = sharedMemoryObject.getBaseMemory();
        if (baseMemory) {
            std::cout << "Base memory address: " << baseMemory << std::endl;
            
            // 写入一些数据
            const char* testData = "Hello, Shared Memory!";
            std::memcpy(baseMemory, testData, std::strlen(testData));
            
            // 读取数据
            char readBuffer[256];
            std::memcpy(readBuffer, baseMemory, std::strlen(testData));
            readBuffer[std::strlen(testData)] = '\0';
            
            std::cout << "Data written and read: " << readBuffer << std::endl;
        }
        
        if (sharedMemoryObject.isValid()) {
            std::cout << "Shared memory object is valid" << std::endl;
        } else {
            std::cout << "Shared memory object is invalid" << std::endl;
        }
        
        // 测试接口继承
        const IMemorySize& memoryInterface = sharedMemoryObject;
        std::cout << "Memory size via interface: " << memoryInterface.getMemorySize() << " bytes" << std::endl;
        
        const ISharedMemory& sharedMemoryInterface = sharedMemoryObject;
        std::cout << "Is valid via interface: " << (sharedMemoryInterface.isValid() ? "true" : "false") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


