#pragma once
#include "types.hpp"


namespace ts {

struct alignas(64) Order{
    OrderId    id;              //  8 bytes
    Symbol     symbol;          // 16 bytes
    Price      limit_price;     //  8 bytes  (0.0 for Market orders)
    Price      avg_fill_price;  //  8 bytes  — accumulated for partial fills
    Nanos      submit_time_ns;  //  8 bytes
    Nanos      last_update_ns;  //  8 bytes
    Quantity   qty;             //  4 bytes  — requested
    Quantity   filled_qty;      //  4 bytes  — cumulative filled
    Side       side;            //  1 byte
    OrderType  type;            //  1 byte
    OrderStatus status;         //  1 byte
    uint8_t _pad[61]{};       // 128 - 67 = 61 bytes padding → 128 bytes (two cache lines)

    Quantity remaining() const { return qty - filled_qty; }

    bool is_done() const {
        return status == OrderStatus::Filled
            || status == OrderStatus::Cancelled
            || status == OrderStatus::Rejected;
    }

};

}