//----------------
// Includes
//----------------

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include <string>
#include <H5Cpp.h>
#include <chrono>

using namespace Mantid;
using namespace NexusGeometry;
namespace {
std::unique_ptr<Geometry::DetectorInfo>
extractDetectorInfo(const Mantid::Geometry::Instrument &instrument) {
  Geometry::ParameterMap pmap;
  return std::move(std::get<1>(instrument.makeBeamline(pmap)));
}

std::pair<std::unique_ptr<Geometry::ComponentInfo>,
          std::unique_ptr<Geometry::DetectorInfo>>
extractBeamline(const Mantid::Geometry::Instrument &instrument) {
  Geometry::ParameterMap pmap;
  auto beamline = instrument.makeBeamline(pmap);
  return {std::move(std::get<0>(beamline)), std::move(std::get<1>(beamline))};
}
}
class NexusGeometryParserTest : public CxxTest::TestSuite {
public:
  std::unique_ptr<const Mantid::Geometry::Instrument> makeTestInstrument() {
    H5std_string nexusFilename = "SMALLFAKE_example_geometry.hdf5";
    const auto fullpath = Kernel::ConfigService::Instance().getFullPath(
        nexusFilename, true, Poco::Glob::GLOB_DEFAULT);

    return NexusGeometryParser::createInstrument(fullpath);
  }

