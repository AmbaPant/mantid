// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** CreateChunkingFromInstrument : TODO: DESCRIPTION
 */
class DLLExport CreateChunkingFromInstrument final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  Geometry::Instrument_const_sptr getInstrument();
};

} // namespace DataHandling
} // namespace Mantid
