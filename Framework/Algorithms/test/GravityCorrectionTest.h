// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GRAVITYCORRECTIONTEST_H
#define GRAVITYCORRECTIONTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/GravityCorrection.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;

class GravityCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GravityCorrectionTest *createSuite() { return new GravityCorrectionTest(); }
  static void destroySuite(GravityCorrectionTest *suite) { delete suite; }

  void testName() {
    GravityCorrection gc0;
    TS_ASSERT_EQUALS(gc0.name(), "GravityCorrection")
  }

  void testCategory() {
    GravityCorrection gc1;
    TS_ASSERT_EQUALS(gc1.category(), "ILL\\Reflectometry;Reflectometry")
  }

  void testInit() {
    GravityCorrection gc2;
    TS_ASSERT_THROWS_NOTHING(gc2.initialize())
    gc2.setRethrows(true);
    TS_ASSERT(gc2.isInitialized())
  }

  void testInvalidSlitName() {
    GravityCorrection gc6;
    TS_ASSERT_THROWS_NOTHING(gc6.initialize())
    gc6.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(gc6.setProperty("InputWorkspace", inWS1))
    TS_ASSERT_THROWS_NOTHING(gc6.setProperty("OutputWorkspace", "out1"))
    TSM_ASSERT_THROWS_NOTHING("FirstSlitName slitt does not exist", gc6.setProperty("FirstSlitName", "slitt"))
    TS_ASSERT_THROWS_ANYTHING(gc6.execute())
    TS_ASSERT(!gc6.isExecuted())
  }

  void testReplaceInputWS() {
    // OutputWorkspace should replace the InputWorkspace
    GravityCorrection gc31;
    this->runGravityCorrection(gc31, inWS1, "myOutput1");

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().addOrReplace("myOutput2", inWS1))

    GravityCorrection gc30;
    TS_ASSERT_THROWS_NOTHING(gc30.initialize())
    gc30.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(gc30.setProperty("InputWorkspace", "myOutput2"))
    TS_ASSERT_THROWS_NOTHING(gc30.setProperty("OutputWorkspace", "myOutput2"))
    TS_ASSERT_THROWS_NOTHING(gc30.execute())
    TS_ASSERT(gc30.isExecuted())

    CompareWorkspaces replace;
    compare(replace, "myOutput1", "myOutput2", "1", "1", "1");
  }

  void testSlitPosDiffers() {
    Mantid::Kernel::V3D slit = Mantid::Kernel::V3D(2., 0., 0.);

    Mantid::API::MatrixWorkspace_sptr ws1{WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
        0.5, slit, slit, 0.2, 0.2, source, monitor, sample, detector)};
    GravityCorrection gc21;
    TS_ASSERT_THROWS_NOTHING(gc21.initialize())
    gc21.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(gc21.setProperty("InputWorkspace", ws1))
    TS_ASSERT_THROWS_NOTHING(gc21.setProperty("OutputWorkspace", "ws1out"))
    TSM_ASSERT_THROWS_NOTHING("Position of slits must differ", gc21.setProperty("SecondSlitName", "slit2"))
    TS_ASSERT_THROWS_ANYTHING(gc21.execute())
    TS_ASSERT(!gc21.isExecuted())
  }

  void testBeamDirectionInvariant() {
    GravityCorrection gc4;
    this->runGravityCorrection(gc4, inWS1, "outWSName1");

    GravityCorrection gc5;
    this->runGravityCorrection(gc5, inWS1, "outWSName2");

    const std::string dataCheck = "1";
    const std::string instrumentCheck = "0";
    const std::string axesCheck = "1";
    CompareWorkspaces beamInvariant;
    compare(beamInvariant, "outWSName1", "outWSName2", dataCheck, instrumentCheck, axesCheck);
  }

  void testSlitInputInvariant() {
    // First algorithm run
    GravityCorrection gc7;
    this->runGravityCorrection(gc7, inWS1, "out1", "slit1", "slit2");
    // Second algorithm run
    GravityCorrection gc8;
    this->runGravityCorrection(gc8, inWS1, "out2", "slit2", "slit1");
    // Output workspace comparison
    CompareWorkspaces slitInvariant1;
    compare(slitInvariant1, "out1", "out2", "1");
  }

  void testInstrumentUnchanged() {
    GravityCorrection gc9;
    this->runGravityCorrection(gc9, inWS1, outWSName);
    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().add(inWSName, inWS1))
    CompareWorkspaces instrumentNotModified;
    compare(instrumentNotModified, inWS1->getName(), outWSName, "0");
    if (instrumentNotModified.getPropertyValue("Result") == "0") {
      // check explicitly that the messages are only for data!
      Mantid::API::ITableWorkspace_sptr table =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::ITableWorkspace>("compare_msgs");
      TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Data mismatch")
      TS_ASSERT_THROWS_ANYTHING(table->cell<std::string>(1, 0))
    }
    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().clear())
  }

  void testBinMask() {
    Mantid::API::MatrixWorkspace_sptr ws1{WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument()};
    std::map<size_t, double> masks;
    masks[4] = 0.4;
    masks[52] = 1.;
    masks[53] = 0.8;
    const std::map<size_t, double> mask = masks;
    ws1->setMaskedBins(0, mask);
    TS_ASSERT_EQUALS(mask.size(), 3)
    Mantid::API::MatrixWorkspace::MaskList mList0 = ws1->maskedBins(0);
    GravityCorrection gc10;
    const auto ws2 = this->runGravityCorrection(gc10, ws1, "ws2");
    TS_ASSERT_EQUALS(ws1->blocksize(), ws2->blocksize())
    Mantid::API::MatrixWorkspace::MaskList mList = ws2->maskedBins(0);
    TS_ASSERT_EQUALS(mList0.size(), mList.size())
    auto val = masks.begin();
    for (auto &list : mList)
      TS_ASSERT_EQUALS(list.second, (val++)->second)
    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().clear())
  }

  void testMonitor() {
    GravityCorrection gc12;
    const auto ws2 = this->runGravityCorrection(gc12, inWS1, "out1");
    // spectrum 1 is a monitor, compare input and output spectrum 1
    for (size_t i = 0; i < ws2->blocksize(); ++i) {
      TS_ASSERT_EQUALS(ws2->x(1)[i], inWS1->x(1)[i])
      TS_ASSERT_EQUALS(ws2->y(1)[i], inWS1->y(1)[i])
      TS_ASSERT_EQUALS(ws2->e(1)[i], inWS1->e(1)[i])
    }
  }

  void testSizes() {
    GravityCorrection gc13;
    const auto ws3 = this->runGravityCorrection(gc13, inWS1, "out1");
    TSM_ASSERT_EQUALS("Number indexable items", ws3->size(), inWS1->size())
    TSM_ASSERT_EQUALS("Number of bins", ws3->blocksize(), inWS1->blocksize())
    TSM_ASSERT_EQUALS("Number of spectra", ws3->getNumberHistograms(), inWS1->getNumberHistograms())
  }

  void testInstrumentRotation() {
    // a rotation of the instrument should not vary the output of the gravity
    // correction. but only if the rotation angle is given as input.

    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    std::vector<std::string> componentNames = {"source", "some-surface-holder", "slit1", "slit2"};
    for (const auto &component : componentNames) {
      const auto ID = ws->getInstrument()->getComponentByName(component)->getComponentID();
      double x = ws->getInstrument()->getComponentByName(component)->getPos().X();
      // new rotation
      Mantid::Kernel::Quat rot = Mantid::Kernel::Quat(30., Mantid::Kernel::V3D(0., 1., 0.)) *
                                 ws->getInstrument()->getComponentByName(component)->getRotation();
      ws->mutableComponentInfo().setRotation(ws->mutableComponentInfo().indexOf(ID), rot);
      // new position
      Mantid::Kernel::V3D pos{cos(30.) * x, sin(30.) * x, 0.};
      ws->mutableComponentInfo().setPosition(ws->mutableComponentInfo().indexOf(ID), pos);
    }

    // sample should not be at (15., 0., 0.) position test:
    TS_ASSERT_DIFFERS(ws->getInstrument()->getSample()->getPos(), Mantid::Kernel::V3D(15., 0., 0.))

    GravityCorrection gc16;
    this->runGravityCorrection(gc16, ws, "out1", "slit1",
                               "slit2"); // angle not an input anymore

    GravityCorrection gc17;
    this->runGravityCorrection(gc17, inWS1, "out2");

    CompareWorkspaces rotatedWS;
    compare(rotatedWS, "out1", "out2", "1", "0", "1");
    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().clear())
  }

  void testInstrumentTranslationInBeamDirection() {
    Mantid::Kernel::V3D translate = Mantid::Kernel::V3D(2.9, 0., 0.);
    auto origin = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(0.0, s1, s2, 0.5, 1.0, source,
                                                                                        monitor, sample, detector);
    auto translated = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
        0.0, s1 - translate, s2 - translate, 0.5, 1.0, source - translate, monitor - translate, sample - translate,
        detector - translate);

    GravityCorrection gc18;
    this->runGravityCorrection(gc18, origin, "origin");

    GravityCorrection gc19;
    this->runGravityCorrection(gc19, translated, "translated");

    // Data and x axis (TOF) must be identical
    CompareWorkspaces translatedWS;
    compare(translatedWS, "origin", "translated", "1", "0", "1");
    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().clear())
  }

  void testInstrumentTranslationGeneral() {
    Mantid::Kernel::V3D translate = Mantid::Kernel::V3D(2.9, 2.2, 1.1);
    auto origin = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(0.0, s1, s2, 0.5, 1.0, source,
                                                                                        monitor, sample, detector);
    auto translated = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
        0.0, s1 - translate, s2 - translate, 0.5, 1.0, source - translate, monitor - translate, sample - translate,
        detector - translate);

    GravityCorrection gc18;
    this->runGravityCorrection(gc18, origin, "origin");

    GravityCorrection gc19;
    this->runGravityCorrection(gc19, translated, "translated");

    // Data and x axis (TOF) must be identical
    CompareWorkspaces translatedWS;
    compare(translatedWS, "origin", "translated", "1", "0", "1");
    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().clear())
  }

  void testDx() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();
    auto dx = Mantid::Kernel::make_cow<Mantid::HistogramData::HistogramDx>(
        ws->y(0).size(), Mantid::HistogramData::LinearGenerator(0.1, 0.33));
    ws->setSharedDx(0, dx);
    GravityCorrection gc23;
    const auto out = this->runGravityCorrection(gc23, ws, "hasDx");
    TS_ASSERT_EQUALS(out->hasDx(0), ws->hasDx(0))
    // TODO: WorkspaceCreation creates dx for all spectra if spectrum 0 has dx
    TS_ASSERT_EQUALS(out->hasDx(1), !ws->hasDx(1))
  }

  // Real data tests
  // Use of slit1 and slit2 default values from sample logs
  // Example: FIGARO parameter file defines slit1 and slit2

  void testInputWorkspace2D_1() {
    TS_ASSERT_THROWS_NOTHING(Mantid::API::FrameworkManager::Instance().exec(
        "LoadILLReflectometry", 6, "Filename", "ILL/Figaro/000002", "OutputWorkspace", "ws", "XUnit", "TimeOfFlight");)
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("ws"));
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("ws");
    GravityCorrection gc00;
    Mantid::API::MatrixWorkspace_const_sptr corrected = this->runGravityCorrection(gc00, ws, "OutputWorkspace");
    Mantid::API::MatrixWorkspace_const_sptr cws =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("ws");
    this->noLossOfCounts(cws, corrected);
    // this->notCommonBinCheck(cws, corrected);
    // TOF values, first spectrum
    const auto x1 = ws->x(0);
    const auto x2 = corrected->x(0);
    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().clear())
  }

  void testInputWorkspace2D_2() {
    TS_ASSERT_THROWS_NOTHING(Mantid::API::FrameworkManager::Instance().exec("LoadILLReflectometry", 6, "Filename",
                                                                            "ILL/Figaro/592724.nxs", "OutputWorkspace",
                                                                            "ws", "XUnit", "TimeOfFlight");)
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("ws"));
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("ws");
    GravityCorrection gc00;
    Mantid::API::MatrixWorkspace_const_sptr corrected = this->runGravityCorrection(gc00, ws, "OutputWorkspace");
    Mantid::API::MatrixWorkspace_const_sptr cws =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("ws");
    this->noLossOfCounts(cws, corrected);
    // this->notCommonBinCheck(cws, corrected);
    // TOF values, first spectrum
    const auto x1 = cws->x(0);
    const auto x2 = corrected->x(0);
    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().clear())
  }

  Mantid::API::MatrixWorkspace_const_sptr
  runGravityCorrection(GravityCorrection &gravityCorrection, Mantid::API::MatrixWorkspace_sptr &inWS,
                       const std::string outName, std::string firstSlitName = "", std::string secondSlitName = "") {
    TS_ASSERT_THROWS_NOTHING(gravityCorrection.initialize())
    gravityCorrection.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(gravityCorrection.setProperty("InputWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(gravityCorrection.setProperty("OutputWorkspace", outName))
    if (!firstSlitName.empty())
      TSM_ASSERT_THROWS_NOTHING("FirstSlitName should be slit2",
                                gravityCorrection.setProperty("FirstSlitName", firstSlitName))
    if (!secondSlitName.empty())
      TSM_ASSERT_THROWS_NOTHING("SecondSlitName should be slit1",
                                gravityCorrection.setProperty("SecondSlitName", secondSlitName))
    TS_ASSERT_THROWS_NOTHING(gravityCorrection.execute())
    TS_ASSERT(gravityCorrection.isExecuted())
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(outName))
    return Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(outName);
  }

  void notCommonBinCheck(Mantid::API::MatrixWorkspace_const_sptr &ws1, Mantid::API::MatrixWorkspace_const_sptr &ws2) {
    // Typically bin edges changed after running the algorithm
    for (size_t i = 0; i < ws1->getNumberHistograms(); ++i) {
      const auto x1 = ws1->x(i);
      const auto x2 = ws2->x(i);
      TS_ASSERT_DIFFERS(x1, x2)
    }
  }

  void noLossOfCounts(Mantid::API::MatrixWorkspace_const_sptr &ws1, Mantid::API::MatrixWorkspace_const_sptr &ws2) {
    double totalCounts{0.}, totalCountsCorrected{0.};
    for (size_t i = 0; i < ws1->getNumberHistograms(); i++) {
      for (size_t k = 0; k < ws1->blocksize(); k++) {
        totalCounts += ws1->y(i)[k];
        totalCountsCorrected += ws2->y(i)[k];
      }
    }
  }

  void compare(CompareWorkspaces &compare, const std::string &in1, const std::string &in2,
               const std::string property_value, std::string property_instrument = "1",
               std::string property_axes = "0") {
    // Output workspace comparison
    compare.initialize();
    TS_ASSERT_THROWS_NOTHING(compare.setPropertyValue("Workspace1", in1))
    compare.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace2", in2))
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("CheckInstrument", property_instrument))
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("CheckAxes", property_axes))
    TS_ASSERT(compare.execute())
    TS_ASSERT(compare.isExecuted())
    TS_ASSERT_EQUALS(compare.getPropertyValue("Result"), property_value)
  }

