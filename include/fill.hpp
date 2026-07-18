#pragma once
#include "types.hpp"

// ============================================================
// Fill (Execution Report)
//
// FINANCE NOTE: A fill is confirmation from the exchange that
// your order was (partially) executed. Key fields:
//
//   fill_price  – actual execution price (may differ from limit
//                 price if you got a better fill — price improvement)
//   fill_qty    – how many units were executed in this fill
//   order_id    – links back to your original order
//
// An order can generate multiple fills (partial fills).
// The total of all fill_qty values equals the order's qty
// once the order is fully filled.
//
// ULL NOTE: Fills arrive from the exchange gateway thread and
// must be passed to the strategy thread. We use the SPSC queue
// for this — zero contention, zero locks.
// ============================================================

namespace ts {

struct alignas(64) Fill {
    OrderId  order_id;      //  8 bytes
    Symbol   symbol;        // 16 bytes
    Price    fill_price;    //  8 bytes
    Nanos    timestamp_ns;  //  8 bytes
    Quantity fill_qty;      //  4 bytes
    Side     side;          //  1 byte
    uint8_t  _pad[19]{};    // 19 bytes → 64 bytes (one cache line)
};

} // namespace ts
