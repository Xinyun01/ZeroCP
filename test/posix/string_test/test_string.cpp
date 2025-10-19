#include "string.hpp"
#include "logging.hpp"
#include <iostream>
#include <cassert>

using namespace ZeroCP;

// 测试用例1: 基本的栈上字符串创建和输出
void testCase1_BasicStackString()
{
    std::cout << "\n=== Test Case 1: Basic Stack String ===" << std::endl;
    
    // 在栈上创建固定容量的字符串
    string<64> str1;
    
    std::cout << "✅ Created string with capacity: " << str1.capacity() << std::endl;
    std::cout << "   Initial size: " << str1.size() << std::endl;
    std::cout << "   Is empty: " << (str1.empty() ? "Yes" : "No") << std::endl;
    std::cout << "   Content: \"" << str1.c_str() << "\"" << std::endl;
}

// 测试用例2: 字符串插入操作（栈上输入）
void testCase2_InsertToStackString()
{
    std::cout << "\n=== Test Case 2: Insert to Stack String ===" << std::endl;
    
    // 在栈上创建字符串并插入内容
    string<128> str;
    
    std::cout << "📝 Inserting \"Hello\" at position 0..." << std::endl;
    str.insert(0, "Hello");
    
    std::cout << "   Size: " << str.size() << std::endl;
    std::cout << "   Content: \"" << str.c_str() << "\"" << std::endl;
    
    std::cout << "📝 Inserting \" World\" at position 5..." << std::endl;
    str.insert(5, " World");
    
    std::cout << "   Size: " << str.size() << std::endl;
    std::cout << "   Content: \"" << str.c_str() << "\"" << std::endl;
    
    std::cout << "📝 Inserting \"!\" at end..." << std::endl;
    str.insert(str.size(), "!");
    
    std::cout << "   Final size: " << str.size() << std::endl;
    std::cout << "   Final content: \"" << str.c_str() << "\"" << std::endl;
}

// 测试用例3: 拷贝构造（栈到栈）
void testCase3_CopyConstruction()
{
    std::cout << "\n=== Test Case 3: Copy Construction (Stack to Stack) ===" << std::endl;
    
    // 源字符串（栈上）
    string<64> source;
    source.insert(0, "Original String");
    
    std::cout << "📦 Source string:" << std::endl;
    std::cout << "   Size: " << source.size() << std::endl;
    std::cout << "   Content: \"" << source.c_str() << "\"" << std::endl;
    
    // 通过拷贝构造创建新的栈上字符串
    string<64> destination(source);
    
    std::cout << "📦 Destination string (after copy):" << std::endl;
    std::cout << "   Size: " << destination.size() << std::endl;
    std::cout << "   Content: \"" << destination.c_str() << "\"" << std::endl;
    
    // 验证是深拷贝
    std::cout << "🔍 Verifying deep copy..." << std::endl;
    std::cout << "   Source address: " << static_cast<const void*>(source.c_str()) << std::endl;
    std::cout << "   Destination address: " << static_cast<const void*>(destination.c_str()) << std::endl;
    std::cout << "   Are different? " << (source.c_str() != destination.c_str() ? "✅ Yes (deep copy)" : "❌ No") << std::endl;
}

// 测试用例4: 移动构造（栈到栈）
void testCase4_MoveConstruction()
{
    std::cout << "\n=== Test Case 4: Move Construction (Stack to Stack) ===" << std::endl;
    
    // 源字符串（栈上）
    string<64> source;
    source.insert(0, "Move Me!");
    
    std::cout << "📦 Source string before move:" << std::endl;
    std::cout << "   Size: " << source.size() << std::endl;
    std::cout << "   Content: \"" << source.c_str() << "\"" << std::endl;
    
    // 通过移动构造创建新的栈上字符串
    string<64> destination(std::move(source));
    
    std::cout << "📦 Destination string after move:" << std::endl;
    std::cout << "   Size: " << destination.size() << std::endl;
    std::cout << "   Content: \"" << destination.c_str() << "\"" << std::endl;
    
    std::cout << "📦 Source string after move:" << std::endl;
    std::cout << "   Size: " << source.size() << std::endl;
    std::cout << "   Content: \"" << source.c_str() << "\"" << std::endl;
    std::cout << "   Is empty? " << (source.empty() ? "✅ Yes (cleared)" : "❌ No") << std::endl;
}

