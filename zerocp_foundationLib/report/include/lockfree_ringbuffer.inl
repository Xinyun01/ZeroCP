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
    // 使用 CAS 循环来处理多生产者竞争
    size_t current_write = write_index_.load(std::memory_order_relaxed);
    
    while (true) {
        // 1. 读取当前的读索引
        size_t current_read = read_index_.load(std::memory_order_acquire);
        
        // 2. 计算下一个写位置
        size_t next_write = (current_write + 1) & (Size - 1);
        
        // 3. 检查队列是否已满（下一个写位置等于读位置表示满）
        if (next_write == current_read) {
            return false;  // 队列满
        }
        
        // 4. 尝试使用 CAS 原子地预留写位置
        if (write_index_.compare_exchange_weak(
                current_write, next_write,
                std::memory_order_release,
                std::memory_order_relaxed)) {
            // CAS 成功，我们已经预留了 current_write 位置
            // 5. 写入数据
            buffer_[current_write] = item;
            return true;
        }
        
        // CAS 失败，current_write 已被更新为新值，重试
        // compare_exchange_weak 会自动更新 current_write 为最新值
    }
}

// ========== tryPop 实现 ==========
template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::tryPop(T& item) noexcept
{
    // 使用 CAS 循环来处理多消费者竞争
    size_t current_read = read_index_.load(std::memory_order_relaxed);
    
    while (true) {
        // 1. 读取当前的写索引
        size_t current_write = write_index_.load(std::memory_order_acquire);
        
        // 2. 检查队列是否为空（读索引等于写索引表示空）
        if (current_read == current_write) {
            return false;  // 队列空
        }
        
        // 3. 尝试使用 CAS 原子地预留读位置
        size_t next_read = (current_read + 1) & (Size - 1);
        if (read_index_.compare_exchange_weak(
                current_read, next_read,
                std::memory_order_release,
                std::memory_order_relaxed)) {
            // CAS 成功，我们已经预留了 current_read 位置
            // 4. 读取数据
            item = buffer_[current_read];
            return true;
        }
        
        // CAS 失败，current_read 已被更新为新值，重试
        // compare_exchange_weak 会自动更新 current_read 为最新值
    }
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



