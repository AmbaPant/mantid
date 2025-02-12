// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

using MantidQt::Widgets::MplCpp::Figure;
using MantidQt::Widgets::MplCpp::FigureCanvasQt;

class FigureCanvasQtTest : public CxxTest::TestSuite {
public:
  static FigureCanvasQtTest *createSuite() { return new FigureCanvasQtTest; }
  static void destroySuite(FigureCanvasQtTest *suite) { delete suite; }

public:
  void testConstructionYieldsExpectedSubplot() {
    FigureCanvasQt canvas{111};
    auto geometry = canvas.gca().pyobj().attr("get_geometry")();
    TS_ASSERT_EQUALS(1, geometry[0]);
    TS_ASSERT_EQUALS(1, geometry[1]);
    TS_ASSERT_EQUALS(1, geometry[2]);
  }

  void testConstructionCapturesGivenAxesObject() {
    Figure fig;
    fig.addSubPlot(221);
    FigureCanvasQt canvas{std::move(fig)};
    auto geometry = canvas.gca().pyobj().attr("get_geometry")();
    TS_ASSERT_EQUALS(2, geometry[0]);
    TS_ASSERT_EQUALS(2, geometry[1]);
    TS_ASSERT_EQUALS(1, geometry[2]);
  }

  void testToDataCoordinatesReturnsExpectedPoint() {
    FigureCanvasQt canvas{111};
    canvas.gca().plot({1, 2, 3, 4, 5}, {1, 2, 3, 4, 5});
    canvas.draw(); // We now need to draw as 3.2 changed when autoscaling occurs in plots, to only when drawn, not
                   // when a plot function is ran.

    auto dataCoords =
        canvas.toDataCoords(QPoint(static_cast<int>(canvas.width() * 0.5), static_cast<int>(canvas.height() * 0.25)));
    TS_ASSERT_DELTA(2.9, dataCoords.x(), 0.25);
    TS_ASSERT_DELTA(4.25, dataCoords.y(), 0.25);
  }

  void testAddLegend() {
    FigureCanvasQt canvas{111};
    // This return is needed for compatability with mpl 1.5 so that Line2D
    // object reference is kept alive.
    auto line = canvas.gca().plot({1, 2, 3, 4, 5}, {1, 2, 3, 4, 5}, QString("ro"), QString("Line1"));
    auto legend = canvas.gca().legend(true);

    if (PyObject_HasAttrString(legend.pyobj().ptr(), "get_draggable"))
      TS_ASSERT_EQUALS(true, legend.pyobj().attr("get_draggable")());
  }
};
