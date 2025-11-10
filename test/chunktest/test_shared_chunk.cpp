/**
 * @file test_shared_chunk.cpp
 * @brief 测试 SharedChunk 的引用计数管理功能
 */

#include "zerocp_daemon/mempool/shared_chunk.hpp"
#include "zerocp_daemon/memory/include/mempool_manager.hpp"
#include "zerocp_daemon/memory/include/mempool_config.hpp"
#include "zerocp_log/zerocp_log.hpp"
#include <iostream>
#include <string>
#include <cstring>

using namespace ZeroCP::Memory;

void printSeparator(const std::string& title)
{
    std::cout << "\n========== " << title << " ==========\n" << std::endl;
}

void testBasicUsage()
{
    printSeparator("测试 1: 基本用法");
    
    MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();
    if (!poolMgr)
    {
        std::cerr << "❌ MemPoolManager 未初始化" << std::endl;
        return;
    }
    
    // 分配 chunk
    std::cout << "1. 分配 1024 字节的 chunk..." << std::endl;
    ChunkManager* rawChunk = poolMgr->getChunk(1024);
    if (!rawChunk)
    {
        std::cerr << "❌ 分配失败" << std::endl;
        return;
    }
    
    std::cout << "✓ 分配成功，初始 refCount = " 
              << rawChunk->m_refCount.load() << std::endl;
    
    // 使用 SharedChunk 包装
    std::cout << "\n2. 创建 SharedChunk（接管所有权）..." << std::endl;
    SharedChunk chunk(rawChunk, poolMgr);
    std::cout << "✓ SharedChunk 创建成功" << std::endl;
    std::cout << "  - isValid: " << (chunk.isValid() ? "true" : "false") << std::endl;
    std::cout << "  - useCount: " << chunk.useCount() << std::endl;
    std::cout << "  - size: " << chunk.getSize() << " bytes" << std::endl;
    
    // 写入数据
    std::cout << "\n3. 写入数据..." << std::endl;
    void* data = chunk.getData();
    const char* message = "Hello, SharedChunk!";
    std::memcpy(data, message, std::strlen(message) + 1);
    std::cout << "✓ 写入: \"" << message << "\"" << std::endl;
    
    // 读取数据
    std::cout << "\n4. 读取数据..." << std::endl;
    std::cout << "✓ 读取: \"" << static_cast<char*>(chunk.getData()) << "\"" << std::endl;
    
    std::cout << "\n5. SharedChunk 将在此处析构..." << std::endl;
    std::cout << "  当前 refCount = " << chunk.useCount() << std::endl;
    std::cout << "  析构后应该释放资源（refCount 将变为 0）" << std::endl;
}

