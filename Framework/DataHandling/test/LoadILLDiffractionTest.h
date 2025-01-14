// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/LoadILLDiffraction.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"
#include "MantidTypes/Core/DateAndTimeHelpers.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadILLDiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLDiffractionTest *createSuite() { return new LoadILLDiffractionTest(); }
  static void destroySuite(LoadILLDiffractionTest *suite) { delete suite; }

  LoadILLDiffractionTest() {
    ConfigService::Instance().appendDataSearchSubDir("ILL/D20/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D2B/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D1B/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/IN5/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/PANTHER/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/SHARP/");
  }

  void setUp() override {
    m_oldFacility = ConfigService::Instance().getFacility().name();
    ConfigService::Instance().setFacility("ILL");

    m_oldInstrument = ConfigService::Instance().getInstrument().name();
    ConfigService::Instance().setString("default.instrument", "");
  }

  void tearDown() override {
    if (!m_oldFacility.empty()) {
      ConfigService::Instance().setFacility(m_oldFacility);
    }
    if (!m_oldInstrument.empty()) {
      ConfigService::Instance().setString("default.instrument", m_oldInstrument);
    }
  }

  void test_Init() {
    LoadILLDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_D20_transposed_2theta() {
    // Tests the axis conversion and transposition
    // for non-detector scan D20 data from cycle 203
    LoadILLDiffraction alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "170607.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertAxisAndTranspose", true))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 3072)
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "Degrees")
    checkTimeFormat(outputWS);
  }

  void test_D20_no_scan() {
    // Tests the no-scan case for D20
    // Temperature ramp is not a motor scan so produces a file per T

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "967100.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DataType", "Raw"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 3073)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())

    // two theta of the first pixel
    TS_ASSERT_DELTA(outputWS->detectorInfo().signedTwoTheta(1) * RAD_2_DEG, -2.79662, 1E-5)

    TS_ASSERT_EQUALS(outputWS->x(0)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 2685529.)
    TS_ASSERT_DELTA(outputWS->e(0)[0], 1638.75, 0.01)

    TS_ASSERT_EQUALS(outputWS->x(1)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->e(1)[0], 0.)

    TS_ASSERT_EQUALS(outputWS->x(64)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(64)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->e(64)[0], 0.)

    TS_ASSERT_EQUALS(outputWS->x(65)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(65)[0], 548.)
    TS_ASSERT_DELTA(outputWS->e(65)[0], 23.4, 0.01)

    TS_ASSERT_EQUALS(outputWS->x(1111)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(1111)[0], 6285)
    TS_ASSERT_DELTA(outputWS->e(1111)[0], 79.27, 0.01)

    TS_ASSERT_EQUALS(outputWS->x(3072)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(3072)[0], 7848.)
    TS_ASSERT_DELTA(outputWS->e(3072)[0], 88.58, 0.01)

    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("simulated_d20.TotalCount"))
    TS_ASSERT(run.hasProperty("AcquisitionSpy.Time"))
    TS_ASSERT(run.hasProperty("SampleSettings.SampleTemp"))
    TS_ASSERT(run.hasProperty("ScanType"))
    TS_ASSERT(run.hasProperty("PixelSize"))
    TS_ASSERT(run.hasProperty("ResolutionMode"))
    TS_ASSERT(run.hasProperty("Ei"))

    const auto sim = run.getLogData("simulated_d20.TotalCount");
    const auto spy = run.getLogData("AcquisitionSpy.Time");
    const auto sample = run.getLogData("SampleSettings.SampleTemp");
    const auto scanType = run.getLogData("ScanType");
    const double pixelSize = run.getLogAsSingleValue("PixelSize");
    const auto resMode = run.getLogData("ResolutionMode");
    const auto ei = run.getLogAsSingleValue("Ei");

    TS_ASSERT_EQUALS(scanType->value(), "NoScan")
    TS_ASSERT_EQUALS(resMode->value(), "Nominal")
    TS_ASSERT_DELTA(pixelSize, 0.05, 1E-10)

    TS_ASSERT_EQUALS(sim->size(), 1)
    TS_ASSERT_EQUALS(spy->size(), 1)
    TS_ASSERT_EQUALS(sample->size(), 1)

    TS_ASSERT_EQUALS(sim->value(), "2017-May-15 14:36:18  5.44174e+06\n")
    TS_ASSERT_EQUALS(spy->value(), "2017-May-15 14:36:18  240\n")
    TS_ASSERT_EQUALS(sample->value(), "2017-May-15 14:36:18  4.9681\n")

    TS_ASSERT_DELTA(ei, 14.09, 0.01)
    TS_ASSERT_EQUALS(outputWS->run().getProperty("Detector.calibration_file")->value(), "none")
    checkTimeFormat(outputWS);
  }

  void test_D20_no_scan_requesting_calibrated_throws() {
    // Tests the no-scan case for D20
    // Temperature ramp is not a motor scan so produces a file per T

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "967100.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DataType", "Calibrated"))
    std::string err_msg("Some invalid Properties found: \n"
                        " DataType: Calibrated data requested, but only raw data exists in this NeXus file.");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::runtime_error & e, std::string(e.what()), err_msg);
  }

  void test_D20_scan() {
    // Tests the omega scanned case for D20
    // Omega scan is a motor scan, so it is recorded in a single file
    // But it is not a detector scan within our context

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    // Note that, this is the older type of file, and is modified manually
    // to match the final configuration having the custom NX_class attribute
    // So this will not run with generic Load
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "000017.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 3073)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 21)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())

    for (size_t row = 0; row < 10; ++row) {
      for (size_t col = 0; col < 21; ++col) {
        double val = static_cast<double>(col);
        TS_ASSERT_EQUALS(outputWS->y(row)[col], 3. * (val + 1))
        TS_ASSERT_EQUALS(outputWS->x(row)[col], 1. + 0.2 * val)
        TS_ASSERT_EQUALS(outputWS->e(row)[col], sqrt(3. * (val + 1)))
      }
    }

    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("ScanType"))
    TS_ASSERT(run.hasProperty("ScanVar"))
    TS_ASSERT(outputWS->run().hasProperty("omega.position"))
    TS_ASSERT(outputWS->run().hasProperty("detector.totalcount"))
    TS_ASSERT(outputWS->run().hasProperty("acquisitionspy.time"))
    TS_ASSERT(outputWS->run().hasProperty("samplesettings.sampletemp"))
    TS_ASSERT(outputWS->run().hasProperty("magneticfield.field"))
    const auto scanVar = run.getLogData("ScanVar");
    TS_ASSERT_EQUALS(scanVar->value(), "omega.position")
    const auto omega = run.getLogData("omega.position");
    TS_ASSERT_EQUALS(omega->size(), 21)
    const double steps = run.getLogAsSingleValue("ScanSteps");
    const auto scanType = run.getLogData("ScanType");
    TS_ASSERT_EQUALS(scanType->value(), "OtherScan")
    TS_ASSERT_DELTA(steps, 21., 1E-10)

    const std::string omegaTimeSeriesValue = "2017-Feb-15 08:58:52  1\n2017-Feb-15 08:58:52.521547000  "
                                             "1.2\n2017-Feb-15 08:58:53.043086000  1.4\n2017-Feb-15 "
                                             "08:58:53.564674000  1.6\n2017-Feb-15 08:58:54.086244000  "
                                             "1.8\n2017-Feb-15 08:58:54.600926000  2\n2017-Feb-15 "
                                             "08:58:55.122357000  2.2\n2017-Feb-15 08:58:55.643809000  "
                                             "2.4\n2017-Feb-15 08:58:56.165310000  2.6\n2017-Feb-15 "
                                             "08:58:56.686815000  2.8\n2017-Feb-15 08:58:57.208370000  "
                                             "3\n2017-Feb-15 08:58:57.730012999  3.2\n2017-Feb-15 "
                                             "08:58:58.251527998  3.4\n2017-Feb-15 08:58:58.773040998  "
                                             "3.6\n2017-Feb-15 08:58:59.294480998  3.8\n2017-Feb-15 "
                                             "08:58:59.815922997  4\n2017-Feb-15 08:59:00.337767997  "
                                             "4.2\n2017-Feb-15 08:59:00.859268997  4.4\n2017-Feb-15 "
                                             "08:59:01.380606996  4.6\n2017-Feb-15 08:59:01.902055996  "
                                             "4.8\n2017-Feb-15 08:59:02.423509996  5\n";

    TS_ASSERT_EQUALS(omega->value(), omegaTimeSeriesValue)
    checkTimeFormat(outputWS);
  }

  void test_D20_detector_scan_offset() {
    // Checks the 2theta0 for a D20 detector scan
    LoadILLDiffraction alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "129080"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("_outWS");
    TS_ASSERT(outputWS)

    const auto detectorInfo = outputWS->detectorInfo();
    const auto indexOfFirstDet = detectorInfo.indexOf(1);
    const V3D position = detectorInfo.position(std::make_pair(indexOfFirstDet, 0));
    double r, theta, phi;
    position.getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(theta, 5.825, 0.001);
    TS_ASSERT_LESS_THAN(position.X(), 0.);
    checkTimeFormat(outputWS);
  }

  void test_D20_multifile() {
    // Tests 2 non-scanned files for D20 with the generic Load on ADS
    // This tests indirectly the confidence method
    // (and NexusDescriptor issue therein)

    Load alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "967100-967101.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("_outWS");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 3073)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    checkTimeFormat(outputWS);
  }

  void test_D2B_alignment() {
    // Tests the D2B loading for a file from Cycle 1 in 04.2018
    // This should have increased pixel size than previously and
    // the corresponding IPF file should contain vertical and horizontal tube
    // alignments.

    LoadILLDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "535401.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("PixelHeight"))
    TS_ASSERT(run.hasProperty("MaxHeight"))
    TS_ASSERT_DELTA(run.getLogAsSingleValue("PixelHeight"), 0.00276, 1E-5)
    TS_ASSERT_DELTA(run.getLogAsSingleValue("MaxHeight"), 0.19386, 1E-5)
    const auto &detInfo = outputWS->detectorInfo();
    const auto tube1CentreTime1 = detInfo.position({70, 0});
    TS_ASSERT_DELTA(tube1CentreTime1.Y(), 0., 0.001)
    double r, theta, phi;
    tube1CentreTime1.getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(theta, 11.25, 0.001)
    const auto tube1CentreTime2 = detInfo.position({70, 1});
    TS_ASSERT_DELTA(tube1CentreTime2.Y(), 0., 0.001)
    tube1CentreTime2.getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(theta, 11.2, 0.001)
    const auto tube23CentreTime1 = detInfo.position({128 * 22 + 69, 0});
    TS_ASSERT_DELTA(tube23CentreTime1.Y(), 0., 0.001)
    tube23CentreTime1.getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(theta, 16.238, 0.001)
    const auto tube23CentreTime2 = detInfo.position({128 * 22 + 69, 1});
    TS_ASSERT_DELTA(tube23CentreTime2.Y(), 0., 0.001)
    tube23CentreTime2.getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(theta, 16.288, 0.001)
    const auto tube128CentreTime1 = detInfo.position({128 * 127 + 68, 0});
    TS_ASSERT_DELTA(tube128CentreTime1.Y(), 0., 0.001)
    tube128CentreTime1.getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(theta, 147.5, 0.001)
    const auto tube128CentreTime2 = detInfo.position({128 * 127 + 68, 1});
    TS_ASSERT_DELTA(tube128CentreTime2.Y(), 0., 0.001)
    tube128CentreTime2.getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(theta, 147.55, 0.001)
    checkTimeFormat(outputWS);
  }

  void do_test_D2B_single_file(const std::string &dataType) {
    // Test a D2B detector scan file with 25 detector positions

    const int NUMBER_OF_TUBES = 128;
    const int NUMBER_OF_PIXELS = 128;
    const int SCAN_COUNT = 25;
    const int NUMBER_OF_MONITORS = 1;

    LoadILLDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "508093.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DataType", dataType))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    const auto &detInfo = outputWS->detectorInfo();
    // Number of time indexes * (number of tubes * number of pixels + monitor)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(),
                     SCAN_COUNT * (NUMBER_OF_TUBES * NUMBER_OF_PIXELS + NUMBER_OF_MONITORS))
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)

    // Check time ranges
    const std::string EXPECTED_START_TIME = "2015-04-16T16:25:31";
    const std::string EXPECTED_SECOND_TIME = "2015-04-16T16:26:08.804000000";
    const std::string EXPECTED_SECOND_FROM_END_TIME = "2015-04-16T16:40:34.289000000";
    const std::string EXPECTED_END_TIME = "2015-04-16T16:41:11.956000000";

    TS_ASSERT_EQUALS(detInfo.scanCount(), SCAN_COUNT)

    const auto startRange = detInfo.scanIntervals()[0];
    const auto secondRange = detInfo.scanIntervals()[1];
    const auto secondFromEndRange = detInfo.scanIntervals()[detInfo.scanCount() - 2];
    const auto endRange = detInfo.scanIntervals()[detInfo.scanCount() - 1];
    TS_ASSERT_EQUALS(startRange.first.toISO8601String(), EXPECTED_START_TIME)
    TS_ASSERT_EQUALS(startRange.second.toISO8601String(), EXPECTED_SECOND_TIME)
    TS_ASSERT_EQUALS(secondRange.first.toISO8601String(), EXPECTED_SECOND_TIME)
    TS_ASSERT_EQUALS(secondFromEndRange.second.toISO8601String(), EXPECTED_SECOND_FROM_END_TIME)
    TS_ASSERT_EQUALS(endRange.first.toISO8601String(), EXPECTED_SECOND_FROM_END_TIME)
    TS_ASSERT_EQUALS(endRange.second.toISO8601String(), EXPECTED_END_TIME)

    // Check monitor does not move
    for (size_t j = 0; j < detInfo.scanCount(); ++j) {
      TS_ASSERT(detInfo.isMonitor({0, j}))
      TS_ASSERT_EQUALS(detInfo.position({0, j}), detInfo.position({0, 0}))
    }

    // Check detector tubes are moved as expected
    const double ANGULAR_DETECTOR_SPACING = 1.25;
    const double ANGULAR_SCAN_INCREMENT = 0.05;
    const double TUBE_128_FIRST_ANGLE = 147.496;

    for (size_t i = 0; i < NUMBER_OF_TUBES; ++i) {
      for (size_t j = 0; j < detInfo.scanCount(); ++j) {
        // Find two pixels just above and just below the centre, and take their
        // average position as the tube centre
        auto belowCentrePixel = i * NUMBER_OF_PIXELS + NUMBER_OF_PIXELS / 2;
        auto aboveCentrePixel = belowCentrePixel + 1;
        TS_ASSERT(!detInfo.isMonitor({belowCentrePixel, j}))
        TS_ASSERT(!detInfo.isMonitor({aboveCentrePixel, j}))
        auto tubeCentre = (detInfo.position({belowCentrePixel, j}) + detInfo.position({aboveCentrePixel, j})) / 2;
        // Check the tube centre is 90 degrees from the y-axis
        TS_ASSERT_DELTA(tubeCentre.angle(V3D(0, 1, 0)) * RAD_2_DEG, 90.0, 1e-6)
        // Check the tube centre is at the expected angle from the z-axis
        // (incoming beam). The last angle for tube_128 is hard coded here, then
        // for each time index the angle increments by ANGULAR_SCAN_INCREMENT
        // (0.05 deg). Then detectors themselves are spaced apart by
        // ANGULARD_DETECTOR_SPACING (1.25 deg).
        //
        // A generous tolerance is required as the NeXus file contains the
        // actual hardware readings, which have a large tolerance.
        TS_ASSERT_DELTA(tubeCentre.angle(V3D(0, 0, 1)) * RAD_2_DEG,
                        std::abs(ANGULAR_SCAN_INCREMENT * double(j) + TUBE_128_FIRST_ANGLE -
                                 ANGULAR_DETECTOR_SPACING * (NUMBER_OF_TUBES - 1) +
                                 ANGULAR_DETECTOR_SPACING * double(i)),
                        1e-2)
      }
      checkTimeFormat(outputWS);
    }

    TS_ASSERT(outputWS->run().hasProperty("Multi.TotalCount"))

    if (dataType == "Raw") {
      TS_ASSERT_DELTA(outputWS->y(25)[0], 0, 1e-12)
      TS_ASSERT_EQUALS(outputWS->run().getProperty("Detector.calibration_file")->value(), "none")
    } else {
      TS_ASSERT_DELTA(outputWS->y(25)[0], 1, 1e-12)
      TS_ASSERT_EQUALS(outputWS->run().getProperty("Detector.calibration_file")->value(), "d2bcal_23Nov16_c.2d")
    }
  }

  void test_D2B_single_file() { do_test_D2B_single_file("Auto"); }

  void test_D2B_single_file_calibrated() { do_test_D2B_single_file("Calibrated"); }

  void test_D2B_single_file_raw() { do_test_D2B_single_file("Raw"); }

  void test_D2B_single_point_scan() {
    LoadILLDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "543614.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    const auto run = outputWS->run();
    TS_ASSERT(run.hasProperty("ScanType"));
    const auto type = run.getLogData("ScanType");
    TS_ASSERT_EQUALS(type->value(), "DetectorScan");
    TS_ASSERT(run.hasProperty("ScanVar"));
    const auto scanVar = run.getLogData("ScanVar");
    TS_ASSERT_EQUALS(scanVar->value(), "2theta.position");
    checkTimeFormat(outputWS);
  }

  void checkTimeFormat(MatrixWorkspace_const_sptr outputWS) {
    TS_ASSERT(outputWS->run().hasProperty("start_time"));
    TS_ASSERT(
        Mantid::Types::Core::DateAndTimeHelpers::stringIsISO8601(outputWS->run().getProperty("start_time")->value()));
  }

  void test_D1B() {
    const int NUMBER_OF_TUBES = 1280;
    const int NUMBER_OF_MONITORS = 1;

    LoadILLDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "473432.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("TwoThetaOffset", "0.0"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    const auto run = outputWS->run();

    const auto &detInfo = outputWS->detectorInfo();
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), NUMBER_OF_TUBES + NUMBER_OF_MONITORS)

    TS_ASSERT(!detInfo.isMonitor({1, 0}))
    auto firstTube = detInfo.position({1, 0});
    TS_ASSERT_DELTA(firstTube.angle(V3D(0, 0, 1)) * RAD_2_DEG, 0.85, 1e-6)

    TS_ASSERT_EQUALS(outputWS->y(13)[0], 1394)
    checkTimeFormat(outputWS);
  }

  void test_IN5_omega_scan() {
    // Tests the omega-scan case for IN5

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "199857.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DataType", "Raw"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 98305)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 17)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(98304))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())

    TS_ASSERT_DELTA(outputWS->x(0)[0], 276.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(0)[0], 0.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(0)[0], 0.00, 0.01)

    TS_ASSERT_DELTA(outputWS->x(65)[15], 279.75, 0.01)
    TS_ASSERT_DELTA(outputWS->y(65)[15], 1.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(65)[15], 1.00, 0.01)

    TS_ASSERT_DELTA(outputWS->x(98304)[0], 276.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(98304)[0], 2471.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(98304)[0], 49.71, 0.01)

    TS_ASSERT_DELTA(outputWS->x(98304)[16], 280.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(98304)[16], 513.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(98304)[16], 22.65, 0.01)

    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("AcquisitionSpy.Time"))
    TS_ASSERT(run.hasProperty("SampleSettings.SampleTemp"))
    TS_ASSERT(run.hasProperty("ScanType"))
    TS_ASSERT(run.hasProperty("ScanVar"))
    TS_ASSERT(run.hasProperty("ResolutionMode"))
    TS_ASSERT(run.hasProperty("Ei"))
    TS_ASSERT(run.hasProperty("run_list"))

    const auto spy = run.getLogData("AcquisitionSpy.Time");
    const auto sample = run.getLogData("SampleSettings.SampleTemp");
    const auto scanType = run.getLogData("ScanType");
    const auto scanVar = run.getLogData("ScanVar");
    const auto ei = run.getLogAsSingleValue("wavelength");
    const auto runList = run.getLogData("run_list");

    TS_ASSERT_EQUALS(scanType->value(), "OtherScan")
    TS_ASSERT_EQUALS(scanVar->value(), "samplerotation.position")
    TS_ASSERT_EQUALS(spy->size(), 17)
    TS_ASSERT_EQUALS(sample->size(), 17)
    TS_ASSERT_DELTA(ei, 4.80, 0.01)
    TS_ASSERT_EQUALS(outputWS->run().getProperty("Detector.calibration_file")->value(), "none")
    TS_ASSERT_EQUALS(runList->value(), "199857")
    checkTimeFormat(outputWS);
  }

  void test_PANTHER_omega_scan() {
    // Tests the omega-scan case for PANTHER

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "010578.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DataType", "Raw"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 73729)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 16)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(73728))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())

    TS_ASSERT_DELTA(outputWS->x(0)[0], 0.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(0)[0], 0.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(0)[0], 0.00, 0.01)

    TS_ASSERT_DELTA(outputWS->x(65)[15], 30.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(65)[15], 3.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(65)[15], 1.73, 0.01)

    TS_ASSERT_DELTA(outputWS->x(73728)[0], 0.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(73728)[0], 497.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(73728)[0], 22.29, 0.01)

    TS_ASSERT_DELTA(outputWS->x(73728)[15], 30.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(73728)[15], 504.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(73728)[15], 22.45, 0.01)

    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("AcquisitionSpy.Time"))
    TS_ASSERT(run.hasProperty("SampleSettings.SampleTemp"))
    TS_ASSERT(run.hasProperty("ScanType"))
    TS_ASSERT(run.hasProperty("ScanVar"))
    TS_ASSERT(run.hasProperty("ResolutionMode"))
    TS_ASSERT(run.hasProperty("Ei"))
    TS_ASSERT(run.hasProperty("run_list"))

    const auto spy = run.getLogData("AcquisitionSpy.Time");
    const auto sample = run.getLogData("SampleSettings.SampleTemp");
    const auto scanType = run.getLogData("ScanType");
    const auto scanVar = run.getLogData("ScanVar");
    const auto ei = run.getLogAsSingleValue("wavelength");
    const auto runList = run.getLogData("run_list");

    TS_ASSERT_EQUALS(scanType->value(), "OtherScan")
    TS_ASSERT_EQUALS(scanVar->value(), "d1t.position")
    TS_ASSERT_EQUALS(spy->size(), 16)
    TS_ASSERT_EQUALS(sample->size(), 16)

    TS_ASSERT_DELTA(ei, 1.5288, 0.0001)
    TS_ASSERT_EQUALS(outputWS->run().getProperty("Detector.calibration_file")->value(), "none")
    TS_ASSERT_EQUALS(runList->value(), "10578")
    checkTimeFormat(outputWS);
  }

  void test_SHARP_omega_scan() {
    // Tests the omega-scan case for SHARP

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "000104"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DataType", "Raw"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 61441)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 8)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(61440))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())

    TS_ASSERT_DELTA(outputWS->x(0)[0], 60.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(0)[0], 163.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(0)[0], 12.77, 0.01)

    TS_ASSERT_DELTA(outputWS->x(65)[7], 62.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(65)[7], 222.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(65)[7], 14.90, 0.01)

    TS_ASSERT_DELTA(outputWS->x(61440)[0], 60.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(61440)[0], 128.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(61440)[0], 11.31, 0.01)

    TS_ASSERT_DELTA(outputWS->x(61440)[7], 62.00, 0.01)
    TS_ASSERT_DELTA(outputWS->y(61440)[7], 128.00, 0.01)
    TS_ASSERT_DELTA(outputWS->e(61440)[7], 11.31, 0.01)

    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("AcquisitionSpy.Time"))
    TS_ASSERT(run.hasProperty("SampleSettings.SampleTemp"))
    TS_ASSERT(run.hasProperty("ScanType"))
    TS_ASSERT(run.hasProperty("ScanVar"))
    TS_ASSERT(run.hasProperty("ResolutionMode"))
    TS_ASSERT(run.hasProperty("Ei"))
    TS_ASSERT(run.hasProperty("run_list"))

    const auto spy = run.getLogData("AcquisitionSpy.Time");
    const auto sample = run.getLogData("SampleSettings.SampleTemp");
    const auto scanType = run.getLogData("ScanType");
    const auto scanVar = run.getLogData("ScanVar");
    const auto ei = run.getLogAsSingleValue("wavelength");
    const auto runList = run.getLogData("run_list");

    TS_ASSERT_EQUALS(scanType->value(), "OtherScan")
    TS_ASSERT_EQUALS(scanVar->value(), "updown.position")
    TS_ASSERT_EQUALS(spy->size(), 8)
    TS_ASSERT_EQUALS(sample->size(), 8)

    TS_ASSERT_DELTA(ei, 5.12, 0.01)
    TS_ASSERT_EQUALS(outputWS->run().getProperty("Detector.calibration_file")->value(), "none")
    TS_ASSERT_EQUALS(runList->value(), "104")
    checkTimeFormat(outputWS);
  }

private:
  const double RAD_2_DEG = 180.0 / M_PI;
  std::string m_oldFacility;
  std::string m_oldInstrument;
};

class LoadILLDiffractionTestPerformance : public CxxTest::TestSuite {
public:
  static LoadILLDiffractionTestPerformance *createSuite() { return new LoadILLDiffractionTestPerformance(); }
  static void destroySuite(LoadILLDiffractionTestPerformance *suite) { delete suite; }

  LoadILLDiffractionTestPerformance() {}

  void setUp() override {
    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setPropertyValue("Filename", "ILL/D2B/508093.nxs");
    m_alg.setPropertyValue("OutputWorkspace", "__");
  }

  void test_performance() {
    for (int i = 0; i < 5; ++i) {
      TS_ASSERT_THROWS_NOTHING(m_alg.execute());
    }
  }

private:
  LoadILLDiffraction m_alg;
};
