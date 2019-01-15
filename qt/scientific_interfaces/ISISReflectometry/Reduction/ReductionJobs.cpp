// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReductionJobs.h"
#include "Common/IndexOf.h"
#include "Common/Map.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <iostream>
namespace MantidQt {
namespace CustomInterfaces {

namespace {
Group &findOrMakeGroupWithName(ReductionJobs &jobs,
                               std::string const &groupName) {
  auto maybeGroupIndex = jobs.indexOfGroupWithName(groupName);
  if (maybeGroupIndex.is_initialized())
    return jobs.mutableGroups()[maybeGroupIndex.get()];
  else
    return jobs.appendGroup(Group(groupName));
} // unnamed
} // namespace

ReductionJobs::ReductionJobs( // cppcheck-suppress passedByValue
    std::vector<Group> groups)
    : m_groups(std::move(groups)), m_groupNameSuffix(1) {}

ReductionJobs::ReductionJobs() : m_groupNameSuffix(1) {}

Group &ReductionJobs::appendGroup(Group group) {
  assertOrThrow(group.name().empty() || !hasGroupWithName(group.name()),
                "Cannot have multiple groups with a matching non-empty name.");
  m_groups.emplace_back(std::move(group));
  return m_groups.back();
}

boost::optional<int>
ReductionJobs::indexOfGroupWithName(std::string const &groupName) {
  return indexOf(m_groups, [&groupName](Group const &group) -> bool {
    return group.name() == groupName;
  });
}

Group &ReductionJobs::insertGroup(Group group, int beforeIndex) {
  assertOrThrow(group.name().empty() || !hasGroupWithName(group.name()),
                "Cannot have multiple groups with a matching non-empty name.");
  return *m_groups.insert(m_groups.begin() + beforeIndex, std::move(group));
}

bool ReductionJobs::hasGroupWithName(std::string const &groupName) const {
  return std::any_of(m_groups.crbegin(), m_groups.crend(),
                     [&groupName](Group const &group) -> bool {
                       return group.name() == groupName;
                     });
}

void ReductionJobs::removeGroup(int index) {
  m_groups.erase(m_groups.begin() + index);
  ensureAtLeastOneGroupExists(*this);
}

void ReductionJobs::removeAllGroups() {
  m_groups.clear();
  ensureAtLeastOneGroupExists(*this);
}

std::vector<Group> &ReductionJobs::mutableGroups() { return m_groups; }

std::vector<Group> const &ReductionJobs::groups() const { return m_groups; }

std::string ReductionJobs::nextEmptyGroupName() {
  std::string name = "Group" + std::to_string(m_groupNameSuffix);
  m_groupNameSuffix++;
  return name;
}

/* This function is called after deleting groups to ensure that the model
 * always contains at least one group - it adds an empty group if
 * required. This is to mimic behaviour of the JobTreeView, which cannot delete
 * the last group/row so it always leaves at least one empty group.
 */
void ensureAtLeastOneGroupExists(ReductionJobs &jobs) {
  if (jobs.groups().size() == 0)
    appendEmptyGroup(jobs);
}

void removeGroup(ReductionJobs &jobs, int groupIndex) {
  jobs.removeGroup(groupIndex);
}

void removeAllRowsAndGroups(ReductionJobs &jobs) { jobs.removeAllGroups(); }

void appendEmptyRow(ReductionJobs &jobs, int groupIndex) {
  jobs.mutableGroups()[groupIndex].appendEmptyRow();
}

void appendEmptyGroup(ReductionJobs &jobs) {
  jobs.appendGroup(Group(jobs.nextEmptyGroupName()));
}

void insertEmptyGroup(ReductionJobs &jobs, int beforeGroup) {
  jobs.insertGroup(Group(jobs.nextEmptyGroupName()), beforeGroup);
}

void insertEmptyRow(ReductionJobs &jobs, int groupIndex, int beforeRow) {
  jobs.mutableGroups()[groupIndex].insertRow(boost::none, beforeRow);
}

void updateRow(ReductionJobs &jobs, int groupIndex, int rowIndex,
               boost::optional<Row> const &newValue) {
  if (newValue.is_initialized()) {
    jobs.mutableGroups()[groupIndex].updateRow(rowIndex, newValue);
  } else {
    jobs.mutableGroups()[groupIndex].updateRow(rowIndex, boost::none);
  }
}

void mergeRowIntoGroup(ReductionJobs &jobs, Row const &row,
                       double thetaTolerance, std::string const &groupName) {
  auto &group = findOrMakeGroupWithName(jobs, groupName);
  auto indexOfRowToUpdate =
      group.indexOfRowWithTheta(row.theta(), thetaTolerance);

  if (indexOfRowToUpdate.is_initialized()) {
    auto newRowValue = mergedRow(group[indexOfRowToUpdate.get()].get(), row);
    group.updateRow(indexOfRowToUpdate.get(), newRowValue);
  } else {
    group.appendRow(row);
  }
}

void removeRow(ReductionJobs &jobs, int groupIndex, int rowIndex) {
  jobs.mutableGroups()[groupIndex].removeRow(rowIndex);
}

bool setGroupName(ReductionJobs &jobs, int groupIndex,
                  std::string const &newValue) {
  auto &group = jobs.mutableGroups()[groupIndex];
  if (group.name() != newValue) {
    if (newValue.empty() || !jobs.hasGroupWithName(newValue)) {
      group.setName(newValue);
    } else {
      return false;
    }
  }
  return true;
}

std::string groupName(ReductionJobs const &jobs, int groupIndex) {
  return jobs[groupIndex].name();
}

void prettyPrintModel(ReductionJobs const & /*jobs*/) {
  std::cout << "ReductionJobs:\n";
  //  for (auto &&group : jobs.groups()) {
  //    std::cout << group;
  //  }
  std::cout << std::endl;
}

Group const &ReductionJobs::operator[](int index) const {
  return m_groups[index];
}

MantidWidgets::Batch::RowPath ReductionJobs::getPath(Group const &group) const {
  // Find this group in the groups list
  auto groupIter = std::find_if(m_groups.cbegin(), m_groups.cend(),
                                [&group](Group const &currentGroup) -> bool {
                                  return &currentGroup == &group;
                                });
  // Calling this function with a group that's not in the list is an error
  if (groupIter == m_groups.cend()) {
    throw std::runtime_error(
        std::string(
            "Internal error: could not find table location for group ") +
        group.name());
  }
  // Found the group so return its index as the path
  auto const groupIndex = static_cast<int>(groupIter - m_groups.cbegin());
  return {groupIndex};
}

MantidWidgets::Batch::RowPath ReductionJobs::getPath(Row const &row) const {
  auto groupIndex = 0;
  for (auto const &group : m_groups) {
    // See if the row is in this group
    auto const &rows = group.rows();
    auto rowIter =
        std::find_if(rows.cbegin(), rows.cend(),
                     [&row](boost::optional<Row> const &currentRow) -> bool {
                       return currentRow && &currentRow.get() == &row;
                     });
    if (rowIter == rows.cend()) {
      // Try the next group
      ++groupIndex;
      continue;
    }

    // Found the row, so return its group and row indices as the path
    auto const rowIndex = static_cast<int>(rowIter - rows.cbegin());
    return {groupIndex, rowIndex};
  }

  throw std::runtime_error(
      "Internal error: could not find table location for row");
}
} // namespace CustomInterfaces
} // namespace MantidQt
