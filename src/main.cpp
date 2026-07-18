#include "../src/mock_exchange.hpp"
#include "../src/order_manager.hpp"
#include "../src/reference_data_loader.hpp"
#include "../include/latency_timer.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    auto instruments =
        ts::ReferenceDataLoader::load("../data/instruments.csv");

    double ghz = ts::cpu_ghz();

    ts::MockExchange exchange(instruments);
    ts::OrderManager om(exchange, ghz);

    exchange.start();

    // Submit one order
    ts::OrderId id = om.submit(
        ts::Symbol("AAPL"),
        ts::Side::Buy,
        ts::OrderType::Limit,
        185.00,
        100
    );

    std::cout << "Submitted Order ID = " << id << '\n';

    // Give the exchange time to process the order
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ts::Fill fill;
    ts::Order order;

    // Process all available fills
    while (om.process_fills(fill, order)) {
        std::cout
            << "Fill received:\n"
            << "Order ID: " << fill.order_id << '\n'
            << "Filled Qty: " << fill.fill_qty << '\n'
            << "Fill Price: " << fill.fill_price << '\n'
            << "Order Status: " << ts::to_str(order.status) << '\n'
            << "Total Filled: " << order.filled_qty << "/"
            << order.qty << "\n\n";
    }

    om.print_latency_stats();

    exchange.stop();
}