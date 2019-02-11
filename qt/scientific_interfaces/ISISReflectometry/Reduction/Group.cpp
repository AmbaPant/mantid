// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Group.h"
#include "Common/IndexOf.h"
#include "Common/Map.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <cmath>

namespace MantidQt {
namespace CustomInterfaces {

Group::Group( // cppcheck-suppress passedByValue
    std::string name, std::vector<boost::optional<Row>> rows)
    : m_name(std::move(name)), m_postprocessedWorkspaceName(),
      m_rows(std::move(rows)) {}

Group::Group(
    // cppcheck-suppress passedByValue
    std::string name)
    : m_name(std::move(name)), m_rows() {}

std::string const &Group::name() const { return m_name; }

bool Group::requiresPostprocessing() const { return m_rows.size() > 1; }

std::string Group::postprocessedWorkspaceName() const {
  return m_postprocessedWorkspaceName;
}

boost::optional<int> Group::indexOfRowWithTheta(double theta,
                                                double tolerance) const {
  return indexOf(m_rows,
                 [theta, tolerance](boost::optional<Row> const &row) -> bool {
                   return row.is_initialized() &&
                          std::abs(row.get().theta() - theta) < tolerance;
                 });
}

void Group::setName(std::string const &name) { m_name = name; }

void Group::resetState() {
  for (auto &row : m_rows)
    row->resetState();
}

bool Group::allRowsAreValid() const {
  return std::all_of(m_rows.cbegin(), m_rows.cend(),
                     [](boost::optional<Row> const &row) -> bool {
                       return row.is_initialized();
                     });
}

std::vector<boost::optional<Row>> const &Group::rows() const { return m_rows; }

std::vector<boost::optional<Row>> &Group::mutableRows() { return m_rows; }

void Group::appendRow(boost::optional<Row> const &row) {
  m_rows.emplace_back(row);
}

void Group::appendEmptyRow() { m_rows.emplace_back(boost::none); }

void Group::insertRow(boost::optional<Row> const &row, int beforeRowAtIndex) {
  m_rows.insert(m_rows.begin() + beforeRowAtIndex, row);
}

void Group::removeRow(int rowIndex) { m_rows.erase(m_rows.begin() + rowIndex); }

void Group::updateRow(int rowIndex, boost::optional<Row> const &row) {
  m_rows[rowIndex] = row;
}

boost::optional<Row> const &Group::operator[](int rowIndex) const {
  return m_rows[rowIndex];
}
} // namespace CustomInterfaces
} // namespace MantidQt
