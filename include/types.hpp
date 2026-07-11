#pragma once
#include <cstdint>
#include <array>
#include <cstring>

namespace ts {

using Price = double;
using Quantity = int32_t;
using OrderId = uint64_t;
using Nanos = uint64_t;

enum class Side: uint8_t {Buy, Sell};
enum class OrderType: uint8_t {Market, Limit};
enum class OrderStatus: uint8_t { New, Acknowledged, PartialFill, Filled, Cancelled, Rejected };
enum class AssetClass: uint8_t { Equity, ETF, Futures, FX };

struct Symbol { 
    std::array<char, 16> data{};

    Symbol() = default;

    explicit Symbol(const char* str) {
        std::strncpy(data.data(), str, data.size() - 1);
        data.back() = '\0'; // Ensure null-termination
    }

    const char* c_str() const { return data.data(); }

    bool operator==(const Symbol& other) const {
        return data == other.data;
    }

    bool operator!=(const Symbol& other) const {
        return !(*this == other);
    }
};


// readable helpers (used only in reporting, never in hot path)
inline const char* to_str(Side s)        { return s == Side::Buy ? "BUY" : "SELL"; }
inline const char* to_str(OrderType t)   { return t == OrderType::Market ? "MARKET" : "LIMIT"; }
inline const char* to_str(AssetClass a) {
    switch (a) {
        case AssetClass::Equity:  return "EQUITY";
        case AssetClass::ETF:     return "ETF";
        case AssetClass::Futures: return "FUTURES";
        case AssetClass::FX:      return "FX";
        default:                  return "UNKNOWN";
    }
}
inline const char* to_str(OrderStatus s) {
    switch (s) {
        case OrderStatus::New:         return "NEW";
        case OrderStatus::Acknowledged:return "ACKNOWLEDGED";
        case OrderStatus::PartialFill: return "PARTIAL_FILL";
        case OrderStatus::Filled:      return "FILLED";
        case OrderStatus::Cancelled:   return "CANCELLED";
        case OrderStatus::Rejected:    return "REJECTED";
        default:                       return "UNKNOWN";
    }
}
}   // namespace ts