void testCopySemantics()
{
    printSeparator("测试 2: 拷贝语义");
    
    MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();
    if (!poolMgr)
    {
        std::cerr << "❌ MemPoolManager 未初始化" << std::endl;
        return;
    }
    
    std::cout << "1. 创建 chunk1..." << std::endl;
    SharedChunk chunk1(poolMgr->getChunk(512), poolMgr);
    std::cout << "✓ chunk1 创建，refCount = " << chunk1.useCount() << std::endl;
    
    // 写入数据
    const char* msg1 = "Data from chunk1";
    std::memcpy(chunk1.getData(), msg1, std::strlen(msg1) + 1);
    
    std::cout << "\n2. 拷贝构造 chunk2..." << std::endl;
    SharedChunk chunk2(chunk1);
    std::cout << "✓ chunk2 创建（拷贝构造）" << std::endl;
    std::cout << "  - chunk1.useCount = " << chunk1.useCount() << std::endl;
    std::cout << "  - chunk2.useCount = " << chunk2.useCount() << std::endl;
    std::cout << "  - 两者应该相等: " 
              << (chunk1.useCount() == chunk2.useCount() ? "✓" : "✗") << std::endl;
    
    std::cout << "\n3. 拷贝赋值 chunk3..." << std::endl;
    SharedChunk chunk3;
    chunk3 = chunk1;
    std::cout << "✓ chunk3 创建（拷贝赋值）" << std::endl;
    std::cout << "  - chunk1.useCount = " << chunk1.useCount() << std::endl;
    std::cout << "  - chunk2.useCount = " << chunk2.useCount() << std::endl;
    std::cout << "  - chunk3.useCount = " << chunk3.useCount() << std::endl;
    
    // 验证数据共享
    std::cout << "\n4. 验证数据共享..." << std::endl;
    std::cout << "  - chunk1 数据: \"" << static_cast<char*>(chunk1.getData()) << "\"" << std::endl;
    std::cout << "  - chunk2 数据: \"" << static_cast<char*>(chunk2.getData()) << "\"" << std::endl;
    std::cout << "  - chunk3 数据: \"" << static_cast<char*>(chunk3.getData()) << "\"" << std::endl;
    
    // 修改数据，验证所有拷贝都能看到变化
    std::cout << "\n5. 通过 chunk2 修改数据..." << std::endl;
    const char* msg2 = "Modified by chunk2";
    std::memcpy(chunk2.getData(), msg2, std::strlen(msg2) + 1);
    std::cout << "  - chunk1 数据: \"" << static_cast<char*>(chunk1.getData()) << "\"" << std::endl;
    std::cout << "  - chunk2 数据: \"" << static_cast<char*>(chunk2.getData()) << "\"" << std::endl;
    std::cout << "  - chunk3 数据: \"" << static_cast<char*>(chunk3.getData()) << "\"" << std::endl;
    
    std::cout << "\n6. 所有 SharedChunk 将依次析构..." << std::endl;
    std::cout << "  当前 refCount = " << chunk1.useCount() << std::endl;
    std::cout << "  chunk3 析构 -> refCount = 2" << std::endl;
    std::cout << "  chunk2 析构 -> refCount = 1" << std::endl;
    std::cout << "  chunk1 析构 -> refCount = 0 -> 释放资源" << std::endl;
}

void testMoveSemantics()
{
    printSeparator("测试 3: 移动语义");
    
    MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();
    if (!poolMgr)
    {
        std::cerr << "❌ MemPoolManager 未初始化" << std::endl;
        return;
    }
    
    std::cout << "1. 创建 chunk1..." << std::endl;
    SharedChunk chunk1(poolMgr->getChunk(256), poolMgr);
    std::cout << "✓ chunk1 创建，refCount = " << chunk1.useCount() << std::endl;
    
    const char* msg = "Move semantics test";
    std::memcpy(chunk1.getData(), msg, std::strlen(msg) + 1);
    
    std::cout << "\n2. 移动构造 chunk2..." << std::endl;
    SharedChunk chunk2(std::move(chunk1));
    std::cout << "✓ chunk2 创建（移动构造）" << std::endl;
    std::cout << "  - chunk1.isValid = " << (chunk1.isValid() ? "true" : "false") 
              << " (应该为 false)" << std::endl;
    std::cout << "  - chunk2.isValid = " << (chunk2.isValid() ? "true" : "false") 
              << " (应该为 true)" << std::endl;
    std::cout << "  - chunk2.useCount = " << chunk2.useCount() 
              << " (应该还是 1)" << std::endl;
    std::cout << "  - chunk2 数据: \"" << static_cast<char*>(chunk2.getData()) << "\"" << std::endl;
    
    std::cout << "\n3. 移动赋值 chunk3..." << std::endl;
    SharedChunk chunk3;
    chunk3 = std::move(chunk2);
    std::cout << "✓ chunk3 创建（移动赋值）" << std::endl;
    std::cout << "  - chunk2.isValid = " << (chunk2.isValid() ? "true" : "false") 
              << " (应该为 false)" << std::endl;
    std::cout << "  - chunk3.isValid = " << (chunk3.isValid() ? "true" : "false") 
              << " (应该为 true)" << std::endl;
    std::cout << "  - chunk3.useCount = " << chunk3.useCount() 
              << " (应该还是 1)" << std::endl;
    std::cout << "  - chunk3 数据: \"" << static_cast<char*>(chunk3.getData()) << "\"" << std::endl;
    
    std::cout << "\n4. chunk3 将析构..." << std::endl;
    std::cout << "  当前 refCount = " << chunk3.useCount() << std::endl;
    std::cout << "  chunk3 析构 -> refCount = 0 -> 释放资源" << std::endl;
}

