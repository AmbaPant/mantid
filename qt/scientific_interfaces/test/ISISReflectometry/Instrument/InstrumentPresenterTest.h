// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INSTRUMENTPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_INSTRUMENTPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/Instrument/InstrumentPresenter.h"
#include "MockInstrumentView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class InstrumentPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentPresenterTest *createSuite() {
    return new InstrumentPresenterTest();
  }
  static void destroySuite(InstrumentPresenterTest *suite) { delete suite; }

  InstrumentPresenterTest() : m_view() {}

  void testSetValidWavelengthRange() {
    auto const range = RangeInLambda(1.5, 14);
    runTestForValidWavelengthRange(range, range);
  }

  void testWavelengthRangeIsInvalidIfStartGreaterThanEnd() {
    auto const range = RangeInLambda(7.5, 2);
    runTestForInvalidWavelengthRange(range);
  }

  void testWavelengthRangeIsInvalidIfZeroLength() {
    auto const range = RangeInLambda(7.5, 7.5);
    runTestForInvalidWavelengthRange(range);
  }

  void testWavelengthRangeIsValidIfStartUnset() {
    auto const range = RangeInLambda(0.0, 7.5);
    runTestForValidWavelengthRange(range, range);
  }

  void testWavelengthRangeIsValidIfEndUnset() {
    auto const range = RangeInLambda(7.5, 0.0);
    runTestForValidWavelengthRange(range, range);
  }

  void testWavelengthRangeIsValidButNotUpdatedIfUnset() {
    auto const range = RangeInLambda(0.0, 0.0);
    runTestForValidWavelengthRange(range, boost::none);
  }

  void testIntegratedMonitorsToggled() {
    auto presenter = makePresenter();
    auto const integrate = !presenter.instrument().integratedMonitors();

    EXPECT_CALL(m_view, getIntegrateMonitors()).WillOnce(Return(integrate));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().integratedMonitors(), integrate);
    verifyAndClear();
  }

  void testSetMonitorIndex() {
    auto presenter = makePresenter();
    auto const monitorIndex = 3;

    EXPECT_CALL(m_view, getMonitorIndex()).WillOnce(Return(monitorIndex));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().monitorIndex(), monitorIndex);
    verifyAndClear();
  }

  void testSetValidMonitorIntegralRange() {
    auto const range = RangeInLambda(3.4, 12.2);
    runTestForValidMonitorIntegralRange(range, range);
  }

  void testMonitorIntegralRangeIsInvalidIfStartGreaterThanEnd() {
    auto const range = RangeInLambda(7.5, 4);
    runTestForInvalidMonitorIntegralRange(range);
  }

  void testMonitorIntegralRangeIsInvalidIfZeroLength() {
    auto const range = RangeInLambda(7.5, 7.5);
    runTestForInvalidMonitorIntegralRange(range);
  }

  void testMonitorIntegralRangeIsValidIfStartUnset() {
    auto const range = RangeInLambda(0.0, 4.5);
    runTestForValidMonitorIntegralRange(range, range);
  }

  void testMonitorIntegralRangeIsValidIfEndUnset() {
    auto const range = RangeInLambda(4.5, 0.0);
    runTestForValidMonitorIntegralRange(range, range);
  }

  void testMonitorIntegralRangeIsValidButNotUpdatedIfUnset() {
    auto const range = RangeInLambda(0.0, 0.0);
    runTestForValidMonitorIntegralRange(range, boost::none);
  }

  void testSetValidMonitorBackgroundRange() {
    auto const range = RangeInLambda(2.0, 13);
    runTestForValidMonitorBackgroundRange(range, range);
  }

  void testMonitorBackgroundRangeIsInvalidIfStartGreaterThanEnd() {
    auto const range = RangeInLambda(3.5, 3.4);
    runTestForInvalidMonitorBackgroundRange(range);
  }

  void testMonitorBackgroundRangeIsInvalidIfZeroLength() {
    auto const range = RangeInLambda(2.0, 2.0);
    runTestForInvalidMonitorBackgroundRange(range);
  }

  void testMonitorBackgroundRangeIsInvalidIfOnlyStartSet() {
    auto const range = RangeInLambda(2.001, 0.0);
    runTestForInvalidMonitorBackgroundRange(range);
  }

  void testMonitorBackgroundRangeIsInvalidIfOnlyEndSet() {
    auto const range = RangeInLambda(0.0, 7.8);
    runTestForInvalidMonitorBackgroundRange(range);
  }

  void testMonitorBackgroundRangeIsValidButNotUpdatedIfUnset() {
    auto const range = RangeInLambda(0.0, 0.0);
    runTestForValidMonitorBackgroundRange(range, boost::none);
  }

  void testCorrectDetectorsToggledUpdatesModel() {
    auto presenter = makePresenter();
    auto const correctDetectors = !presenter.instrument().correctDetectors();

    EXPECT_CALL(m_view, getCorrectDetectors())
        .WillOnce(Return(correctDetectors));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().correctDetectors(),
                     correctDetectors);
    verifyAndClear();
  }

  void testEnablingCorrectDetectorsEnablesCorrectionType() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getCorrectDetectors()).WillOnce(Return(true));
    EXPECT_CALL(m_view, enableDetectorCorrectionType()).Times(1);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void testDiablingCorrectDetectorsDisablesCorrectionType() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getCorrectDetectors()).WillOnce(Return(false));
    EXPECT_CALL(m_view, disableDetectorCorrectionType()).Times(1);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void testSetDetectorCorrectionTypeUpdatesModel() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getDetectorCorrectionType())
        .WillOnce(Return("RotateAroundSample"));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().detectorCorrectionType(),
                     DetectorCorrectionType::RotateAroundSample);
    verifyAndClear();
  }

  void testAllWidgetsAreEnabledWhenReductionPaused() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, enableAll()).Times(1);
    presenter.onReductionPaused();

    verifyAndClear();
  }

  void testAllWidgetsAreDisabledWhenReductionResumed() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableAll()).Times(1);
    presenter.onReductionResumed();

    verifyAndClear();
  }

