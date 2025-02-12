// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/**
    Writer for compatibility with SansView and NIST reduced data file format.
    Writes I(Qx,Qy), the output of Qxy, to an ASCII file.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The reduced data in units of Q. </LI>
    <LI> Filename          - Output file. </LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SaveNISTDAT final : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SaveNISTDAT"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save I(Qx,Qy) data to a text file compatible with NIST and DANSE "
           "readers.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS;DataHandling\\Text"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