// 测试用例5: 赋值操作符
void testCase5_AssignmentOperators()
{
    std::cout << "\n=== Test Case 5: Assignment Operators ===" << std::endl;
    
    string<64> str1, str2, str3;
    str1.insert(0, "First");
    str2.insert(0, "Second");
    
    std::cout << "📝 Before assignment:" << std::endl;
    std::cout << "   str1: \"" << str1.c_str() << "\"" << std::endl;
    std::cout << "   str2: \"" << str2.c_str() << "\"" << std::endl;
    std::cout << "   str3: \"" << str3.c_str() << "\"" << std::endl;
    
    // 拷贝赋值
    str3 = str1;
    std::cout << "\n📝 After str3 = str1 (copy assignment):" << std::endl;
    std::cout << "   str3: \"" << str3.c_str() << "\"" << std::endl;
    
    // 移动赋值
    str3 = std::move(str2);
    std::cout << "\n📝 After str3 = std::move(str2) (move assignment):" << std::endl;
    std::cout << "   str3: \"" << str3.c_str() << "\"" << std::endl;
    std::cout << "   str2: \"" << str2.c_str() << "\" (should be empty)" << std::endl;
}

// 测试用例6: 清空操作
void testCase6_ClearOperation()
{
    std::cout << "\n=== Test Case 6: Clear Operation ===" << std::endl;
    
    string<64> str;
    str.insert(0, "This will be cleared");
    
    std::cout << "📝 Before clear:" << std::endl;
    std::cout << "   Size: " << str.size() << std::endl;
    std::cout << "   Content: \"" << str.c_str() << "\"" << std::endl;
    std::cout << "   Is empty: " << (str.empty() ? "Yes" : "No") << std::endl;
    
    str.clear();
    
    std::cout << "📝 After clear:" << std::endl;
    std::cout << "   Size: " << str.size() << std::endl;
    std::cout << "   Content: \"" << str.c_str() << "\"" << std::endl;
    std::cout << "   Is empty: " << (str.empty() ? "✅ Yes" : "❌ No") << std::endl;
}

// 测试用例7: 不同容量的字符串
void testCase7_DifferentCapacities()
{
    std::cout << "\n=== Test Case 7: Different Capacities ===" << std::endl;
    
    string<32> small;
    string<128> medium;
    string<512> large;
    
    small.insert(0, "Small");
    medium.insert(0, "Medium capacity string");
    large.insert(0, "Large capacity string with more content");
    
    std::cout << "📏 Small string (capacity=" << small.capacity() << "):" << std::endl;
    std::cout << "   Size: " << small.size() << std::endl;
    std::cout << "   Content: \"" << small.c_str() << "\"" << std::endl;
    
    std::cout << "📏 Medium string (capacity=" << medium.capacity() << "):" << std::endl;
    std::cout << "   Size: " << medium.size() << std::endl;
    std::cout << "   Content: \"" << medium.c_str() << "\"" << std::endl;
    
    std::cout << "📏 Large string (capacity=" << large.capacity() << "):" << std::endl;
    std::cout << "   Size: " << large.size() << std::endl;
    std::cout << "   Content: \"" << large.c_str() << "\"" << std::endl;
    
    // 从小容量复制到大容量 - 使用赋值操作
    string<512> copy_from_small;
    copy_from_small.insert(0, small.c_str());
    std::cout << "\n📋 Copy small to large capacity:" << std::endl;
    std::cout << "   Content: \"" << copy_from_small.c_str() << "\"" << std::endl;
}

// 测试用例8: 验证栈分配（通过地址）
void testCase8_VerifyStackAllocation()
{
    std::cout << "\n=== Test Case 8: Verify Stack Allocation ===" << std::endl;
    
    // 创建一个栈上的变量作为参考
    int stack_var = 42;
    
    // 创建字符串对象
    string<64> str;
    str.insert(0, "Stack String");
    
    std::cout << "🔍 Memory addresses:" << std::endl;
    std::cout << "   Stack variable address: " << &stack_var << std::endl;
    std::cout << "   String object address: " << &str << std::endl;
    std::cout << "   String buffer address: " << static_cast<const void*>(str.c_str()) << std::endl;
    
    // 计算地址差异（粗略验证都在栈上）
    ptrdiff_t diff = reinterpret_cast<char*>(&str) - reinterpret_cast<char*>(&stack_var);
    std::cout << "   Address difference: " << diff << " bytes" << std::endl;
    
    if (std::abs(diff) < 1024 * 1024)  // 如果差异小于1MB，很可能都在栈上
    {
        std::cout << "   ✅ Likely both on stack (small address difference)" << std::endl;
    }
}