private:
  NiceMock<MockInstrumentView> m_view;

  Instrument makeModel() {
    auto wavelengthRange = RangeInLambda(0.0, 0.0);
    auto monitorCorrections = MonitorCorrections(
        0, true, RangeInLambda(0.0, 0.0), RangeInLambda(0.0, 0.0));
    auto detectorCorrections =
        DetectorCorrections(false, DetectorCorrectionType::VerticalShift);
    return Instrument(wavelengthRange, monitorCorrections, detectorCorrections);
  }

  InstrumentPresenter makePresenter() {
    return InstrumentPresenter(&m_view, makeModel());
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }

  void
  runTestForValidWavelengthRange(RangeInLambda const &range,
                                 boost::optional<RangeInLambda> const &result) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getLambdaMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getLambdaMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showLambdaRangeValid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().wavelengthRange(), result);
    verifyAndClear();
  }

  void runTestForInvalidWavelengthRange(RangeInLambda const &range) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getLambdaMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getLambdaMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showLambdaRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().wavelengthRange(), boost::none);
    verifyAndClear();
  }

  void runTestForValidMonitorIntegralRange(
      RangeInLambda const &range,
      boost::optional<RangeInLambda> const &result) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getMonitorIntegralMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorIntegralMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorIntegralRangeValid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().monitorIntegralRange(), result);
    verifyAndClear();
  }

  void runTestForInvalidMonitorIntegralRange(RangeInLambda const &range) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getMonitorIntegralMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorIntegralMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorIntegralRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().monitorIntegralRange(),
                     boost::none);
    verifyAndClear();
  }

  void runTestForValidMonitorBackgroundRange(
      RangeInLambda const &range,
      boost::optional<RangeInLambda> const &result) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getMonitorBackgroundMin())
        .WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorBackgroundMax())
        .WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorBackgroundRangeValid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().monitorBackgroundRange(), result);
    verifyAndClear();
  }

  void runTestForInvalidMonitorBackgroundRange(RangeInLambda const &range) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getMonitorBackgroundMin())
        .WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorBackgroundMax())
        .WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorBackgroundRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.instrument().monitorBackgroundRange(),
                     boost::none);
    verifyAndClear();
  }
};

#endif // MANTID_CUSTOMINTERFACES_INSTRUMENTPRESENTERTEST_H_
