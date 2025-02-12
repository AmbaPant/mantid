// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Common/Encoder.h"
#include "../ReflMockObjects.h"
#include "CoderCommonTester.h"
#include "MantidPythonInterface/core/WrapPython.h"

#include <cxxtest/TestSuite.h>

#include <QMap>
#include <QString>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class EncoderTest : public CxxTest::TestSuite {
public:
  static EncoderTest *createSuite() { return new EncoderTest(); }
  static void destroySuite(EncoderTest *suite) { delete suite; }

  void setUp() override { Mantid::API::AlgorithmFactory::Instance().subscribe<ReflectometryISISLoadAndProcess>(); }

  void tearDown() override {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("ReflectometryISISLoadAndProcess", 1);
  }

  void test_encoder() {
    CoderCommonTester tester;
    QtMainWindowView mwv;
    mwv.initLayout();
    Encoder encoder;
    auto map = encoder.encode(&mwv, "");

    map.insert(QString("tag"), QVariant(QString("ISIS Reflectometry")));

    tester.testMainWindowView(&mwv, map);
  }

  void test_encodeBatch() {
    CoderCommonTester tester;
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Encoder encoder;
    auto map = encoder.encodeBatch(&mwv, 0);

    tester.testBatch(gui, &mwv, map);

    TS_ASSERT(map.contains(QString("version")))
    auto constexpr expectedVersion = "1";
    TS_ASSERT_EQUALS(expectedVersion, map[QString("version")].toString().toStdString())
  }
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
