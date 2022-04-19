// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "Experiment.h"
#include "IBatch.h"
#include "Instrument.h"
#include "Reduction/LookupRow.h"
#include "RunsTable.h"
#include "Slicing.h"

#include <boost/optional.hpp>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class Batch

    The Batch model holds the entire reduction configuration for a batch of
    runs.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL Batch final : public IBatch {
public:
  Batch(Experiment const &experiment, Instrument const &instrument, RunsTable &runsTable, Slicing const &slicing);

  Experiment const &experiment() const override;
  Instrument const &instrument() const override;
  RunsTable const &runsTable() const;
  RunsTable &mutableRunsTable();
  Slicing const &slicing() const override;

  std::vector<MantidWidgets::Batch::RowLocation> selectedRowLocations() const;
  template <typename T>
  bool isInSelection(T const &item, std::vector<MantidWidgets::Batch::RowLocation> const &selectedRowLocations) const;
  boost::optional<LookupRow> findLookupRow(Row const &row) const override;
  boost::optional<LookupRow> findWildcardLookupRow() const override;
  void resetState();
  void resetSkippedItems();
  boost::optional<Item &> getItemWithOutputWorkspaceOrNone(std::string const &wsName);

  void updateLookupIndex(Row &row);
  void updateLookupIndexesOfGroup(Group &group);
  void updateLookupIndexesOfTable();

private:
  Experiment const &m_experiment;
  Instrument const &m_instrument;
  RunsTable &m_runsTable;
  Slicing const &m_slicing;
};

template <typename T>
bool Batch::isInSelection(T const &item,
                          std::vector<MantidWidgets::Batch::RowLocation> const &selectedRowLocations) const {
  return m_runsTable.isInSelection(item, selectedRowLocations);
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
