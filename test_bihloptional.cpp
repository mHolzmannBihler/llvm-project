// Test file for BihlOptional::emplace and ::create nullptr checking

#include <new>
#include <utility>

namespace BihlOptional {
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
}

void test_bihloptional() {
    BihlOptional::Optional<int> opt;
    
    // Should warn: emplace return value not checked
    opt.emplace(42);
    
    // Should NOT warn: emplace checked in if
    if (auto* ptr = opt.emplace(99)) {
        // Use ptr safely
        (void)ptr;
    }
    
    // Should NOT warn: emplace with explicit nullptr check
    auto* result = opt.emplace(123);
    if (result != nullptr) {
        // Use result safely
        (void)result;
    }
    
    // Should NOT warn: emplace checked with negation
    if (!opt.emplace(456)) {
        // Handle allocation failure
    }
    
    // Should warn: create return value not checked
    opt.create();
    
    // Should NOT warn: create checked in if
    if (auto* ptr = opt.create()) {
        // Use ptr safely
        (void)ptr;
    }
    
    // Should NOT warn: create with explicit nullptr check
    auto* result2 = opt.create();
    if (result2 != nullptr) {
        // Use result2 safely
        (void)result2;
    }
    
    // Should NOT warn: create checked with negation
    if (!opt.create()) {
        // Handle allocation failure
    }
}
