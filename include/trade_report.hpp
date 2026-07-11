#pragma once
#include "types.hpp"
#include <cstdio>

// ============================================================
// Trade Report
//
// FINANCE NOTE: A trade report summarises a completed order.
//
//   gross_pnl   – unrealised P&L vs entry price.
//                 For a buy: (current_price - avg_fill) * qty
//                 For a sell: (avg_fill - current_price) * qty
//                 We use mid-price as the mark-to-market proxy.
//
//   slippage    – difference between the price you requested
//                 (limit or mid at submission) and the actual
//                 fill price. Negative = you paid more than expected.
//
//   commission  – brokerage fee (mocked at $0.005 per share for equities,
//                 $2.00 per futures contract).
//
// FINANCE NOTE: net_pnl = gross_pnl - commission
// In a real system you would also subtract exchange fees,
// clearing fees, and SEC/FINRA transaction taxes.
// ============================================================

namespace ts {

struct TradeReport {
    OrderId  order_id;
    Symbol   symbol;
    Side     side;
    Quantity qty;
    Price    avg_fill_price;
    Price    requested_price;   // limit or mid-at-submission
    Price    mark_price;        // current mid for unrealised P&L
    Price    slippage;          // avg_fill - requested (buy), requested - avg_fill (sell)
    Price    gross_pnl;
    Price    commission;
    Price    net_pnl;
    Nanos    fill_time_ns;

    void print() const {
        std::printf("\n========== TRADE REPORT ==========\n");
        std::printf("  Order ID   : %lu\n",      order_id);
        std::printf("  Symbol     : %s\n",        symbol.c_str());
        std::printf("  Side       : %s\n",        to_str(side));
        std::printf("  Quantity   : %d\n",        qty);
        std::printf("  Avg Fill   : %.4f\n",      avg_fill_price);
        std::printf("  Requested  : %.4f\n",      requested_price);
        std::printf("  Slippage   : %+.4f\n",     slippage);
        std::printf("  Mark Price : %.4f\n",      mark_price);
        std::printf("  Gross P&L  : %+.2f USD\n", gross_pnl);
        std::printf("  Commission : -%.2f USD\n", commission);
        std::printf("  Net P&L    : %+.2f USD\n", net_pnl);
        std::printf("==================================\n\n");
    }
};

}
