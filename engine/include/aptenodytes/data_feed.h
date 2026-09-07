#pragma once
#include <optional>
#include <vector>

#include "bar.h"

namespace apten {

// Abstract feed — seam for Tier 3 TickFeed without touching engine.
class DataFeed {
public:
    virtual ~DataFeed() = default;
    virtual std::optional<Bar> next() = 0;
    virtual void reset() = 0;
    virtual bool done() const = 0;
};

// Tier 2: sequential bar feed from sorted vector (loaded from parquet/csv).
class BarFeed : public DataFeed {
public:
    explicit BarFeed(std::vector<Bar> bars) : bars_(std::move(bars)), idx_(0) {}

    std::optional<Bar> next() override;
    void reset() override { idx_ = 0; }
    bool done() const override { return idx_ >= bars_.size(); }
    size_t size() const { return bars_.size(); }

private:
    std::vector<Bar> bars_;
    size_t idx_;
};

}  // namespace apten
