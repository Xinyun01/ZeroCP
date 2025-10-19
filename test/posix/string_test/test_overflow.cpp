#include "string.hpp"
#include "logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace ZeroCP;

int main()
{
    // 初始化日志系统
    Log::Log_Manager::getInstance().start();
    Log::Log_Manager::getInstance().setLogLevel(Log::LogLevel::Debug);
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         String Capacity Overflow Test with Logging          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // 测试1：运行时溢出（使用 const char*）
    std::cout << "Test 1: Runtime overflow with const char*" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;
    string<10> str1;
    str1.insert(0, "12345");
    std::cout << "After first insert: \"" << str1.c_str() << "\" (size: " << str1.size() << ")" << std::endl;
    
    str1.insert(5, "67890");
    std::cout << "After second insert: \"" << str1.c_str() << "\" (size: " << str1.size() << ")" << std::endl;
    
    // 这个操作会失败并记录日志
    std::cout << "\nAttempting to insert 'X' (should trigger overflow log)..." << std::endl;
    const char* overflow_str = "X";
    str1.insert(10, overflow_str);
    std::cout << "After overflow attempt: \"" << str1.c_str() << "\" (size: " << str1.size() << ")" << std::endl;
    
    // 测试2：运行时溢出（使用字符串字面量）
    std::cout << "\n\nTest 2: Runtime overflow with string literal" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
    string<15> str2;
    str2.insert(0, "Hello ");
    std::cout << "After first insert: \"" << str2.c_str() << "\" (size: " << str2.size() << ")" << std::endl;
    
    str2.insert(6, "World");
    std::cout << "After second insert: \"" << str2.c_str() << "\" (size: " << str2.size() << ")" << std::endl;
    
    // 这个操作会失败并记录日志
    std::cout << "\nAttempting to insert ' Again' (should trigger overflow log)..." << std::endl;
    str2.insert(11, " Again");
    std::cout << "After overflow attempt: \"" << str2.c_str() << "\" (size: " << str2.size() << ")" << std::endl;
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                   Tests Completed Successfully               ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::cout << "\nWaiting for log output to flush..." << std::endl;
    // 等待一下让日志输出完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 停止日志系统
    Log::Log_Manager::getInstance().stop();
    
    return 0;
}

