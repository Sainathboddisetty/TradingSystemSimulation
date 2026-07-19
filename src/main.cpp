#include "../src/mock_exchange.hpp"
#include "../src/order_manager.hpp"
#include "../src/reference_data_loader.hpp"
#include "../src/strategy.hpp"
#include "../include/latency_timer.hpp"

#include <iostream>

int main() {
    auto instruments =
        ts::ReferenceDataLoader::load("../data/instruments.csv");

    double ghz = ts::cpu_ghz();

    ts::MockExchange exchange(instruments);
    ts::OrderManager om(exchange, ghz);

    // alpha = 0.1
    // threshold = 0.3%
    ts::Strategy strategy(om, instruments, 0.1, 0.003);

    std::cout << "=============================\n";
    std::cout << " EMA Mean Reversion Test\n";
    std::cout << "=============================\n\n";

    //--------------------------------------------------------
    // Quote 1 : Initialize EMA
    //--------------------------------------------------------
    ts::Quote q1;
    q1.symbol = ts::Symbol("AAPL");
    q1.bid = 100.0;
    q1.ask = 100.2;

    std::cout << "Quote 1\n";
    strategy.on_quote(q1);

    std::cout << "EMA = "
              << strategy.current_ema(ts::Symbol("AAPL"))
              << "\n\n";

    //--------------------------------------------------------
    // Quote 2 : Small move (no signal expected)
    //--------------------------------------------------------
    ts::Quote q2;
    q2.symbol = ts::Symbol("AAPL");
    q2.bid = 100.1;
    q2.ask = 100.3;

    std::cout << "Quote 2\n";
    strategy.on_quote(q2);

    std::cout << "EMA = "
              << strategy.current_ema(ts::Symbol("AAPL"))
              << "\n\n";

    //--------------------------------------------------------
    // Quote 3 : Cheap -> BUY
    //--------------------------------------------------------
    ts::Quote q3;
    q3.symbol = ts::Symbol("AAPL");
    q3.bid = 99.4;
    q3.ask = 99.5;

    std::cout << "Quote 3 (Expect BUY)\n";
    strategy.on_quote(q3);

    std::cout << "EMA = "
              << strategy.current_ema(ts::Symbol("AAPL"))
              << "\n\n";

    //--------------------------------------------------------
    // Quote 4 : Another BUY should NOT happen because
    // an order is already outstanding.
    //--------------------------------------------------------
    ts::Quote q4;
    q4.symbol = ts::Symbol("AAPL");
    q4.bid = 99.3;
    q4.ask = 99.4;

    std::cout << "Quote 4 (Should NOT BUY again)\n";
    strategy.on_quote(q4);

    std::cout << '\n';

    //--------------------------------------------------------
    // Pretend order #1 filled.
    // (Replace 1 with actual order id if needed.)
    //--------------------------------------------------------
    std::cout << "Simulating fill...\n";
    strategy.on_fill(1);

    //--------------------------------------------------------
    // Quote 5 : Expensive -> SELL
    //--------------------------------------------------------
    ts::Quote q5;
    q5.symbol = ts::Symbol("AAPL");
    q5.bid = 101.5;
    q5.ask = 101.6;

    std::cout << "Quote 5 (Expect SELL)\n";
    strategy.on_quote(q5);

    std::cout << "EMA = "
              << strategy.current_ema(ts::Symbol("AAPL"))
              << "\n\n";

    return 0;
}