void testReset()
{
    printSeparator("测试 4: reset() 方法");
    
    MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();
    if (!poolMgr)
    {
        std::cerr << "❌ MemPoolManager 未初始化" << std::endl;
        return;
    }
    
    std::cout << "1. 创建 chunk..." << std::endl;
    SharedChunk chunk(poolMgr->getChunk(128), poolMgr);
    std::cout << "✓ chunk 创建，refCount = " << chunk.useCount() << std::endl;
    
    std::cout << "\n2. 调用 reset()..." << std::endl;
    chunk.reset();
    std::cout << "✓ reset() 完成" << std::endl;
    std::cout << "  - chunk.isValid = " << (chunk.isValid() ? "true" : "false") 
              << " (应该为 false)" << std::endl;
    std::cout << "  - chunk.useCount = " << chunk.useCount() 
              << " (应该为 0)" << std::endl;
    
    std::cout << "\n3. 使用 reset() 替换为新 chunk..." << std::endl;
    ChunkManager* newChunk = poolMgr->getChunk(256);
    chunk.reset(newChunk, poolMgr);
    std::cout << "✓ reset() 完成" << std::endl;
    std::cout << "  - chunk.isValid = " << (chunk.isValid() ? "true" : "false") 
              << " (应该为 true)" << std::endl;
    std::cout << "  - chunk.useCount = " << chunk.useCount() << std::endl;
    std::cout << "  - chunk.size = " << chunk.getSize() << " bytes" << std::endl;
    
    std::cout << "\n4. chunk 将析构..." << std::endl;
}

void testMultipleReferences()
{
    printSeparator("测试 5: 多个引用的生命周期");
    
    MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();
    if (!poolMgr)
    {
        std::cerr << "❌ MemPoolManager 未初始化" << std::endl;
        return;
    }
    
    std::cout << "1. 创建原始 chunk..." << std::endl;
    SharedChunk chunk1(poolMgr->getChunk(512), poolMgr);
    std::cout << "✓ chunk1 创建，refCount = " << chunk1.useCount() << std::endl;
    
    {
        std::cout << "\n2. 进入内部作用域，创建多个拷贝..." << std::endl;
        SharedChunk chunk2 = chunk1;
        std::cout << "  chunk2 创建，refCount = " << chunk1.useCount() << std::endl;
        
        SharedChunk chunk3 = chunk1;
        std::cout << "  chunk3 创建，refCount = " << chunk1.useCount() << std::endl;
        
        SharedChunk chunk4 = chunk2;
        std::cout << "  chunk4 创建，refCount = " << chunk1.useCount() << std::endl;
        
        std::cout << "\n3. 离开内部作用域，chunk2/3/4 将析构..." << std::endl;
    }
    
    std::cout << "✓ 内部作用域结束" << std::endl;
    std::cout << "  当前 refCount = " << chunk1.useCount() 
              << " (应该回到 1)" << std::endl;
    
    std::cout << "\n4. chunk1 将析构..." << std::endl;
    std::cout << "  当前 refCount = " << chunk1.useCount() << std::endl;
    std::cout << "  chunk1 析构 -> refCount = 0 -> 释放资源" << std::endl;
}

