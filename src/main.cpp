#include "../include/latency_timer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(){

    double ghz = ts::cpu_ghz();
    uint64_t t0 = ts::rdtscp();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint64_t t1 = ts::rdtscp();
    double ns = ts::ticks_to_ns(t1 - t0, ghz);
    std::cout << "Latency: " << ns << " ns" << std::endl;
    return 0;
}
