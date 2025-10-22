# RelativePointer 使用指南

## 📋 概述

`RelativePointer` 是一个用于共享内存的智能指针类，它存储**相对偏移量**而不是绝对地址，使得不同进程可以安全地共享指针。

## 🎯 核心概念

### 为什么需要 RelativePointer？

在共享内存场景中：
```
进程A: 共享内存映射到虚拟地址 0x7f1234567000
进程B: 相同共享内存映射到虚拟地址 0x7f9876543000
```

如果进程A在共享内存中存储了一个**绝对指针** `0x7f1234567100`，进程B读取后会得到错误的地址！

**解决方案：** 使用相对偏移量
```
相对偏移 = 指针地址 - 共享内存基地址
例如：offset = 0x7f1234567100 - 0x7f1234567000 = 0x100
```

## 🔧 基本使用

### 1️⃣ 注册共享内存段

在创建共享内存后，需要注册段ID和基地址的映射：

```cpp
#include "zerocp_foundationLib/posix/memory/include/relative_pointer.hpp"

// 创建共享内存
auto shmResult = PosixSharedMemoryObjectBuilder()
    .setName("/my_shm")
    .setMemorySize(1024 * 1024)
    .create();

if (shmResult.has_value())
{
    void* baseAddress = shmResult->getBaseAddress();
    uint64_t segmentId = 42; // 你的段ID（可以从配置或协商中获取）
    
    // 注册到全局注册表
    ZeroCP::SegmentRegistry::instance().registerSegment(segmentId, baseAddress);
}
```

### 2️⃣ 创建 RelativePointer

**方式1：从原生指针创建**
```cpp
struct MyData {
    int value;
    char name[32];
};

// 假设 dataPtr 指向共享内存中的某个位置
MyData* dataPtr = static_cast<MyData*>(baseAddress) + 10;

// 创建相对指针
ZeroCP::RelativePointer<MyData> relPtr(dataPtr, segmentId);

// relPtr 现在存储的是偏移量，可以安全地在共享内存中传递
```

**方式2：从偏移量创建**
```cpp
// 如果你已经知道偏移量（例如从共享内存中读取）
uint64_t offset = 1024;
ZeroCP::RelativePointer<MyData> relPtr(offset, segmentId);
```

### 3️⃣ 使用 RelativePointer

```cpp
// 获取原生指针
MyData* ptr = relPtr.get();

// 解引用
MyData& data = *relPtr;
data.value = 42;

// 成员访问
relPtr->value = 100;
strcpy(relPtr->name, "Hello");

// 检查是否为空
if (relPtr) {
    std::cout << "Valid pointer" << std::endl;
}

// 获取段ID和偏移量
uint64_t id = relPtr.get_segment_id();
uint64_t offset = relPtr.get_offset();
```

### 4️⃣ 在共享内存中存储

```cpp
struct SharedHeader {
    ZeroCP::RelativePointer<MyData> dataPtr;
    ZeroCP::RelativePointer<char> namePtr;
    uint64_t count;
};

// 在共享内存中写入
SharedHeader* header = static_cast<SharedHeader*>(baseAddress);
MyData* data = static_cast<MyData*>(baseAddress) + sizeof(SharedHeader);

header->dataPtr = ZeroCP::RelativePointer<MyData>(data, segmentId);
header->count = 10;
```

### 5️⃣ 从其他进程读取

```cpp
// 进程B：打开相同的共享内存
auto shmResult = PosixSharedMemoryObjectBuilder()
    .setName("/my_shm")
    .setAccessMode(AccessMode::ReadWrite)
    .setOpenMode(OpenMode::OpenExisting)
    .create();

void* baseAddressB = shmResult->getBaseAddress();

// 注册（注意：基地址可能不同！）
ZeroCP::SegmentRegistry::instance().registerSegment(segmentId, baseAddressB);

// 读取共享内存
SharedHeader* header = static_cast<SharedHeader*>(baseAddressB);

// RelativePointer 自动转换为当前进程的虚拟地址
MyData* data = header->dataPtr.get();
std::cout << "Value: " << data->value << std::endl;
```

## 🔒 清理资源

```cpp
// 在销毁共享内存前，取消注册
ZeroCP::SegmentRegistry::instance().unregisterSegment(segmentId);
```

## ⚠️ 注意事项

1. **线程安全**：`SegmentRegistry` 是线程安全的，但 `RelativePointer` 本身不是
2. **段ID唯一性**：确保每个共享内存段有唯一的ID
3. **注册时机**：必须在使用 `RelativePointer::get()` 之前注册段
4. **生命周期**：确保共享内存在使用 RelativePointer 期间保持有效

## 📊 完整示例

```cpp
#include "zerocp_foundationLib/posix/memory/include/relative_pointer.hpp"
#include "zerocp_foundationLib/posix/ipc/include/posix_sharedmemory.hpp"

struct Message {
    int id;
    ZeroCP::RelativePointer<Message> next; // 链表结构
};

int main() {
    // 创建共享内存
    auto shm = PosixSharedMemoryObjectBuilder()
        .setName("/my_messages")
        .setMemorySize(4096)
        .setAccessMode(AccessMode::ReadWrite)
        .setOpenMode(OpenMode::CreateOrOpen)
        .create();
    
    if (!shm.has_value()) {
        return -1;
    }
    
    void* base = shm->getBaseAddress();
    uint64_t segId = 1;
    
    // 注册段
    ZeroCP::SegmentRegistry::instance().registerSegment(segId, base);
    
    // 创建消息链表
    Message* msg1 = static_cast<Message*>(base);
    Message* msg2 = msg1 + 1;
    
    msg1->id = 1;
    msg1->next = ZeroCP::RelativePointer<Message>(msg2, segId);
    
    msg2->id = 2;
    msg2->next = ZeroCP::RelativePointer<Message>(nullptr, segId);
    
    // 遍历链表
    ZeroCP::RelativePointer<Message> current(msg1, segId);
    while (current) {
        std::cout << "Message ID: " << current->id << std::endl;
        current = current->next;
    }
    
    // 清理
    ZeroCP::SegmentRegistry::instance().unregisterSegment(segId);
    
    return 0;
}
```

## 🚀 高级用法

### 多段支持

```cpp
// 段1：元数据
uint64_t metaSegId = 1;
void* metaBase = /* ... */;
ZeroCP::SegmentRegistry::instance().registerSegment(metaSegId, metaBase);

// 段2：数据
uint64_t dataSegId = 2;
void* dataBase = /* ... */;
ZeroCP::SegmentRegistry::instance().registerSegment(dataSegId, dataBase);

// 跨段引用
struct Metadata {
    ZeroCP::RelativePointer<char> dataPtr; // 指向段2
};

Metadata* meta = static_cast<Metadata*>(metaBase);
char* data = static_cast<char*>(dataBase);
meta->dataPtr = ZeroCP::RelativePointer<char>(data, dataSegId);
```

## 📝 总结

RelativePointer 的核心优势：
- ✅ 跨进程安全：不同进程可以正确解引用
- ✅ 零拷贝友好：配合共享内存实现高效数据传输
- ✅ 类型安全：模板提供编译时类型检查
- ✅ 易用性：操作符重载使其像普通指针一样使用