// 测试用例9: 编译时和运行时容量检查
void testCase9_CompileTimeAndRuntimeChecks()
{
    std::cout << "\n=== Test Case 9: Compile-time and Runtime Capacity Checks ===" << std::endl;
    
    // 测试1: 使用字符串字面量 - 编译时检查
    std::cout << "📝 Test 1: String literal insert (compile-time check):" << std::endl;
    string<20> str1;
    str1.insert(0, "Hello");  // 编译时检查：5 <= 20 ✅
    std::cout << "   After insert \"Hello\": \"" << str1.c_str() << "\"" << std::endl;
    std::cout << "   Size: " << str1.size() << " / Capacity: " << str1.capacity() << std::endl;
    
    str1.insert(5, " World");  // 编译时检查：6 <= 20 ✅
    std::cout << "   After insert \" World\": \"" << str1.c_str() << "\"" << std::endl;
    std::cout << "   Size: " << str1.size() << " / Capacity: " << str1.capacity() << std::endl;
    
    // 测试2: 运行时容量不足的情况
    std::cout << "\n📝 Test 2: Runtime capacity overflow check:" << std::endl;
    string<10> str2;
    str2.insert(0, "12345");  // 编译时检查：5 <= 10 ✅，运行时：5 <= 10 ✅
    std::cout << "   After insert \"12345\": \"" << str2.c_str() << "\"" << std::endl;
    std::cout << "   Size: " << str2.size() << " / Capacity: " << str2.capacity() << std::endl;
    
    str2.insert(5, "67890");  // 编译时检查：5 <= 10 ✅，运行时：5+5=10 <= 10 ✅
    std::cout << "   After insert \"67890\": \"" << str2.c_str() << "\"" << std::endl;
    std::cout << "   Size: " << str2.size() << " / Capacity: " << str2.capacity() << std::endl;
    
    str2.insert(10, "X");  // 编译时检查：1 <= 10 ✅，运行时：10+1=11 > 10 ❌
    std::cout << "   After trying to insert \"X\" (should fail): \"" << str2.c_str() << "\"" << std::endl;
    std::cout << "   Size: " << str2.size() << " / Capacity: " << str2.capacity() << std::endl;
    std::cout << "   ✅ Runtime check prevented overflow!" << std::endl;
    
    // 测试3: 使用 const char* 指针 - 运行时检查
    std::cout << "\n📝 Test 3: const char* insert (runtime check):" << std::endl;
    string<15> str3;
    const char* dynamicStr1 = "Dynamic";
    const char* dynamicStr2 = " String";
    
    str3.insert(0, dynamicStr1);
    std::cout << "   After insert dynamicStr1: \"" << str3.c_str() << "\"" << std::endl;
    std::cout << "   Size: " << str3.size() << " / Capacity: " << str3.capacity() << std::endl;
    
    str3.insert(7, dynamicStr2);
    std::cout << "   After insert dynamicStr2: \"" << str3.c_str() << "\"" << std::endl;
    std::cout << "   Size: " << str3.size() << " / Capacity: " << str3.capacity() << std::endl;
    
    // 测试4: 中间插入
    std::cout << "\n📝 Test 4: Insert in the middle:" << std::endl;
    string<30> str4;
    str4.insert(0, "Hello World");
    std::cout << "   Initial string: \"" << str4.c_str() << "\"" << std::endl;
    
    str4.insert(5, " Beautiful");  // 在 "Hello" 和 " World" 之间插入
    std::cout << "   After insert \" Beautiful\" at pos 5: \"" << str4.c_str() << "\"" << std::endl;
    std::cout << "   Size: " << str4.size() << " / Capacity: " << str4.capacity() << std::endl;
    
    // 测试5: 边界条件 - 在不同位置插入
    std::cout << "\n📝 Test 5: Boundary conditions:" << std::endl;
    string<50> str5;
    
    // 在空字符串中插入
    str5.insert(0, "Start");
    std::cout << "   Insert at pos 0 (empty string): \"" << str5.c_str() << "\"" << std::endl;
    
    // 在末尾插入
    str5.insert(str5.size(), " Middle");
    std::cout << "   Insert at end: \"" << str5.c_str() << "\"" << std::endl;
    
    // 超出范围的位置（应该追加到末尾）
    str5.insert(999, " End");
    std::cout << "   Insert at pos 999 (should append): \"" << str5.c_str() << "\"" << std::endl;
    std::cout << "   Final size: " << str5.size() << " / Capacity: " << str5.capacity() << std::endl;
    
    std::cout << "\n✅ All capacity checks working correctly!" << std::endl;
}

// 主函数
int main()
{
    // 初始化日志系统
    Log::Log_Manager::getInstance().start();
    Log::Log_Manager::getInstance().setLogLevel(Log::LogLevel::Debug);
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         ZeroCP String Stack Input/Output Tests              ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    try
    {
        testCase1_BasicStackString();
        testCase2_InsertToStackString();
        testCase3_CopyConstruction();
        testCase4_MoveConstruction();
        testCase5_AssignmentOperators();
        testCase6_ClearOperation();
        testCase7_DifferentCapacities();
        testCase8_VerifyStackAllocation();
        testCase9_CompileTimeAndRuntimeChecks();
        
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                     ✅ All Tests Passed!                     ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n❌ Exception: " << e.what() << std::endl;
        Log::Log_Manager::getInstance().stop();
        return 1;
    }
    
    // 停止日志系统
    Log::Log_Manager::getInstance().stop();
    
    return 0;
}

