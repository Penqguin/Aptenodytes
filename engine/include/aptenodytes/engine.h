#pragma once
#include "data_feed.h"
#include "execution_model.h"
#include "portfolio.h"
#include "strategy.h"

namespace apten {

class Engine {
public:
    Engine(DataFeed& feed, ExecutionModel& exec, Portfolio& portfolio, Strategy& strategy)
        : feed_(feed), exec_(exec), portfolio_(portfolio), strategy_(strategy) {}

    // Run event loop: for each bar -> strategy.on_bar -> execute pending at next bar -> mark.
    void run();

private:
    DataFeed& feed_;
    ExecutionModel& exec_;
    Portfolio& portfolio_;
    Strategy& strategy_;
};

}  // namespace apten
