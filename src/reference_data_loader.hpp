#pragma once
#include "../include/instrument.hpp"
#include <vector>
#include <cstdio>
#include <cstring>

// ============================================================
// Reference Data Loader
//
// FINANCE NOTE: Reference data (static data) describes *what*
// can be traded — as opposed to market data which describes
// *current prices*. It is loaded once at startup from a file,
// database, or message bus (e.g. Bloomberg static data feed).
//
// In production, a dedicated reference data service publishes
// instrument definitions over a reliable multicast channel.
// Clients subscribe and keep an in-memory map keyed by symbol.
//
// We parse a simple CSV here. The fields match what a real
// exchange data dictionary would provide.
// ============================================================

namespace ts {

class ReferenceDataLoader {
public:
    // Returns all instruments parsed from the CSV file.
    // Empty vector if file cannot be opened.
    static std::vector<Instrument> load(const char* csv_path) {
        std::vector<Instrument> instruments;

        FILE* f = std::fopen(csv_path, "r");
        if (!f) {
            std::fprintf(stderr, "[RefData] ERROR: cannot open '%s'\n", csv_path);
            return instruments;
        }

        char line[256];
        bool first = true;
        while (std::fgets(line, sizeof(line), f)) {
            if (first) { first = false; continue; } // skip header

            // Trim trailing newline
            line[std::strcspn(line, "\n")] = '\0';

            char sym[16]{}, exch[16]{}, ccy[16]{}, ac_str[16]{};
            double tick_size = 0.0, base_price = 0.0;
            int lot_size = 1;

            int parsed = std::sscanf(line, "%15[^,],%15[^,],%lf,%d,%15[^,],%15[^,],%lf",
                sym, exch, &tick_size, &lot_size, ccy, ac_str, &base_price);

            if (parsed < 7) {
                std::fprintf(stderr, "[RefData] WARN: skipping malformed line: %s\n", line);
                continue;
            }

            Instrument inst{};
            inst.symbol     = Symbol(sym);
            inst.exchange   = Symbol(exch);
            inst.currency   = Symbol(ccy);
            inst.tick_size  = tick_size;
            inst.lot_size   = lot_size;
            inst.base_price = base_price;

            if      (std::strcmp(ac_str, "EQUITY")  == 0) inst.asset_class = AssetClass::Equity;
            else if (std::strcmp(ac_str, "ETF")     == 0) inst.asset_class = AssetClass::ETF;
            else if (std::strcmp(ac_str, "FUTURES") == 0) inst.asset_class = AssetClass::Futures;
            else if (std::strcmp(ac_str, "FX")      == 0) inst.asset_class = AssetClass::FX;
            else                                           inst.asset_class = AssetClass::Equity;

            instruments.push_back(inst);
        }

        std::fclose(f);

        std::printf("[RefData] Loaded %zu instrument(s) from '%s'\n",
            instruments.size(), csv_path);
        return instruments;
    }
};

}
