// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** SavePDFGui : Saves a workspace containing a pair distrebution function in a
format readable by the PDFgui package.

Required Properties:
<UL>
<LI> InputWorkspace - An input workspace with units of Atomic Distance </LI>
<LI> Filename - The filename to use for the saved data </LI>
</UL>
*/
class DLLExport SavePDFGui final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"SaveAscii"}; }
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  void writeMetaData(std::ofstream &out, const API::MatrixWorkspace_const_sptr &inputWS);
  void writeWSData(std::ofstream &out, const API::MatrixWorkspace_const_sptr &inputWS);
};

} // namespace DataHandling
} // namespace Mantid
