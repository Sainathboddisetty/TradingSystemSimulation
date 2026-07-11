#include "../include/order.hpp"
#include <iostream>


int main(){
    
    ts::Order order;
    order.id = 123456789;
    order.symbol = ts::Symbol("AAPL");
    order.limit_price = 150.25;
    order.qty = 100;
    std::cout << "Order created: " << order.id<< std::endl;
    // Print order details
    std::cout << "Symbol: " << order.symbol.c_str() << std::endl;
    std::cout << "Limit Price: " << order.limit_price << std::endl;
    std::cout << "Quantity: " << order.qty << std::endl;    
    return 0;
}
