// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/FileDescriptor.h"

#include <deque>

namespace Mantid {
namespace MDAlgorithms {

/** ConvertSpiceDataToRealSpace : Convert data from SPICE file to singals
  in real space contained in MDEventWrokspaces
*/
class DLLExport ConvertSpiceDataToRealSpace final : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertSpiceDataToRealSpace"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Load a HFIR powder diffractometer SPICE file."; }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\ConstantWavelength;DataHandling\\Text"; }

  /// Returns a confidence value that this algorithm can load a file
  // virtual int confidence(Kernel::FileDescriptor &descriptor) const;

private:
  /// Typdef for the white-space separated file data type.
  using DataCollectionType = std::deque<std::string>;

  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  /// Load data by call
  DataObjects::TableWorkspace_sptr loadSpiceData(const std::string &spicefilename);

  /// Parse data table workspace to a vector of matrix workspaces
  std::vector<API::MatrixWorkspace_sptr> convertToMatrixWorkspace(const DataObjects::TableWorkspace_sptr &tablews,
                                                                  const API::MatrixWorkspace_const_sptr &parentws,
                                                                  Types::Core::DateAndTime runstart,
                                                                  std::map<std::string, std::vector<double>> &logvecmap,
                                                                  std::vector<Types::Core::DateAndTime> &vectimes);

  /// Create an MDEventWorspace by converting vector of matrix workspace data
  API::IMDEventWorkspace_sptr createDataMDWorkspace(const std::vector<API::MatrixWorkspace_sptr> &vec_ws2d);

  /// Create an MDWorkspace for monitor counts
  API::IMDEventWorkspace_sptr createMonitorMDWorkspace(const std::vector<API::MatrixWorkspace_sptr> &vec_ws2d,
                                                       const std::vector<double> &vecmonitor);

  /// Read parameter information from table workspace
  void readTableInfo(const DataObjects::TableWorkspace_const_sptr &tablews, size_t &ipt, size_t &irotangle,
                     size_t &itime, std::vector<std::pair<size_t, size_t>> &anodelist,
                     std::map<std::string, size_t> &samplenameindexmap);

  /// Return sample logs
  void parseSampleLogs(const DataObjects::TableWorkspace_sptr &tablews, const std::map<std::string, size_t> &indexlist,
                       std::map<std::string, std::vector<double>> &logvecmap);

  /// Load one run (one pt.) to a matrix workspace
  API::MatrixWorkspace_sptr loadRunToMatrixWS(const DataObjects::TableWorkspace_sptr &tablews, size_t irow,
                                              const API::MatrixWorkspace_const_sptr &parentws,
                                              Types::Core::DateAndTime runstart, size_t ipt, size_t irotangle,
                                              size_t itime, const std::vector<std::pair<size_t, size_t>> &anodelist,
                                              double &duration);

  /// Append Experiment Info
  void addExperimentInfos(const API::IMDEventWorkspace_sptr &mdws,
                          const std::vector<API::MatrixWorkspace_sptr> &vec_ws2d);

  /// Append sample logs to MD workspace
  void appendSampleLogs(const API::IMDEventWorkspace_sptr &mdws,
                        const std::map<std::string, std::vector<double>> &logvecmap,
                        const std::vector<Types::Core::DateAndTime> &vectimes);

  /// Parse detector efficiency table workspace to map
  std::map<detid_t, double> parseDetectorEfficiencyTable(const DataObjects::TableWorkspace_sptr &detefftablews);

  /// Apply the detector's efficiency correction to
  void correctByDetectorEfficiency(std::vector<API::MatrixWorkspace_sptr> vec_ws2d,
                                   const std::map<detid_t, double> &detEffMap);

  /// Name of instrument
  std::string m_instrumentName;

  /// Number of detectors
  size_t m_numSpec = 0;

  /// x-y-z-value minimum
  std::vector<double> m_extentMins;
  /// x-y-z value maximum
  std::vector<double> m_extentMaxs;
  /// Number of bins
  std::vector<size_t> m_numBins;
  /// Dimension of the output MDEventWorkspace
  size_t m_nDimensions = 3;
};

} // namespace MDAlgorithms
} // namespace Mantid
