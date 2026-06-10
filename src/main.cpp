#include "../include/instrument.hpp"
#include <iostream>


int main(){
    
     ts::Instrument inst;
    inst.symbol = ts::Symbol("AAPL");
    inst.exchange = ts::Symbol("NASDAQ");
    inst.currency = ts::Symbol("USD");
    inst.tick_size = 0.01;
    inst.lot_size = 1;
    inst.base_price = 150.00;
    inst.asset_class = ts::AssetClass::Equity;
    inst.print();
    std::cout << "Rounded price for 150.023: " << inst.round_to_tick(150.023) << std::endl;
    return 0;
}
