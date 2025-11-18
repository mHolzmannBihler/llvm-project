#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <new>

// BihlList mock for testing
namespace BihlList {
    template <class C>
    class List {
    public:
        C* add(C* element) { return element; }
        
        template<class... Ts>
        C* emplace_back(Ts&&... args) {
            return this->add(new(std::nothrow) C(std::forward<Ts>(args)...));
        }
    };
}

void test_bihllist() {
    BihlList::List<int> bihl_list;
    
    // Should warn: return value not checked for nullptr
    bihl_list.emplace_back(42);
    
    // Should NOT warn: return value checked in if
    if (auto* ptr = bihl_list.emplace_back(99)) {
        // Use ptr safely
        (void)ptr;
    }
    
    // Should NOT warn: explicit nullptr check
    auto* result = bihl_list.emplace_back(123);
    if (result != nullptr) {
        // Use result safely
        (void)result;
    }
    
    // Should NOT warn: checked with negation
    if (!bihl_list.emplace_back(456)) {
        // Handle allocation failure
    }
}

void test_comprehensive_stl_allocations() {
    // === std::vector tests ===
    std::vector<int> vec;
    vec.push_back(1);           // Should warn
    vec.emplace_back(2);        // Should warn  
    vec.insert(vec.begin(), 3); // Should warn
    vec.resize(100);            // Should warn
    vec.reserve(200);           // Should warn
    vec.assign(10, 42);         // Should warn
    vec.shrink_to_fit();        // Should warn
    
    // === std::deque tests ===
    std::deque<int> deq;
    deq.push_back(1);           // Should warn
    deq.push_front(2);          // Should warn
    deq.emplace_back(3);        // Should warn
    deq.emplace_front(4);       // Should warn
    
    // === std::list tests ===
    std::list<int> lst;
    lst.push_back(1);           // Should warn
    lst.push_front(2);          // Should warn
    lst.emplace_back(3);        // Should warn
    lst.emplace_front(4);       // Should warn
    
    // === std::string tests ===
    std::string str;
    str.append("hello");        // Should warn
    str += " world";            // Should warn (operator+=)
    str.replace(0, 1, "H");     // Should warn
    
    // === std::map tests ===
    std::map<int, std::string> mp;
    mp.insert({1, "one"});      // Should warn
    mp.emplace(2, "two");       // Should warn
    mp[3] = "three";            // Should warn (operator[])
    
    // === std::unordered_map tests ===
    std::unordered_map<int, std::string> ump;
    ump.insert({1, "one"});     // Should warn
    ump.emplace(2, "two");      // Should warn
    ump[3] = "three";           // Should warn (operator[])
    ump.reserve(100);           // Should warn
    ump.rehash(50);             // Should warn
    
    // === std::set tests ===
    std::set<int> st;
    st.insert(1);               // Should warn
    st.emplace(2);              // Should warn
    
    // === std::unordered_set tests ===
    std::unordered_set<int> ust;
    ust.insert(1);              // Should warn
    ust.emplace(2);             // Should warn
    ust.reserve(100);           // Should warn
    ust.rehash(50);             // Should warn
    
    // === Smart pointers ===
    auto smart1 = std::make_unique<int>(42);        // Should warn
    auto smart2 = std::make_shared<std::string>("test"); // Should warn
    
    // === Regular new/nothrow tests ===
    int* ptr1 = new int(42);                        // Should warn
    int* ptr2 = new(std::nothrow) int(99);          // Should NOT warn (nothrow)
    
    // === Protected allocations (should NOT warn) ===
    try {
        vec.push_back(999);                         // Should NOT warn (in try)
        mp[99] = "protected";                       // Should NOT warn (in try)
        auto safe_ptr = std::make_unique<int>(77);  // Should NOT warn (in try)
        int* safe_new = new int(88);                // Should NOT warn (in try)
    } catch (const std::bad_alloc&) {
        // Handle allocation failure
    }
}