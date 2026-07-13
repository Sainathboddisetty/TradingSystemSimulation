#include "../include/spsc_queue.hpp"
#include <iostream>


int main(){

    ts::SPSCQueue<int, 4> queue;
    
    queue.push(1);
    queue.push(2);
    std::cout << "push result for 3: " << queue.push(3) << std::endl; 
    queue.push(4);
    bool push_result = queue.push(4);
    std::cout << "Push result for 4: " << push_result << std::endl; 
    int item;
    queue.pop(item);
    queue.pop(item);
    queue.pop(item);


    return 0;
}
