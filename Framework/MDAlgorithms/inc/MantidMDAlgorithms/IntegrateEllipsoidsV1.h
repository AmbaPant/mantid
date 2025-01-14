// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMDAlgorithms/Integrate3DEvents.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

namespace Mantid {
namespace Geometry {
class DetectorInfo;
}
namespace MDAlgorithms {

class DLLExport IntegrateEllipsoidsV1 final : public API::Algorithm {
public:
  static void initInstance(API::Algorithm &alg);
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Integrate Single Crystal Diffraction Bragg peaks using 3D "
           "ellipsoids.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"IntegrateEllipsoidsTwoStep"}; }
  const std::string category() const override;

private:
  using PrincipleAxes = std::array<std::vector<double>, 3>;

  void init() override;
  void exec() override;
  void qListFromEventWS(Integrate3DEvents &integrator, API::Progress &prog, DataObjects::EventWorkspace_sptr &wksp,
                        Kernel::DblMatrix const &UBinv, bool hkl_integ);
  void qListFromHistoWS(Integrate3DEvents &integrator, API::Progress &prog, DataObjects::Workspace2D_sptr &wksp,
                        Kernel::DblMatrix const &UBinv, bool hkl_integ);

  /// Calculate if this Q is on a detector
  void calculateE1(const Geometry::DetectorInfo &detectorInfo);

  void runMaskDetectors(const Mantid::DataObjects::PeaksWorkspace_sptr &peakWS, const std::string &property,
                        const std::string &values);

  /// save for all detector pixels
  std::vector<Kernel::V3D> E1Vec;

  MDWSDescription m_targWSDescr;

  void initTargetWSDescr(API::MatrixWorkspace_sptr &wksp);
};

} // namespace MDAlgorithms
} // namespace Mantid
