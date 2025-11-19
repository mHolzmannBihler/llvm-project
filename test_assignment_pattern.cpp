#include <new>
#include <utility>

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

class State {
public:
    State(int a, int b) {}
};

void test_assignment_pattern() {
    BihlList::List<State> states;
    State* tempState = nullptr;
    
    // Should NOT warn: assigned to variable, then checked
    tempState = states.emplace_back(1, 2);
    if(!tempState) {
        // Handle error
    }
    
    // Should warn: assigned but never checked
    tempState = states.emplace_back(3, 4);
    
    // Should NOT warn: direct check in if
    if (auto* s = states.emplace_back(5, 6)) {
        (void)s;
    }
}