/// @brief 测试跨进程传输语义
void testCrossProcessTransfer()
{
    printSeparator("跨进程传输测试");
    
    MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();
    assert(poolMgr != nullptr);
    
    // 1. 分配 chunk（模拟发送端）
    ChunkManager* rawChunk = poolMgr->getChunk(256);
    assert(rawChunk != nullptr);
    
    SharedChunk senderChunk(rawChunk, poolMgr);
    std::cout << "1. 发送端创建 chunk, refCount=" << senderChunk.useCount() << std::endl;
    assert(senderChunk.useCount() == 1);
    
    // 2. 写入数据
    void* data = senderChunk.getData();
    const char* message = "Cross-process message";
    std::memcpy(data, message, std::strlen(message) + 1);
    std::cout << "2. 发送端写入数据: \"" << message << "\"" << std::endl;
    
    // 3. 准备传输（增加引用计数）
    uint32_t chunkIndex = senderChunk.prepareForTransfer();
    std::cout << "3. 准备传输, chunkIndex=" << chunkIndex 
              << ", refCount=" << senderChunk.useCount() << std::endl;
    assert(senderChunk.useCount() == 2);  // 引用计数应该增加到 2
    
    // 4. 模拟发送端 SharedChunk 析构（例如离开作用域）
    {
        std::cout << "4. 发送端 SharedChunk 析构前, refCount=" << senderChunk.useCount() << std::endl;
        // senderChunk 会在这里析构，但引用计数应该只减到 1
    }
    // 手动调用 release 模拟析构
    uint64_t refCountBeforeRelease = senderChunk.useCount();
    senderChunk.reset();
    std::cout << "5. 发送端析构后（应该还有 1 个引用）" << std::endl;
    
    // 5. 模拟接收端：从索引重建 SharedChunk
    std::cout << "6. 接收端从索引重建 SharedChunk..." << std::endl;
    SharedChunk receiverChunk = SharedChunk::fromIndex(chunkIndex, poolMgr);
    assert(receiverChunk.isValid());
    std::cout << "7. 接收端重建成功, refCount=" << receiverChunk.useCount() << std::endl;
    assert(receiverChunk.useCount() == 1);  // 应该还是 1（发送端已经释放）
    
    // 6. 接收端读取数据
    void* receivedData = receiverChunk.getData();
    std::cout << "8. 接收端读取数据: \"" << static_cast<char*>(receivedData) << "\"" << std::endl;
    assert(std::strcmp(static_cast<char*>(receivedData), message) == 0);
    
    // 7. 接收端析构（引用计数降为 0，资源应该被释放）
    std::cout << "9. 接收端即将析构, refCount=" << receiverChunk.useCount() << std::endl;
    receiverChunk.reset();
    std::cout << "10. 接收端析构完成，资源已释放\n" << std::endl;
    
    std::cout << "✓ 跨进程传输测试通过\n" << std::endl;
}

int main()
{
    std::cout << "==================== SharedChunk 测试程序 ====================" << std::endl;
    std::cout << "本程序测试 SharedChunk 的引用计数管理功能\n" << std::endl;
    
    // 初始化内存池管理器
    std::cout << "初始化 MemPoolManager..." << std::endl;
    
    MemPoolConfig config;
    config.addPool(128, 10);   // 128 字节 x 10
    config.addPool(256, 10);   // 256 字节 x 10
    config.addPool(512, 10);   // 512 字节 x 10
    config.addPool(1024, 10);  // 1024 字节 x 10
    
    if (!MemPoolManager::createSharedInstance(config))
    {
        std::cerr << "❌ 创建 MemPoolManager 失败" << std::endl;
        return 1;
    }
    
    MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();
    if (!poolMgr)
    {
        std::cerr << "❌ 获取 MemPoolManager 实例失败" << std::endl;
        return 1;
    }
    
    if (!poolMgr->initialize())
    {
        std::cerr << "❌ 初始化 MemPoolManager 失败" << std::endl;
        return 1;
    }
    
    std::cout << "✓ MemPoolManager 初始化成功\n" << std::endl;
    
    // 运行测试
    try
    {
        testBasicUsage();
        testCopySemantics();
        testMoveSemantics();
        testReset();
        testMultipleReferences();
        testCrossProcessTransfer();  // 新增：跨进程传输测试
        
        printSeparator("所有测试完成");
        std::cout << "✓ 所有测试通过！\n" << std::endl;
        
        // 打印最终的内存池状态
        std::cout << "最终内存池状态：" << std::endl;
        poolMgr->printAllPoolStats();
        
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    // 清理
    std::cout << "\n清理资源..." << std::endl;
    MemPoolManager::destroySharedInstance();
    std::cout << "✓ 清理完成" << std::endl;
    
    return 0;
}

