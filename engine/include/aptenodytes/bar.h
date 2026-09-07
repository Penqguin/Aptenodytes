#pragma once
#include <cstdint>
#include <string>

namespace apten {

struct Bar {
    int64_t ts_utc_ms;  // UTC epoch ms, bar open time
    std::string symbol;
    double open;
    double high;
    double low;
    double close;
    double adj_close;  // adjusted close (splits/dividends)
    double volume;

    // invariant: high >= max(open,close), low <= min(open,close)
};

}  // namespace apten
