#include "../src/reference_data_loader.hpp"
#include <iostream>

int main() {

    auto instruments = ts::ReferenceDataLoader::load("../data/instruments.csv");

    std::cout << "\nLoaded Instruments:\n\n";

    for (const auto& inst : instruments) {
        std::cout
            << "Symbol      : " << inst.symbol.c_str() << '\n'
            << "Exchange    : " << inst.exchange.c_str() << '\n'
            << "Currency    : " << inst.currency.c_str() << '\n'
            << "Tick Size   : " << inst.tick_size << '\n'
            << "Lot Size    : " << inst.lot_size << '\n'
            << "Base Price  : " << inst.base_price << '\n'
            << "Asset Class : ";

        switch (inst.asset_class) {
            case ts::AssetClass::Equity:
                std::cout << "Equity";
                break;
            case ts::AssetClass::ETF:
                std::cout << "ETF";
                break;
            case ts::AssetClass::Futures:
                std::cout << "Futures";
                break;
            case ts::AssetClass::FX:
                std::cout << "FX";
                break;
        }

        std::cout << "\n--------------------------\n";
    }

    return 0;
}