// lockfree_ringbuffer.inl - LockFreeRingBuffer 模板类实现
// 此文件包含所有模板方法的实现，由 lockfree_ringbuffer.hpp 包含
// 注意：命名空间已在 lockfree_ringbuffer.hpp 中声明，此处不再重复声明

// ========== 构造函数 ==========
template<typename T, size_t Size>
LockFreeRingBuffer<T, Size>::LockFreeRingBuffer() noexcept
    : write_index_(0)  // 显式初始化写索引为 0
    , read_index_(0)   // 显式初始化读索引为 0
    , buffer_()        // 显式调用 std::array 的默认构造（会构造所有元素）
{
    // 构造函数体：可以添加额外的初始化逻辑
    // buffer_ 中的每个元素都会调用 T 的默认构造函数
}

// ========== tryPush 实现 ==========
template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::tryPush(const T& item) noexcept
{
    // 1. 读取当前的写索引和读索引
    size_t current_write = write_index_.load(std::memory_order_relaxed);
    size_t current_read = read_index_.load(std::memory_order_acquire);
    
    // 2. 计算下一个写位置
    size_t next_write = (current_write + 1) & (Size - 1);  // 使用位运算取模
    
    // 3. 检查队列是否已满（下一个写位置等于读位置表示满）
    if (next_write == current_read) {
        return false;  // 队列满
    }
    
    // 4. 写入数据
    buffer_[current_write] = item;
    
    // 5. 更新写索引（使用 release 保证写入可见性）
    write_index_.store(next_write, std::memory_order_release);
    
    return true;
}

// ========== tryPop 实现 ==========
template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::tryPop(T& item) noexcept
{
    // 1. 读取当前的读索引和写索引
    size_t current_read = read_index_.load(std::memory_order_relaxed);
    size_t current_write = write_index_.load(std::memory_order_acquire);
    
    // 2. 检查队列是否为空（读索引等于写索引表示空）
    if (current_read == current_write) {
        return false;  // 队列空
    }
    
    // 3. 读取数据
    item = buffer_[current_read];
    
    // 4. 计算下一个读位置
    size_t next_read = (current_read + 1) & (Size - 1);  // 使用位运算取模
    
    // 5. 更新读索引（使用 release 保证读取完成的可见性）
    read_index_.store(next_read, std::memory_order_release);
    
    return true;
}

// ========== isEmpty 实现 ==========
template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::isEmpty() const noexcept
{
    size_t current_read = read_index_.load(std::memory_order_acquire);
    size_t current_write = write_index_.load(std::memory_order_acquire);
    return current_read == current_write;
}

// ========== isFull 实现 ==========
template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::isFull() const noexcept
{
    size_t current_write = write_index_.load(std::memory_order_acquire);
    size_t current_read = read_index_.load(std::memory_order_acquire);
    size_t next_write = (current_write + 1) & (Size - 1);
    return next_write == current_read;
}

// ========== size 实现 ==========
template<typename T, size_t Size>
size_t LockFreeRingBuffer<T, Size>::size() const noexcept
{
    size_t current_write = write_index_.load(std::memory_order_acquire);
    size_t current_read = read_index_.load(std::memory_order_acquire);
    
    // 计算队列中的元素数量
    if (current_write >= current_read) {
        return current_write - current_read;
    } else {
        return Size - (current_read - current_write);
    }
}

// ========== capacity 实现 ==========
template<typename T, size_t Size>
constexpr size_t LockFreeRingBuffer<T, Size>::capacity() const noexcept
{
    return Size - 1;  // 实际可用容量是 Size-1（因为需要一个空位来区分满和空）
}

// ========== 零拷贝接口实现 ==========

template<typename T, size_t Size>
T* LockFreeRingBuffer<T, Size>::beginPush() noexcept
{
    size_t current_write = write_index_.load(std::memory_order_relaxed);
    size_t current_read = read_index_.load(std::memory_order_acquire);
    size_t next_write = (current_write + 1) & (Size - 1);
    
    if (next_write == current_read) {
        return nullptr;  // 队列满
    }
    
    return &buffer_[current_write];
}

template<typename T, size_t Size>
void LockFreeRingBuffer<T, Size>::commitPush() noexcept
{
    size_t current_write = write_index_.load(std::memory_order_relaxed);
    size_t next_write = (current_write + 1) & (Size - 1);
    write_index_.store(next_write, std::memory_order_release);
}

template<typename T, size_t Size>
const T* LockFreeRingBuffer<T, Size>::beginPop() noexcept
{
    size_t current_read = read_index_.load(std::memory_order_relaxed);
    size_t current_write = write_index_.load(std::memory_order_acquire);
    
    if (current_read == current_write) {
        return nullptr;  // 队列空
    }
    
    return &buffer_[current_read];
}

template<typename T, size_t Size>
void LockFreeRingBuffer<T, Size>::commitPop() noexcept
{
    size_t current_read = read_index_.load(std::memory_order_relaxed);
    size_t next_read = (current_read + 1) & (Size - 1);
    read_index_.store(next_read, std::memory_order_release);
}