private:
  const std::string outWSName{"GravityCorrectionTest_OutputWorkspace"};
  const std::string inWSName{"GravityCorrectionTest_InputWorkspace"};
  Mantid::Kernel::V3D source{Mantid::Kernel::V3D(0., 0., 0.)};
  Mantid::Kernel::V3D monitor{Mantid::Kernel::V3D(0.5, 0., 0.)};
  Mantid::Kernel::V3D s1{Mantid::Kernel::V3D(1., 0., 0.)};
  Mantid::Kernel::V3D s2{Mantid::Kernel::V3D(2., 0., 0.)};
  Mantid::Kernel::V3D sample{Mantid::Kernel::V3D(3., 0., 0.)};
  Mantid::Kernel::V3D detector{Mantid::Kernel::V3D(4., 4., 0.)};
  Mantid::API::MatrixWorkspace_sptr inWS1{WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
      0.0, s1, s2, 0.5, 1.0, source, monitor, sample, detector, 100, 2000.0)};
  Mantid::API::MatrixWorkspace_sptr inWS3{
      WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrumentMultiDetector(
          0.5, 0.25, Mantid::Kernel::V3D(-3., 40., 0.), Mantid::Kernel::V3D(-2., 29.669, 0.), 0.2, 0.3,
          Mantid::Kernel::V3D(-5.94366667, 52.99776017, 0.), Mantid::Kernel::V3D(1., 0., 0.),
          Mantid::Kernel::V3D(0., 0., 0.), Mantid::Kernel::V3D(0.854, 35.73, 0.), 4, 50, 0.02)};
};

// Performance testing
class GravityCorrectionTestPerformance : public CxxTest::TestSuite {
public:
  static GravityCorrectionTestPerformance *createSuite() { return new GravityCorrectionTestPerformance(); }
  static void destroySuite(GravityCorrectionTestPerformance *suite) { delete suite; }

  GravityCorrectionTestPerformance() {
    Mantid::API::FrameworkManager::Instance().exec("LoadILLReflectometry", 6, "Filename", "ILL/Figaro/592724.nxs",
                                                   "OutputWorkspace", "ws", "XUnit", "TimeOfFlight");
    alg.initialize();
    alg.setProperty("InputWorkspace", "ws");
    alg.setProperty("OutputWorkspace", "anon");
  }

  void test_performace() {
    int i = 0;
    while (i < 10) {
      alg.execute();
      ++i;
    }
  }

private:
  GravityCorrection alg;
};

#endif // GRAVITYCORRECTIONTEST_H
