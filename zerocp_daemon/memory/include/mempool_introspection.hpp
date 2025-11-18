#ifndef ZEROCP_MEMPOOL_INTROSPECTION_HPP
#define ZEROCP_MEMPOOL_INTROSPECTION_HPP

class MempoolIntrospection
{
public:
    MempoolIntrospection() = default;
    MempoolIntrospection(const MempoolIntrospection& other) = delete;
    MempoolIntrospection(MempoolIntrospection&& other) noexcept = delete;
    MempoolIntrospection& operator=(const MempoolIntrospection& other) = delete;
    MempoolIntrospection& operator=(MempoolIntrospection&& other) noexcept = delete;
    ~MempoolIntrospection() noexcept = default;
private:
}


#endif // ZEROCP_MEMPOOL_INTROSPECTION_HPP