  void test_basic_instrument_information() {
    auto instrument = makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);
    auto detectorInfo = std::move(beamline.second);
    TSM_ASSERT_EQUALS("Detectors + 1 monitor", detectorInfo->size(),
                      128 * 2 + 1);
    TSM_ASSERT_EQUALS("Detectors + 3 non-detector components",
                      componentInfo->size(), detectorInfo->size() + 3);
  }

  void test_source_is_where_expected() {
    auto instrument = makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);

    auto sourcePosition =
        Kernel::toVector3d(componentInfo->position(componentInfo->source()));

    TS_ASSERT(sourcePosition.isApprox(Eigen::Vector3d(
        0, 0, -34.281))); // Check against fixed position in file
  }

  void test_simple_translation() {
    auto instrument = makeTestInstrument();
    auto detectorInfo = extractDetectorInfo(*instrument);
    // First pixel in bank 2
    auto det0Position = Kernel::toVector3d(
        detectorInfo->position(detectorInfo->indexOf(1100000)));
    Eigen::Vector3d unitVector(0, 0,
                               1); // Fixed in file location vector attribute
    const double magnitude = 4.0;  // Fixed in file location value
    Eigen::Vector3d offset(-0.498, -0.200, 0.00); // All offsets for pixel x,
                                                  // and y specified separately,
                                                  // z defaults to 0
    auto expectedDet0Position = offset + (magnitude * unitVector);
    TS_ASSERT(det0Position.isApprox(expectedDet0Position));
  }

  void test_complex_translation() {
    auto instrument = makeTestInstrument();
    auto detectorInfo = extractDetectorInfo(*instrument);
    // First pixel in bank 1

    auto det0Postion = Kernel::toVector3d(
        detectorInfo->position(detectorInfo->indexOf(2100000)));
    Eigen::Vector3d unitVectorTranslation(
        0.2651564830210424, 0.0,
        0.9642053928037905);         // Fixed in file location vector attribute
    const double magnitude = 4.148;  // Fixed in file location value
    const double rotation = -15.376; // Fixed in file orientation value
    Eigen::Vector3d rotationAxis(
        0, -1, 0); // Fixed in file orientation vector attribute
    Eigen::Vector3d offset(-0.498, -0.200, 0.00); // All offsets for pixel x,
                                                  // and y specified separately,
                                                  // z defaults to 0
    auto affine = Eigen::Transform<double, 3, Eigen::Affine>::Identity();
    // Rotation of bank
    affine = Eigen::Quaterniond(
                 Eigen::AngleAxisd(rotation * M_PI / 180, rotationAxis)) *
             affine;
    // Translation of bank
    affine = Eigen::Translation3d(magnitude * unitVectorTranslation) * affine;
    auto expectedPosition = affine * offset;
    TS_ASSERT(det0Postion.isApprox(expectedPosition, 1e-3));
  }

  void test_shape_cylinder_shape() {

    auto instrument = makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);
    const auto &det1Shape = componentInfo->shape(1);
    const auto &det2Shape = componentInfo->shape(2);
    TSM_ASSERT_EQUALS("Pixel shapes should be the same within bank", &det1Shape,
                      &det2Shape);

    const auto *csgShape1 =
        dynamic_cast<const Mantid::Geometry::CSGObject *>(&det1Shape);
    TSM_ASSERT("Expected pixel shape as CSGObject", csgShape1 != nullptr);
    const auto *csgShape2 =
        dynamic_cast<const Mantid::Geometry::CSGObject *>(&det2Shape);
    TSM_ASSERT("Expected monitors shape as CSGObject", csgShape2 != nullptr);
    auto shapeBB = det1Shape.getBoundingBox();
    TS_ASSERT_DELTA(shapeBB.xMax() - shapeBB.xMin(), 0.03125 - (-0.03125),
                    1e-9); // Cylinder length fixed in file
    TS_ASSERT_DELTA(shapeBB.yMax() - shapeBB.yMin(), 2 * 0.00405,
                    1e-9); // Cylinder radius fixed in file
  }

  void test_mesh_shape() {

    auto instrument = makeTestInstrument();
    auto beamline = extractBeamline(*instrument);
    auto componentInfo = std::move(beamline.first);
    auto detectorInfo = std::move(beamline.second);
    const size_t monitorIndex = 0; // Fixed in file
    TS_ASSERT(detectorInfo->isMonitor(monitorIndex));
    TSM_ASSERT("Monitor shape", componentInfo->hasValidShape(monitorIndex));
    const auto &monitorShape = componentInfo->shape(monitorIndex);
    const auto *meshShape =
        dynamic_cast<const Mantid::Geometry::MeshObject *>(&monitorShape);
    TSM_ASSERT("Expected monitors shape as mesh", meshShape != nullptr);

    TS_ASSERT_EQUALS(meshShape->numberOfTriangles(),
                     6 * 2); // Each face of cube split into 2 triangles
    TS_ASSERT_EQUALS(meshShape->numberOfVertices(),
                     8); // 8 unique vertices in cube
    auto shapeBB = monitorShape.getBoundingBox();
    TS_ASSERT_DELTA(shapeBB.xMax() - shapeBB.xMin(), 2.0, 1e-9);
    TS_ASSERT_DELTA(shapeBB.yMax() - shapeBB.yMin(), 2.0, 1e-9);
    TS_ASSERT_DELTA(shapeBB.zMax() - shapeBB.zMin(), 2.0, 1e-9);
  }
};

class NexusGeometryParserTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometryParserTestPerformance *createSuite() {
    return new NexusGeometryParserTestPerformance();
  }

  NexusGeometryParserTestPerformance() {
    m_wishHDF5DefinitionPath = Kernel::ConfigService::Instance().getFullPath(
        "WISH_Definition_10Panels.hdf5", true, Poco::Glob::GLOB_DEFAULT);
  }
  static void destroySuite(NexusGeometryParserTestPerformance *suite) {
    delete suite;
  }

  void test_load_wish() {
    auto start = std::chrono::high_resolution_clock::now();
    auto wishInstrument =
        NexusGeometryParser::createInstrument(m_wishHDF5DefinitionPath);
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "Creating WISH instrument took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     stop - start).count() << " ms" << std::endl;
    auto detInfo = extractDetectorInfo(*wishInstrument);
    TS_ASSERT_EQUALS(detInfo->size(), 778245); // Sanity check
  }

private:
  std::string m_wishHDF5DefinitionPath;
};
