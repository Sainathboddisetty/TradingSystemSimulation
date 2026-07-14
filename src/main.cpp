#include "../include/memory_pool.hpp"
#include <iostream>


int main(){

    //**Test:**
// - [ ] Allocate 10 objects, verify they're different
// - [ ] Deallocate them all
// - [ ] Allocate again, verify you get the same pointers (stack behavior)

    ts::MemoryPool<int, 10> pool;
    int* ptrs[10];
    for (int i = 0; i < 10; ++i) {
        ptrs[i] = pool.allocate();
        *ptrs[i] = i;
    }
    for (int i = 0; i < 10; ++i) {
        std::cout << "Allocated: " << *ptrs[i] << " at " << ptrs[i] << std::endl;
    }
    for (int i = 0; i < 10; ++i) {
        pool.deallocate(ptrs[i]);
    }
    for (int i = 0; i < 10; ++i) {
        ptrs[i] = pool.allocate();
        *ptrs[i] = i+1;
        std::cout << "Re-allocated: " << *ptrs[i] << " at " << ptrs[i] << std::endl;
    }

    return 0;
}
