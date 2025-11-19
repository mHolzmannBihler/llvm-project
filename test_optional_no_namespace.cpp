#include <new>
#include <utility>

// Optional class WITHOUT namespace (like in your codebase)
template<typename T>
class Optional {
private:
    T* pData = nullptr;
    
public:
    template<class... Args>
    T* emplace(Args&&... args) {
        this->pData = new(std::nothrow) T(std::forward<Args>(args)...);
        return this->pData;
    }
    
    T* create() {
        this->pData = new(std::nothrow) T();
        return this->pData;
    }
};

class Socket {
public:
    Socket(int a, int b, int c) {}
};

void test_optional_no_namespace() {
    Optional<Socket> sock;
    
    // Should NOT warn: emplace checked with negation (like your code)
    if (!sock.emplace(1, 2, 3)) {
        // Handle error
    }
    
    // Should warn: unchecked emplace
    sock.emplace(4, 5, 6);
    
    // Should NOT warn: checked in if
    if (auto* ptr = sock.emplace(7, 8, 9)) {
        (void)ptr;
    }
    
    // Should NOT warn: checked with nullptr comparison
    auto* result = sock.emplace(10, 11, 12);
    if (result != nullptr) {
        (void)result;
    }
}
