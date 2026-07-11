#pragma once
#include "types.hpp"

namespace ts {

struct alignas(64) Quote {
    Symbol   symbol;        // 16 bytes
    Price    bid;           //  8 bytes
    Price    ask;           //  8 bytes
    Quantity bid_size;      //  4 bytes
    Quantity ask_size;      //  4 bytes
    Nanos    timestamp_ns;  //  8 bytes  — 48 bytes total
    uint8_t  _pad[16]{};    // 16 bytes padding → 64 bytes (one cache line)

    //const defination here helpful so that if we pass Quote object to funtion, we know mid won't modify and we can call even when the object passed as const again.

    Price mid() const { return (bid + ask) * 0.5; }
    Price spread() const { return ask - bid; }

    bool valid() const { return bid > 0.0 && ask > bid; }

};

}