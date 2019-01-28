// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_EXPERIMENTPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_EXPERIMENTPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/Experiment/ExperimentPresenter.h"
#include "../../ReflMockObjects.h"
#include "MockExperimentView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

// The missing braces warning is a false positive -
// https://llvm.org/bugs/show_bug.cgi?id=21629
GNU_DIAG_OFF("missing-braces")

class ExperimentPresenterTest : public CxxTest::TestSuite {
  using OptionsRow = std::array<std::string, 8>;
  using OptionsTable = std::vector<OptionsRow>;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentPresenterTest *createSuite() {
    return new ExperimentPresenterTest();
  }
  static void destroySuite(ExperimentPresenterTest *suite) { delete suite; }

  ExperimentPresenterTest() : m_view() {}

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testAllWidgetsAreEnabledWhenReductionPaused() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, enableAll()).Times(1);
    presenter.reductionPaused();

    verifyAndClear();
  }

  void testAllWidgetsAreDisabledWhenReductionResumed() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableAll()).Times(1);
    presenter.reductionResumed();

    verifyAndClear();
  }

  void testModelUpdatedWhenAnalysisModeChanged() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getAnalysisMode())
        .WillOnce(Return(std::string("MultiDetectorAnalysis")));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().analysisMode(),
                     AnalysisMode::MultiDetector);
    verifyAndClear();
  }

  void testModelUpdatedWhenSummationTypeChanged() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    presenter.notifySummationTypeChanged();

    TS_ASSERT_EQUALS(presenter.experiment().summationType(),
                     SummationType::SumInQ);
    verifyAndClear();
  }

  void testSumInQWidgetsDisabledWhenChangeToSumInLambda() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableReductionType()).Times(1);
    EXPECT_CALL(m_view, disableIncludePartialBins()).Times(1);
    presenter.notifySummationTypeChanged();

    verifyAndClear();
  }

  void testSumInQWidgetsEnabledWhenChangeToSumInQ() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    EXPECT_CALL(m_view, enableReductionType()).Times(1);
    EXPECT_CALL(m_view, enableIncludePartialBins()).Times(1);
    presenter.notifySummationTypeChanged();

    verifyAndClear();
  }

  void testChangingIncludePartialBinsUpdatesModel() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    EXPECT_CALL(m_view, getIncludePartialBins()).WillOnce(Return(true));
    presenter.notifySettingsChanged();

    TS_ASSERT(presenter.experiment().includePartialBins());
    verifyAndClear();
  }

  void testChangingDebugOptionUpdatesModel() {
    auto presenter = makePresenter();

    expectViewReturnsSumInQDefaults();
    EXPECT_CALL(m_view, getDebugOption()).WillOnce(Return(true));
    presenter.notifySettingsChanged();

    TS_ASSERT(presenter.experiment().debug());
    verifyAndClear();
  }

  void testSetPolarizationCorrectionsUpdatesModel() {
    auto presenter = makePresenter();
    PolarizationCorrections polCorr(PolarizationCorrectionType::PA, 1.2, 1.3,
                                    2.4, 2.5);

    EXPECT_CALL(m_view, getPolarizationCorrectionType()).WillOnce(Return("PA"));
    EXPECT_CALL(m_view, getCRho()).WillOnce(Return(polCorr.cRho().get()));
    EXPECT_CALL(m_view, getCAlpha()).WillOnce(Return(polCorr.cAlpha().get()));
    EXPECT_CALL(m_view, getCAp()).WillOnce(Return(polCorr.cAp().get()));
    EXPECT_CALL(m_view, getCPp()).WillOnce(Return(polCorr.cPp().get()));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().polarizationCorrections(), polCorr);
    verifyAndClear();
  }

  void testSettingPolarizationCorrectionsToNoneDisablesInputs() {
    runWithPolarizationCorrectionInputsDisabled("None");
  }

  void testSetPolarizationCorrectionsToParameterFileDisablesInputs() {
    runWithPolarizationCorrectionInputsDisabled("ParameterFile");
  }

  void testSettingPolarizationCorrectionsToPAEnablesInputs() {
    runWithPolarizationCorrectionInputsEnabled("PA");
  }

  void testSettingPolarizationCorrectionsToPNREnablesInputs() {
    runWithPolarizationCorrectionInputsEnabled("PNR");
  }

  void testSetFloodCorrectionsUpdatesModel() {
    auto presenter = makePresenter();
    FloodCorrections floodCorr(FloodCorrectionType::Workspace,
                               std::string{"testWS"});

    EXPECT_CALL(m_view, getFloodCorrectionType()).WillOnce(Return("Workspace"));
    EXPECT_CALL(m_view, getFloodWorkspace())
        .WillOnce(Return(floodCorr.workspace().get()));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().floodCorrections(), floodCorr);
    verifyAndClear();
  }

  void testSetFloodCorrectionsToWorkspaceEnablesInputs() {
    runWithFloodCorrectionInputsEnabled("Workspace");
  }

  void testSetFloodCorrectionsToParameterFileDisablesInputs() {
    runWithFloodCorrectionInputsDisabled("ParameterFile");
  }

  void testSetValidTransmissionRunRange() {
    RangeInLambda range(7.2, 10);
    runTestForValidTransmissionRunRange(range, range);
  }

  void testTransmissionRunRangeIsInvalidIfStartGreaterThanEnd() {
    RangeInLambda range(10.2, 7.1);
    runTestForInvalidTransmissionRunRange(range);
  }

  void testTransmissionRunRangeIsInvalidIfZeroLength() {
    RangeInLambda range(7.1, 7.1);
    runTestForInvalidTransmissionRunRange(range);
  }

  void testTransmissionRunRangeIsValidIfStartUnset() {
    RangeInLambda range(0.0, 7.1);
    runTestForValidTransmissionRunRange(range, range);
  }

  void testTransmissionRunRangeIsValidIfEndUnset() {
    RangeInLambda range(5, 0.0);
    runTestForValidTransmissionRunRange(range, range);
  }

  void testTransmissionRunRangeIsValidButNotUpdatedIfUnset() {
    RangeInLambda range(0.0, 0.0);
    runTestForValidTransmissionRunRange(range, boost::none);
  }

  void testSetStitchOptions() {
    auto presenter = makePresenter();
    auto const optionsString = "Params=0.02";
    std::map<std::string, std::string> optionsMap = {{"Params", "0.02"}};

    EXPECT_CALL(m_view, getStitchOptions()).WillOnce(Return(optionsString));
    EXPECT_CALL(m_view, showStitchParametersValid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().stitchParameters(), optionsMap);
    verifyAndClear();
  }

  void testSetStitchOptionsInvalid() {
    auto presenter = makePresenter();
    auto const optionsString = "0.02";
    std::map<std::string, std::string> emptyOptionsMap;

    EXPECT_CALL(m_view, getStitchOptions()).WillOnce(Return(optionsString));
    EXPECT_CALL(m_view, showStitchParametersInvalid());
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.experiment().stitchParameters(),
                     emptyOptionsMap);
    verifyAndClear();
  }

  void testNewPerAngleDefaultsRequested() {
    auto presenter = makePresenter();

    // row should be added to view
    EXPECT_CALL(m_view, addPerThetaDefaultsRow());
    // new value should be requested from view to update model
    EXPECT_CALL(m_view, getPerAngleOptions()).Times(1);
    presenter.notifyNewPerAngleDefaultsRequested();

    verifyAndClear();
  }

  void testRemovePerAngleDefaultsRequested() {
    auto presenter = makePresenter();

    int const indexToRemove = 0;
    // row should be removed from view
    EXPECT_CALL(m_view, removePerThetaDefaultsRow(indexToRemove)).Times(1);
    // new value should be requested from view to update model
    EXPECT_CALL(m_view, getPerAngleOptions()).Times(1);
    presenter.notifyRemovePerAngleDefaultsRequested(indexToRemove);

    verifyAndClear();
  }

  void testChangingPerAngleDefaultsUpdatesModel() {
    auto presenter = makePresenter();

    auto const row = 1;
    auto const column = 0;
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(),
                                       optionsRowWithSecondAngle()};
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    presenter.notifyPerAngleDefaultsChanged(row, column);

    // Check the model contains the per-theta defaults returned by the view
    auto const perThetaDefaults = presenter.experiment().perThetaDefaults();
    TS_ASSERT_EQUALS(perThetaDefaults.size(), 2);
    TS_ASSERT_EQUALS(perThetaDefaults[0], defaultsWithFirstAngle());
    TS_ASSERT_EQUALS(perThetaDefaults[1], defaultsWithSecondAngle());
    verifyAndClear();
  }

  void testMultipleUniqueAnglesAreValid() {
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(),
                                       optionsRowWithSecondAngle()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testMultipleNonUniqueAnglesAreInvalid() {
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(),
                                       optionsRowWithFirstAngle()};
    runTestForNonUniqueAngles(optionsTable);
  }

  void testSingleWildcardRowIsValid() {
    OptionsTable const optionsTable = {optionsRowWithWildcard()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testAngleAndWildcardRowAreValid() {
    OptionsTable const optionsTable = {optionsRowWithFirstAngle(),
                                       optionsRowWithWildcard()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testMultipleWildcardRowsAreInvalid() {
    OptionsTable const optionsTable = {optionsRowWithWildcard(),
                                       optionsRowWithWildcard()};
    runTestForInvalidPerAngleOptions(optionsTable, {0, 1}, 0);
  }

  void testSetFirstTransmissionRun() {
    OptionsTable const optionsTable = {optionsRowWithFirstTransmissionRun()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testFirstTransmissionRunInvalid() {
    OptionsTable const optionsTable = {
        optionsRowWithFirstTransmissionRunInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0, 1);
  }

  void testSetSecondTransmissionRun() {
    OptionsTable const optionsTable = {optionsRowWithSecondTransmissionRun()};
    runTestForInvalidPerAngleOptions(optionsTable, 0, 1);
  }

  void testSecondTransmissionRunInvalid() {
    OptionsTable const optionsTable = {
        optionsRowWithSecondTransmissionRunInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0, 2);
  }

  void testSetBothTransmissionRuns() {
    OptionsTable const optionsTable = {optionsRowWithBothTransmissionRuns()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetQMin() {
    OptionsTable const optionsTable = {optionsRowWithQMin()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetQMinInvalid() {
    OptionsTable const optionsTable = {optionsRowWithQMinInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0, 3);
  }

  void testSetQMax() {
    OptionsTable const optionsTable = {optionsRowWithQMax()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetQMaxInvalid() {
    OptionsTable const optionsTable = {optionsRowWithQMaxInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0, 4);
  }

  void testSetQStep() {
    OptionsTable const optionsTable = {optionsRowWithQStep()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetQStepInvalid() {
    OptionsTable const optionsTable = {optionsRowWithQStepInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0, 5);
  }

  void testSetScale() {
    OptionsTable const optionsTable = {optionsRowWithScale()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetScaleInvalid() {
    OptionsTable const optionsTable = {optionsRowWithScaleInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0, 6);
  }

  void testSetProcessingInstructions() {
    OptionsTable const optionsTable = {optionsRowWithProcessingInstructions()};
    runTestForValidPerAngleOptions(optionsTable);
  }

  void testSetProcessingInstructionsInvalid() {
    OptionsTable const optionsTable = {
        optionsRowWithProcessingInstructionsInvalid()};
    runTestForInvalidPerAngleOptions(optionsTable, 0, 7);
  }

  void testChangingSettingsNotifiesMainPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifySettingsChanged();
    verifyAndClear();
  }

  void testChangingPerAngleDefaultsNotifiesMainPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifyPerAngleDefaultsChanged(0, 0);
    verifyAndClear();
  }

private:
  NiceMock<MockExperimentView> m_view;
  NiceMock<MockReflBatchPresenter> m_mainPresenter;
  double m_thetaTolerance{0.01};

  Experiment makeModel() {
    auto polarizationCorrections =
        PolarizationCorrections(PolarizationCorrectionType::None);
    auto floodCorrections = FloodCorrections(FloodCorrectionType::Workspace);
    auto transmissionRunRange = boost::none;
    auto stitchParameters = std::map<std::string, std::string>();
    auto perThetaDefaults = std::vector<PerThetaDefaults>();
    return Experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                      SummationType::SumInLambda, false, false,
                      std::move(polarizationCorrections),
                      std::move(floodCorrections),
                      std::move(transmissionRunRange),
                      std::move(stitchParameters), std::move(perThetaDefaults));
  }

  ExperimentPresenter makePresenter() {
    // The presenter gets values from the view on construction so the view must
    // return something sensible
    auto presenter =
        ExperimentPresenter(&m_view, makeModel(), m_thetaTolerance);
    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }

  void expectViewReturnsSumInQDefaults() {
    EXPECT_CALL(m_view, getSummationType())
        .WillOnce(Return(std::string("SumInQ")));
    EXPECT_CALL(m_view, getReductionType())
        .WillOnce(Return(std::string("DivergentBeam")));
  }

  void runWithPolarizationCorrectionInputsDisabled(std::string const &type) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getPolarizationCorrectionType()).WillOnce(Return(type));
    EXPECT_CALL(m_view, disablePolarizationCorrectionInputs()).Times(1);
    EXPECT_CALL(m_view, getCRho()).Times(0);
    EXPECT_CALL(m_view, getCAlpha()).Times(0);
    EXPECT_CALL(m_view, getCAp()).Times(0);
    EXPECT_CALL(m_view, getCPp()).Times(0);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void runWithPolarizationCorrectionInputsEnabled(std::string const &type) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getPolarizationCorrectionType()).WillOnce(Return(type));
    EXPECT_CALL(m_view, enablePolarizationCorrectionInputs()).Times(1);
    EXPECT_CALL(m_view, getCRho()).Times(1);
    EXPECT_CALL(m_view, getCAlpha()).Times(1);
    EXPECT_CALL(m_view, getCAp()).Times(1);
    EXPECT_CALL(m_view, getCPp()).Times(1);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void runWithFloodCorrectionInputsDisabled(std::string const &type) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getFloodCorrectionType()).WillOnce(Return(type));
    EXPECT_CALL(m_view, disableFloodCorrectionInputs()).Times(1);
    EXPECT_CALL(m_view, getFloodWorkspace()).Times(0);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void runWithFloodCorrectionInputsEnabled(std::string const &type) {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getFloodCorrectionType()).WillOnce(Return(type));
    EXPECT_CALL(m_view, enableFloodCorrectionInputs()).Times(1);
    EXPECT_CALL(m_view, getFloodWorkspace()).Times(1);
    presenter.notifySettingsChanged();

    verifyAndClear();
  }

  void runTestForValidTransmissionRunRange(
      RangeInLambda const &range,
      boost::optional<RangeInLambda> const &result) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStartOverlap())
        .WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getTransmissionEndOverlap())
        .WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showTransmissionRangeValid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.experiment().transmissionRunRange(), result);
    verifyAndClear();
  }

  void runTestForInvalidTransmissionRunRange(RangeInLambda const &range) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getTransmissionStartOverlap())
        .WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getTransmissionEndOverlap())
        .WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showTransmissionRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();
    TS_ASSERT_EQUALS(presenter.experiment().transmissionRunRange(),
                     boost::none);
    verifyAndClear();
  }

  // These functions create various rows in the per-theta defaults tables,
  // either as an input array of strings or an output model
  OptionsRow optionsRowWithFirstAngle() { return {"0.5", "13463", ""}; }
  PerThetaDefaults defaultsWithFirstAngle() {
    return PerThetaDefaults(0.5, std::make_pair("13463", ""), RangeInQ(),
                            boost::none, boost::none);
  }

  OptionsRow optionsRowWithSecondAngle() { return {"2.3", "13463", "13464"}; }
  PerThetaDefaults defaultsWithSecondAngle() {
    return PerThetaDefaults(2.3, std::make_pair("13463", "13464"), RangeInQ(),
                            boost::none, boost::none);
  }
  OptionsRow optionsRowWithWildcard() { return {"", "13463", "13464"}; }
  OptionsRow optionsRowWithFirstTransmissionRun() { return {"", "13463"}; }
  OptionsRow optionsRowWithFirstTransmissionRunInvalid() { return {"", "bad"}; }
  OptionsRow optionsRowWithSecondTransmissionRun() { return {"", "", "13464"}; }
  OptionsRow optionsRowWithSecondTransmissionRunInvalid() {
    return {"", "", "bad"};
  }
  OptionsRow optionsRowWithBothTransmissionRuns() {
    return {"", "13463", "13464"};
  }
  OptionsRow optionsRowWithQMin() { return {"", "", "", "0.008"}; }
  OptionsRow optionsRowWithQMinInvalid() { return {"", "", "", "bad"}; }
  OptionsRow optionsRowWithQMax() { return {"", "", "", "", "0.1"}; }
  OptionsRow optionsRowWithQMaxInvalid() { return {"", "", "", "", "bad"}; }
  OptionsRow optionsRowWithQStep() { return {"", "", "", "", "", "0.02"}; }
  OptionsRow optionsRowWithQStepInvalid() {
    return {"", "", "", "", "", "bad"};
  }
  OptionsRow optionsRowWithScale() { return {"", "", "", "", "", "", "1.4"}; }
  OptionsRow optionsRowWithScaleInvalid() {
    return {"", "", "", "", "", "", "bad"};
  }
  OptionsRow optionsRowWithProcessingInstructions() {
    return {"", "", "", "", "", "", "", "1-4"};
  }
  OptionsRow optionsRowWithProcessingInstructionsInvalid() {
    return {"", "", "", "", "", "", "", "bad"};
  }

  void runTestForValidPerAngleOptions(OptionsTable const &optionsTable) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    EXPECT_CALL(m_view, showAllPerAngleOptionsAsValid()).Times(1);
    presenter.notifyPerAngleDefaultsChanged(1, 1);
    verifyAndClear();
  }

  void runTestForInvalidPerAngleOptions(OptionsTable const &optionsTable,
                                        std::vector<int> rows, int column) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    for (auto row : rows)
      EXPECT_CALL(m_view, showPerAngleOptionsAsInvalid(row, column)).Times(1);
    presenter.notifyPerAngleDefaultsChanged(1, 1);
    verifyAndClear();
  }

  void runTestForInvalidPerAngleOptions(OptionsTable const &optionsTable,
                                        int row, int column) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    EXPECT_CALL(m_view, showPerAngleOptionsAsInvalid(row, column)).Times(1);
    presenter.notifyPerAngleDefaultsChanged(1, 1);
    verifyAndClear();
  }

  void runTestForNonUniqueAngles(OptionsTable const &optionsTable) {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    EXPECT_CALL(m_view, showPerAngleThetasNonUnique(m_thetaTolerance)).Times(1);
    presenter.notifyPerAngleDefaultsChanged(0, 0);
    verifyAndClear();
  }
};

GNU_DIAG_ON("missing-braces")

#endif // MANTID_CUSTOMINTERFACES_EXPERIMENTPRESENTERTEST_H_
