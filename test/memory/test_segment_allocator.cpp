/**
 * @file test_segment_allocator.cpp
 * @brief 测试段分配器的完整功能
 */

#include "zerocp_daemon/memory/include/segment_allocator.hpp"
#include "zerocp_daemon/memory/include/segmentconfig.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <iostream>
#include <vector>

using namespace ZeroCP;
using namespace ZeroCP::Memory;
using namespace ZeroCP::Daemon;

/**
 * @brief 创建测试配置
 */
SegmentConfig createTestConfig()
{
    SegmentConfig config;
    
    // 创建第一个段：包含2个内存池
    SegmentEntry segment1;
    segment1.segment_id = 1;
    
    // 内存池1：10个 1KB 的chunks
    MemoryPoolConfig pool1;
    pool1.pool_id = 1;
    pool1.chunk_count = 10;
    pool1.chunk_size = 1024;  // 1KB
    
    // 内存池2：20个 4KB 的chunks
    MemoryPoolConfig pool2;
    pool2.pool_id = 2;
    pool2.chunk_count = 20;
    pool2.chunk_size = 4096;  // 4KB
    
    segment1.memory_pools.push_back(pool1);
    segment1.memory_pools.push_back(pool2);
    
    // 创建第二个段：包含1个大内存池
    SegmentEntry segment2;
    segment2.segment_id = 2;
    
    // 内存池3：5个 64KB 的chunks
    MemoryPoolConfig pool3;
    pool3.pool_id = 3;
    pool3.chunk_count = 5;
    pool3.chunk_size = 65536;  // 64KB
    
    segment2.memory_pools.push_back(pool3);
    
    config.segment_entries.push_back(segment1);
    config.segment_entries.push_back(segment2);
    
    return config;
}

/**
 * @brief 测试基本的段分配功能
 */
void test_basic_allocation()
{
    LogInfo() << "========================================";
    LogInfo() << "Test: Basic Segment Allocation";
    LogInfo() << "========================================";
    
    // 创建配置
    SegmentConfig config = createTestConfig();
    
    // 创建段分配器
    SegmentAllocator allocator(config);
    
    // 执行分配
    allocator.allocateSegments();
    
    LogInfo() << "[TEST] Basic allocation completed successfully";
}

/**
 * @brief 测试空配置
 */
void test_empty_config()
{
    LogInfo() << "========================================";
    LogInfo() << "Test: Empty Configuration";
    LogInfo() << "========================================";
    
    SegmentConfig config;  // 空配置
    
    SegmentAllocator allocator(config);
    allocator.allocateSegments();
    
    LogInfo() << "[TEST] Empty config handled successfully";
}

/**
 * @brief 测试单个段单个池
 */
void test_single_segment_single_pool()
{
    LogInfo() << "========================================";
    LogInfo() << "Test: Single Segment Single Pool";
    LogInfo() << "========================================";
    
    SegmentConfig config;
    
    SegmentEntry segment;
    segment.segment_id = 100;
    
    MemoryPoolConfig pool;
    pool.pool_id = 1;
    pool.chunk_count = 100;
    pool.chunk_size = 512;
    
    segment.memory_pools.push_back(pool);
    config.segment_entries.push_back(segment);
    
    SegmentAllocator allocator(config);
    allocator.allocateSegments();
    
    LogInfo() << "[TEST] Single segment/pool allocation completed successfully";
}

/**
 * @brief 主函数
 */
int main(int argc, char** argv)
{
    LogInfo() << "========================================";
    LogInfo() << "Starting Segment Allocator Tests";
    LogInfo() << "========================================\n";
    
    try
    {
        // 运行测试
        test_basic_allocation();
        std::cout << "\n";
        
        test_empty_config();
        std::cout << "\n";
        
        test_single_segment_single_pool();
        std::cout << "\n";
        
        LogInfo() << "========================================";
        LogInfo() << "All Tests Passed!";
        LogInfo() << "========================================";
        
        return 0;
    }
    catch (const std::exception& e)
    {
        LogError() << "Test failed with exception: " << e.what();
        return 1;
    }
}


