// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "Common/DllConfig.h"
#include "LookupRow.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class Row;

class MANTIDQT_ISISREFLECTOMETRY_DLL LookupTable {
public:
  LookupTable() = default;
  LookupTable(std::vector<LookupRow> rowsIn);
  LookupTable(std::initializer_list<LookupRow> rowsIn);

  std::vector<LookupRow> const &rows() const;
  boost::optional<LookupRow> findLookupRow(Row const &row, double tolerance) const;
  boost::optional<LookupRow> findWildcardLookupRow() const;
  std::vector<LookupRow::ValueArray> toValueArray() const;

  bool operator==(LookupTable const &rhs) const noexcept;
  bool operator!=(LookupTable const &rhs) const noexcept;

private:
  std::vector<LookupRow> m_lookupRows;

  boost::optional<LookupRow> searchByTheta(std::vector<LookupRow> lookupRows, boost::optional<double> const &,
                                           double) const;
  std::vector<LookupRow> searchByTitle(Row const &row) const;
  std::vector<LookupRow> findMatchingRegexes(std::string const &title) const;
  std::vector<LookupRow> findEmptyRegexes() const;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
