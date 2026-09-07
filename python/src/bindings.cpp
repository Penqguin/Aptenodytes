#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "aptenodytes/bar.h"
#include "aptenodytes/data_feed.h"
#include "aptenodytes/engine.h"
#include "aptenodytes/execution_model.h"
#include "aptenodytes/metrics.h"
#include "aptenodytes/portfolio.h"
#include "aptenodytes/strategy.h"

namespace nb = nanobind;
using namespace apten;

NB_MODULE(_apten_core, m) {
    m.doc() = "Aptenodytes C++ core (nanobind)";

    nb::class_<Bar>(m, "Bar")
        .def(nb::init<>())
        .def_rw("ts_utc_ms", &Bar::ts_utc_ms)
        .def_rw("symbol", &Bar::symbol)
        .def_rw("open", &Bar::open)
        .def_rw("high", &Bar::high)
        .def_rw("low", &Bar::low)
        .def_rw("close", &Bar::close)
        .def_rw("adj_close", &Bar::adj_close)
        .def_rw("volume", &Bar::volume);

    nb::class_<Fill>(m, "Fill")
        .def(nb::init<>())
        .def_rw("ts_utc_ms", &Fill::ts_utc_ms)
        .def_rw("symbol", &Fill::symbol)
        .def_rw("quantity", &Fill::quantity)
        .def_rw("price", &Fill::price)
        .def_rw("commission", &Fill::commission);

    nb::class_<Portfolio>(m, "Portfolio")
        .def(nb::init<double>(), nb::arg("initial_cash"))
        .def_prop_ro("cash", &Portfolio::cash)
        .def_prop_ro("initial_cash", &Portfolio::initial_cash)
        .def_prop_ro("fills", &Portfolio::fills)
        .def_prop_ro("equity_curve", &Portfolio::equity_curve)
        .def("position_qty", &Portfolio::position_qty);
}
