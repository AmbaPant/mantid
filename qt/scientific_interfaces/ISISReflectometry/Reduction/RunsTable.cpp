// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsTable.h"
#include "RowLocation.h"

namespace MantidQt {
namespace CustomInterfaces {

using MantidWidgets::Batch::RowLocation;

RunsTable::RunsTable( // cppcheck-suppress passedByValue
    std::vector<std::string> instruments, double thetaTolerance,
    // cppcheck-suppress passedByValue
    ReductionJobs reductionJobs)
    : m_instruments(std::move(instruments)), m_thetaTolerance(thetaTolerance),
      m_reductionJobs(std::move(reductionJobs)), m_selectedRowLocations() {}

double RunsTable::thetaTolerance() const { return m_thetaTolerance; }

ReductionJobs const &RunsTable::reductionJobs() const {
  return m_reductionJobs;
}

ReductionJobs &RunsTable::mutableReductionJobs() { return m_reductionJobs; }

std::vector<RowLocation> const &RunsTable::selectedRowLocations() const {
  return m_selectedRowLocations;
}

bool RunsTable::hasSelection() const {
  return m_selectedRowLocations.size() > 0;
}

void RunsTable::setSelectedRowLocations(std::vector<RowLocation> selected) {
  m_selectedRowLocations = std::move(selected);
}

void RunsTable::resetState() { m_reductionJobs.resetState(); }
} // namespace CustomInterfaces
} // namespace MantidQt
