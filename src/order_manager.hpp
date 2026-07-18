#pragma once
#include "../include/types.hpp"
#include "../include/order.hpp"
#include "../include/fill.hpp"
#include "../include/memory_pool.hpp"
#include "../include/latency_timer.hpp"
#include "mock_exchange.hpp"

#include <unordered_map>
#include <cstdio>

// ============================================================
// Order Manager
//
// FINANCE NOTE: The order manager (OM) is responsible for:
//   1. Assigning unique order IDs
//   2. Sending orders to the exchange gateway
//   3. Tracking live orders (new → acknowledged → partial → filled)
//   4. Matching fills back to orders
//   5. Enforcing risk checks (position limits, fat-finger checks)
//
// In a real system the OM sits between the strategy and the
// exchange gateway. It adds latency but provides control:
//   - Pre-trade risk: reject orders that exceed limits
//   - Order state machine: correct sequencing of ack/fill/cancel
//   - Reconciliation: detect lost orders via sequence numbers
//
// ULL NOTE: We use a MemoryPool for Order objects to avoid heap
// allocation on the submission path. The live_orders_ map uses
// OrderId as key — a flat hash map (e.g. absl::flat_hash_map)
// would be faster in production than std::unordered_map.
// ============================================================

namespace ts {

class OrderManager {
    static constexpr std::size_t kMaxOrders = 64;

public:
    explicit OrderManager(MockExchange& exchange, double ghz)
        : exchange_(exchange), next_id_(1), ghz_(ghz), lat_stats_(ghz) {}

    // Submit a new order. Returns the assigned OrderId (0 on failure).
    OrderId submit(const Symbol& symbol, Side side, OrderType type,
                   Price limit_price, Quantity qty)
    {
        const uint64_t t_start = rdtscp();

        // --- Pre-trade risk check (simplified) ---
        if (qty <= 0 || limit_price < 0.0) {
            std::printf("[OM] REJECT: invalid params sym=%s qty=%d px=%.4f\n",
                symbol.c_str(), qty, limit_price);
            return 0;
        }

        // --- Allocate order from pool ---
        Order* slot = pool_.allocate();
        if (__builtin_expect(slot == nullptr, 0)) {
            std::printf("[OM] REJECT: order pool exhausted\n");
            return 0;
        }

        // Placement-new to initialise (safe over raw pool memory)
        Order* order = new (slot) Order{};
        order->id            = next_id_++;
        order->symbol        = symbol;
        order->side          = side;
        order->type          = type;
        order->limit_price   = limit_price;
        order->qty           = qty;
        order->filled_qty    = 0;
        order->avg_fill_price= 0.0;
        order->status        = OrderStatus::New;
        order->submit_time_ns = rdtsc();

        live_orders_[order->id] = order;

        // --- Send to exchange ---
        bool sent = exchange_.order_queue().push(*order);
        if (__builtin_expect(!sent, 0)) {
            std::printf("[OM] WARN: exchange queue full, order %lu dropped\n", order->id);
            live_orders_.erase(order->id);
            pool_.deallocate(order);
            return 0;
        }

        const uint64_t t_end = rdtscp();
        lat_stats_.record(t_start, t_end);

        std::printf("[OM] SEND  order=%lu  %s  %s  %s  qty=%d  limit=%.4f\n",
            order->id, symbol.c_str(), to_str(side), to_str(type), qty, limit_price);

        return order->id;
    }

    // Process one fill from the exchange (call in a polling loop).
    // out_fill  — the fill event from the exchange
    // out_order — snapshot of the order state after applying the fill
    // Returns true if a fill was processed.
    bool process_fills(Fill& out_fill, Order& out_order) {
        Fill fill{};
        if (!exchange_.fill_queue().pop(fill)) return false;

        auto it = live_orders_.find(fill.order_id);
        if (it == live_orders_.end()) {
            std::printf("[OM] WARN: fill for unknown order %lu\n", fill.order_id);
            return false;
        }

        Order* order = it->second;
        const Quantity prev_filled = order->filled_qty;
        order->filled_qty += fill.fill_qty;

        // Volume-weighted average fill price across partial fills
        order->avg_fill_price =
            (order->avg_fill_price * prev_filled + fill.fill_price * fill.fill_qty)
            / static_cast<double>(order->filled_qty);

        order->last_update_ns = fill.timestamp_ns;
        order->status = (order->filled_qty >= order->qty)
            ? OrderStatus::Filled : OrderStatus::PartialFill;

        std::printf("[OM] ACK   order=%lu  status=%s  filled=%d/%d  avg_px=%.4f\n",
            order->id, to_str(order->status),
            order->filled_qty, order->qty, order->avg_fill_price);

        out_fill  = fill;
        out_order = *order; // copy snapshot for caller before potential pool return

        if (order->is_done()) {
            live_orders_.erase(it);
            pool_.deallocate(order); // safe: caller has the snapshot in out_order
        }

        return true;
    }

    // Retrieve a live order by ID (returns nullptr if not found / already done)
    const Order* get_order(OrderId id) const {
        auto it = live_orders_.find(id);
        return (it != live_orders_.end()) ? it->second : nullptr;
    }

    void print_latency_stats() const {
        lat_stats_.print_summary("Order Submit");
    }

private:
    MockExchange&                          exchange_;
    MemoryPool<Order, kMaxOrders>          pool_;
    std::unordered_map<OrderId, Order*>    live_orders_;
    uint64_t                               next_id_;
    double                                 ghz_;
    mutable LatencyStats<1024>             lat_stats_;
};

}
