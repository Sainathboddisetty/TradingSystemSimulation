#include "../src/mock_exchange.hpp"
#include "../src/reference_data_loader.hpp"
#include <iostream>

int main() {
    auto instruments =
        ts::ReferenceDataLoader::load("../data/instruments.csv");

    ts::MockExchange exchange(instruments);

    exchange.start();

    ts::Quote q{};

    int received = 0;
    while (received < 10) {
        if (exchange.quote_queue().pop(q)) {
            std::cout
                << q.symbol.c_str()
                << " Bid=" << q.bid
                << " Ask=" << q.ask
                << " BidQty=" << q.bid_size
                << " AskQty=" << q.ask_size
                << '\n';

            ++received;
        }
    }

    exchange.stop();
    return 0;
}