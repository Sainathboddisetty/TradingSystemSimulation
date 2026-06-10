#pragma once
#include "types.hpp"
#include <cstdio>

namespace ts {
struct alignas(64) Instrument{
    Symbol symbol;          // 16 bytes
    Symbol exchange;        // 16 bytes  (NASDAQ, NYSE, CME)
    Symbol currency;        // 16 bytes  (USD, EUR)
    Price tick_size;        // 8  bytes
    Quantity lot_size;      // 4  bytes
    Price base_price;       // 8  bytes
    AssetClass asset_class; // 1  byte 
    uint8_t _pad[59]{}; // total 69 + 59 = 128 bytes (2 cache line size)

    Price round_to_tick(Price p){
        return static_cast<int64_t>(p / tick_size) * tick_size;
    }

    void print() const{
        std::printf("  %-8s  exchange=%-8s  class=%-8s  tick=%.4f  lot=%d  base_px=%.2f\n",
            symbol.c_str(), exchange.c_str(), to_str(asset_class),
            tick_size, lot_size, base_price);
    }


};
}