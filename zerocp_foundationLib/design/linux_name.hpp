#ifndef ZeroCP_Linux_Name_HPP
#define ZeroCP_Linux_Name_HPP

#include <string>
#include <stdexcept>
#include <cstring>

namespace ZeroCP {
namespace Design {

/**
 * @brief Linux系统名字类型，用于限制名字长度符合Linux系统要求
 * 
 * Linux系统对名字长度有严格限制：
 * - 共享内存对象名：最大255字节
 * - 信号量名：最大251字节  
 * - 消息队列名：最大251字节
 * - 文件系统路径：最大255字节
 */
class LinuxName {
public:
    static constexpr size_t MAX_LENGTH = 255;  // Linux系统最大名字长度
    
    /**
     * @brief 默认构造函数
     */
    LinuxName() noexcept : m_name{} {}
    
    /**
     * @brief 从C字符串构造
     * @param name C风格字符串
     * @throws std::invalid_argument 如果名字长度超过限制
     */
    explicit LinuxName(const char* name) {
        if (!name) {
            m_name[0] = '\0';
            return;
        }
        
        size_t len = std::strlen(name);
        if (len >= MAX_LENGTH) {
            throw std::invalid_argument("Linux name too long: " + std::to_string(len) + 
                                      " >= " + std::to_string(MAX_LENGTH));
        }
        
        std::strncpy(m_name, name, MAX_LENGTH - 1);
        m_name[MAX_LENGTH - 1] = '\0';
    }
    
    /**
     * @brief 从std::string构造
     * @param name 字符串
     * @throws std::invalid_argument 如果名字长度超过限制
     */
    explicit LinuxName(const std::string& name) {
        if (name.length() >= MAX_LENGTH) {
            throw std::invalid_argument("Linux name too long: " + std::to_string(name.length()) + 
                                      " >= " + std::to_string(MAX_LENGTH));
        }
        
        std::strncpy(m_name, name.c_str(), MAX_LENGTH - 1);
        m_name[MAX_LENGTH - 1] = '\0';
    }
    
    /**
     * @brief 拷贝构造函数
     */
    LinuxName(const LinuxName& other) noexcept {
        std::strncpy(m_name, other.m_name, MAX_LENGTH - 1);
        m_name[MAX_LENGTH - 1] = '\0';
    }
    
    /**
     * @brief 移动构造函数
     */
    LinuxName(LinuxName&& other) noexcept {
        std::strncpy(m_name, other.m_name, MAX_LENGTH - 1);
        m_name[MAX_LENGTH - 1] = '\0';
        other.m_name[0] = '\0';
    }
    
    /**
     * @brief 拷贝赋值操作符
     */
    LinuxName& operator=(const LinuxName& other) noexcept {
        if (this != &other) {
            std::strncpy(m_name, other.m_name, MAX_LENGTH - 1);
            m_name[MAX_LENGTH - 1] = '\0';
        }
        return *this;
    }
    
    /**
     * @brief 移动赋值操作符
     */
    LinuxName& operator=(LinuxName&& other) noexcept {
        if (this != &other) {
            std::strncpy(m_name, other.m_name, MAX_LENGTH - 1);
            m_name[MAX_LENGTH - 1] = '\0';
            other.m_name[0] = '\0';
        }
        return *this;
    }
    
    /**
     * @brief 获取C风格字符串
     * @return const char* 内部存储的C字符串
     */
    const char* c_str() const noexcept {
        return m_name;
    }
    
    /**
     * @brief 获取std::string
     * @return std::string 字符串副本
     */
    std::string str() const noexcept {
        return std::string(m_name);
    }
    
    /**
     * @brief 获取名字长度
     * @return size_t 名字长度（不包括null终止符）
     */
    size_t length() const noexcept {
        return std::strlen(m_name);
    }
    
    /**
     * @brief 检查是否为空
     * @return bool 如果名字为空返回true
     */
    bool empty() const noexcept {
        return m_name[0] == '\0';
    }
    
    /**
     * @brief 比较操作符
     */
    bool operator==(const LinuxName& other) const noexcept {
        return std::strcmp(m_name, other.m_name) == 0;
    }
    
    bool operator!=(const LinuxName& other) const noexcept {
        return !(*this == other);
    }
    
    bool operator<(const LinuxName& other) const noexcept {
        return std::strcmp(m_name, other.m_name) < 0;
    }
    
    /**
     * @brief 流输出操作符
     */
    friend std::ostream& operator<<(std::ostream& os, const LinuxName& name) {
        return os << name.m_name;
    }

private:
    char m_name[MAX_LENGTH];  // 内部存储的C字符串
};

} // namespace Design
} // namespace ZeroCP

#endif // ZeroCP_Linux_Name_HPP
