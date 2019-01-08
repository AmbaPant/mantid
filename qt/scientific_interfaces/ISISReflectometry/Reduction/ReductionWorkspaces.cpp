// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReductionWorkspaces.h"
#include "Common/Map.h"

namespace MantidQt {
namespace CustomInterfaces {
ReductionWorkspaces::ReductionWorkspaces(
    // cppcheck-suppress passedByValue
    std::vector<std::string> timeOfFlight,
    // cppcheck-suppress passedByValue
    std::pair<std::string, std::string> transmissionRuns,
    // cppcheck-suppress passedByValue
    std::string combinedTransmissionRuns,
    // cppcheck-suppress passedByValue
    std::string iVsLambda,
    // cppcheck-suppress passedByValue
    std::string iVsQ,
    // cppcheck-suppress passedByValue
    std::string iVsQBinned)
    : m_timeOfFlight(std::move(timeOfFlight)),
      m_transmissionRuns(std::move(transmissionRuns)),
      m_combinedTransmissionRuns(combinedTransmissionRuns),
      m_iVsLambda(std::move(iVsLambda)), m_iVsQ(std::move(iVsQ)),
      m_iVsQBinned(std::move(iVsQBinned)) {}

std::vector<std::string> const &ReductionWorkspaces::timeOfFlight() const {
  return m_timeOfFlight;
}

std::pair<std::string, std::string> const &
ReductionWorkspaces::transmissionRuns() const {
  return m_transmissionRuns;
}

std::string const &ReductionWorkspaces::combinedTransmissionRuns() const {
  return m_combinedTransmissionRuns;
}

std::string const &ReductionWorkspaces::iVsLambda() const {
  return m_iVsLambda;
}

std::string const &ReductionWorkspaces::iVsQ() const { return m_iVsQ; }

std::string const &ReductionWorkspaces::iVsQBinned() const {
  return m_iVsQBinned;
}

bool operator==(ReductionWorkspaces const &lhs,
                ReductionWorkspaces const &rhs) {
  return lhs.timeOfFlight() == rhs.timeOfFlight() &&
         lhs.transmissionRuns() == rhs.transmissionRuns() &&
         lhs.iVsLambda() == rhs.iVsLambda() && lhs.iVsQ() == rhs.iVsQ() &&
         lhs.iVsQBinned() == rhs.iVsQBinned();
}

bool operator!=(ReductionWorkspaces const &lhs,
                ReductionWorkspaces const &rhs) {
  return !(lhs == rhs);
}

std::pair<std::string, std::string> transmissionWorkspaceNames(
    std::pair<std::string, std::string> const &transmissionRuns) {
  if (!transmissionRuns.first.empty()) {
    auto first = "TRANS_" + transmissionRuns.first;
    if (!transmissionRuns.second.empty()) {
      auto second = "TRANS_" + transmissionRuns.second;
      return std::make_pair(first, second);
    } else {
      return std::make_pair(first, std::string());
    }
  } else {
    return std::pair<std::string, std::string>();
  }
}

std::string transmissionWorkspacesCombined(
    std::pair<std::string, std::string> const &transmissionRuns) {
  if (!transmissionRuns.first.empty()) {
    auto first = "TRANS_" + transmissionRuns.first;
    if (!transmissionRuns.second.empty()) {
      return first + "_" + transmissionRuns.second;
    } else {
      return first;
    }
  } else {
    return std::string();
  }
}

ReductionWorkspaces
workspaceNames(std::vector<std::string> const &summedRunNumbers,
               std::pair<std::string, std::string> const &transmissionRuns) {

  auto tofWorkspaces =
      map(summedRunNumbers, [](std::string const &runNumber) -> std::string {
        return "TOF_" + runNumber;
      });

  auto joinedRuns = boost::algorithm::join(summedRunNumbers, "+");
  auto iVsLambda = "IvsLam_" + joinedRuns;
  auto iVsQ = "IvsQ_" + joinedRuns;
  auto iVsQBinned = "IvsQ_binned_" + joinedRuns;
  auto transmissionWorkspaces = transmissionWorkspaceNames(transmissionRuns);
  auto combinedTransmissionWorkspace =
      transmissionWorkspacesCombined(transmissionRuns);

  return ReductionWorkspaces(
      std::move(tofWorkspaces), std::move(transmissionWorkspaces),
      std::move(combinedTransmissionWorkspace), std::move(iVsLambda),
      std::move(iVsQ), std::move(iVsQBinned));
}

std::string postprocessedWorkspaceName(
    std::vector<std::vector<std::string> const *> const &summedRunNumbers) {
  auto summedRunList =
      map(summedRunNumbers,
          [](std::vector<std::string> const *summedRuns) -> std::string {
            return boost::algorithm::join(*summedRuns, "+");
          });
  return boost::algorithm::join(summedRunList, "_");
}
} // namespace CustomInterfaces
} // namespace MantidQt
