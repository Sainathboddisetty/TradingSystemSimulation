#pragma once
#include "../include/types.hpp"
#include "../include/instrument.hpp"
#include "../include/quote.hpp"
#include "../include/order.hpp"
#include "../include/fill.hpp"
#include "../include/spsc_queue.hpp"
#include "../include/latency_timer.hpp"

#include <vector>
#include <unordered_map>
#include <random>
#include <cstring>
#include <cstdio>
#include <atomic>
#include <thread>
#include <chrono>

// ============================================================
// Mock Exchange
//
// FINANCE NOTE: A real exchange has:
//   1. An order book per instrument (price-time priority matching)
//   2. A matching engine that creates fills when buy/sell cross
//   3. A market data feed (multicast UDP) publishing quotes
//   4. An order entry gateway (TCP/OUCH/FIX) for order flow
//
// We simulate all of this in a single class:
//   - Random-walk price generation (realistic short-term behaviour)
//   - Immediate fill at ask (buy) or bid (sell) — "market making"
//     In reality fills depend on liquidity and queue position.
//
// ULL NOTE: The exchange runs in its own thread.
//   Orders arrive via an SPSC queue (order_queue_).
//   Quotes depart via an SPSC queue (quote_queue_).
//   Fills depart  via an SPSC queue (fill_queue_).
//
// Queue sizes are powers of 2 — critical for the bitmask trick.
// ============================================================

namespace ts {

struct InstrumentState {
    Instrument   inst;
    Price        current_bid{};
    Price        current_ask{};
    double       drift{};       // per-tick price drift (tiny, for trending)
    double       volatility{};  // per-tick std dev
};

class MockExchange {
public:
    static constexpr std::size_t kQueueSize = 256; // power of 2

    using OrderQueue = SPSCQueue<Order, kQueueSize>;
    using QuoteQueue = SPSCQueue<Quote, kQueueSize>;
    using FillQueue  = SPSCQueue<Fill,  kQueueSize>;

    explicit MockExchange(const std::vector<Instrument>& instruments)
        : running_(false), next_order_id_(1), ghz_(cpu_ghz())
    {
        rng_.seed(42);

        for (const auto& inst : instruments) {
            InstrumentState s;
            s.inst        = inst;
            // Start bid/ask around base price with a 1-tick spread
            s.current_bid = inst.base_price - inst.tick_size;
            s.current_ask = inst.base_price + inst.tick_size;
            s.volatility  = inst.base_price * 0.0002; // 0.02% per tick
            s.drift       = 0.0;
            states_.push_back(s);
        }

        std::printf("[Exchange] Initialised with %zu instrument(s)\n", states_.size());
    }

    // ---- accessors for the client side ----
    OrderQueue& order_queue() { return order_queue_; }
    QuoteQueue& quote_queue() { return quote_queue_; }
    FillQueue&  fill_queue()  { return fill_queue_;  }

    // ---- exchange thread entry point ----
    void start() {
        running_ = true;
        thread_ = std::thread(&MockExchange::run, this);
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

    double ghz() const { return ghz_; }

private:
    void run() {
        using namespace std::chrono_literals;
        std::printf("[Exchange] Thread started\n");

        uint64_t tick = 0;
        while (running_.load(std::memory_order_relaxed)) {
            ++tick;
            tick_prices();
            publish_quotes();
            process_orders();

            // Simulate ~1ms between price ticks.
            // In reality the matching engine processes events as fast as hardware allows.
            std::this_thread::sleep_for(1ms);
        }
        std::printf("[Exchange] Thread stopped after %lu ticks\n", tick);
    }

    void tick_prices() {
        std::normal_distribution<double> noise(0.0, 1.0);
        for (auto& s : states_) {
            double move = s.drift + noise(rng_) * s.volatility;
            // Round to tick size
            s.current_bid = s.inst.round_to_tick(s.current_bid + move);
            s.current_ask = s.inst.round_to_tick(s.current_bid + 2.0 * s.inst.tick_size);
            // Clamp to positive
            if (s.current_bid < s.inst.tick_size) s.current_bid = s.inst.tick_size;
            if (s.current_ask <= s.current_bid)   s.current_ask = s.current_bid + s.inst.tick_size;
        }
    }

    void publish_quotes() {
        for (const auto& s : states_) {
            Quote q{};
            q.symbol       = s.inst.symbol;
            q.bid          = s.current_bid;
            q.ask          = s.current_ask;
            q.bid_size     = 100 + static_cast<int>(rng_() % 900); // random depth
            q.ask_size     = 100 + static_cast<int>(rng_() % 900);
            q.timestamp_ns = rdtsc();

            // Best-effort push — drop if consumer is slow (in ULL, never block exchange)
            quote_queue_.push(q);
        }
    }

    void process_orders() {
        Order order{};
        while (order_queue_.pop(order)) {
            Fill fill = match_order(order);
            fill_queue_.push(fill);

            std::printf("[Exchange] FILL  order=%lu  %s  %s  qty=%d  @ %.4f\n",
                fill.order_id, fill.symbol.c_str(),
                to_str(fill.side), fill.fill_qty, fill.fill_price);
        }
    }

    // Simplified matching: immediate full fill at bid (sell) or ask (buy).
    // FINANCE NOTE: This is "market order" semantics. A limit order would
    // only fill if the market price crosses the limit. We treat all orders
    // as market orders for simplicity.
    Fill match_order(const Order& order) {
        Fill fill{};
        fill.order_id    = order.id;
        fill.symbol      = order.symbol;
        fill.side        = order.side;
        fill.fill_qty    = order.qty;
        fill.timestamp_ns = rdtsc();

        // Find current price for this symbol
        for (const auto& s : states_) {
            if (s.inst.symbol == order.symbol) {
                fill.fill_price = (order.side == Side::Buy) ? s.current_ask : s.current_bid;
                return fill;
            }
        }

        // Symbol not found — fill at 0 (error case)
        fill.fill_price = 0.0;
        return fill;
    }

    alignas(64) OrderQueue order_queue_;
    alignas(64) QuoteQueue quote_queue_;
    alignas(64) FillQueue  fill_queue_;

    std::vector<InstrumentState> states_;
    std::mt19937 rng_;
    std::atomic<bool> running_;
    std::atomic<uint64_t> next_order_id_;
    std::thread thread_;
    double ghz_;
};

}
