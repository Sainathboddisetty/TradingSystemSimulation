#pragma once
#include "../include/types.hpp"
#include "../include/quote.hpp"
#include "../include/instrument.hpp"
#include "order_manager.hpp"

#include <unordered_map>
#include <cstdio>
#include <cstring>

// ============================================================
// Simple Mean-Reversion Strategy
//
// FINANCE NOTE: Mean reversion assumes that prices drift away
// from a "fair value" and eventually snap back. We use an
// exponential moving average (EMA) as the fair value estimate.
//
//   EMA(t) = alpha * price(t) + (1 - alpha) * EMA(t-1)
//
//   Small alpha (e.g. 0.05) → slower EMA, less reactive
//   Large alpha (e.g. 0.50) → faster EMA, more reactive
//
// Entry rules:
//   BUY  when ask < EMA * (1 - threshold) — price is "cheap"
//   SELL when bid > EMA * (1 + threshold) — price is "expensive"
//
// Position management:
//   We hold at most 1 open order per symbol at a time.
//   A real strategy would also have a position limit and a
//   stop-loss to prevent runaway losses.
//
// RISK NOTE: Mean reversion fails in trending markets. Do NOT
// run this on real capital without proper backtesting and risk
// management. This is a teaching example only.
// ============================================================

namespace ts {

struct SymbolState {
    Price    ema{0.0};
    OrderId  open_order_id{0};
    bool     ema_initialised{false};
};

class Strategy {
public:
    Strategy(OrderManager& om, const std::vector<Instrument>& instruments,
             double alpha = 0.1, double threshold = 0.003)
        : om_(om), alpha_(alpha), threshold_(threshold)
    {
        for (const auto& inst : instruments) {
            states_[inst.symbol.c_str()] = SymbolState{};
        }
    }

    // Call on every incoming quote. May submit an order.
    void on_quote(const Quote& q) {
        auto it = states_.find(q.symbol.c_str());
        if (it == states_.end()) return;

        SymbolState& s = it->second;

        // Update EMA with mid price
        Price mid = q.mid();
        if (!s.ema_initialised) {
            s.ema = mid;
            s.ema_initialised = true;
            return;
        }
        s.ema = alpha_ * mid + (1.0 - alpha_) * s.ema;

        // Don't trade if we already have an open order for this symbol
        if (s.open_order_id != 0) return;

        // --- Signal generation ---
        if (q.ask < s.ema * (1.0 - threshold_)) {
            // Ask is below fair value → buy signal
            std::printf("[Strategy] BUY signal  %s  ask=%.4f  ema=%.4f  spread=%.4f\n",
                q.symbol.c_str(), q.ask, s.ema, q.spread());

            OrderId id = om_.submit(q.symbol, Side::Buy, OrderType::Limit, q.ask, 1);
            if (id) s.open_order_id = id;

        } else if (q.bid > s.ema * (1.0 + threshold_)) {
            // Bid is above fair value → sell signal
            std::printf("[Strategy] SELL signal %s  bid=%.4f  ema=%.4f  spread=%.4f\n",
                q.symbol.c_str(), q.bid, s.ema, q.spread());

            OrderId id = om_.submit(q.symbol, Side::Sell, OrderType::Limit, q.bid, 1);
            if (id) s.open_order_id = id;
        }
    }

    // Call after a fill is confirmed to free up the slot for this symbol
    void on_fill(OrderId filled_id) {
        for (auto& [sym, s] : states_) {
            if (s.open_order_id == filled_id) {
                s.open_order_id = 0;
                return;
            }
        }
    }

    Price current_ema(const Symbol& sym) const {
        auto it = states_.find(sym.c_str());
        return (it != states_.end()) ? it->second.ema : 0.0;
    }

private:
    OrderManager& om_;
    double alpha_;
    double threshold_;

    // Keyed by symbol string so lookup is a hash on a short string
    std::unordered_map<std::string, SymbolState> states_;
};

}
