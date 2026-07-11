#include "../include/trade_report.hpp"
#include <iostream>


int main(){
    
    //**Test:** Manually create a trade report, verify the math.
    
    ts::TradeReport report;
    report.order_id = 123456789;
    report.symbol = ts::Symbol("AAPL");
    report.side = ts::Side::Buy;
    report.qty = 100;
    report.avg_fill_price = 150.25;
    report.requested_price = 150.00;
    report.mark_price = 151.00;
    report.slippage = report.avg_fill_price - report.requested_price; // 0.25
    report.gross_pnl = (report.mark_price - report.avg_fill_price) * report.qty; // (151.00 - 150.25) * 100 = 75
    report.commission = 0.005 * report.qty; // $0.005 per share for equities = 0.005 * 100 = 0.5
    report.net_pnl = report.gross_pnl - report.commission; // 75 - 0.5 = 74.5
    report.fill_time_ns = 1234567890;   

    report.print();
    return 0;
}
