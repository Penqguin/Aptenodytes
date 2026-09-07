#include "aptenodytes/data_feed.h"

namespace apten {

std::optional<Bar> BarFeed::next() {
    if (done()) return std::nullopt;
    return bars_[idx_++];
}

}  // namespace apten
