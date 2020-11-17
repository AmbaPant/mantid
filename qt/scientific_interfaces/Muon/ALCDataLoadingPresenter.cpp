// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCDataLoadingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Strings.h"

#include "ALCLatestFileFinder.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MuonAnalysisHelper.h"

#include "Poco/File.h"
#include <Poco/ActiveResult.h>
#include <Poco/Path.h>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace MantidQt::API;

namespace MantidQt {
namespace CustomInterfaces {
ALCDataLoadingPresenter::ALCDataLoadingPresenter(IALCDataLoadingView *view)
    : m_view(view), m_numDetectors(0), m_loadingData(false) {}

void ALCDataLoadingPresenter::initialize() {
  m_view->initialize();

  connect(m_view, SIGNAL(loadRequested()), SLOT(handleLoadRequested()));
  connect(m_view, SIGNAL(runsSelected()), SLOT(updateAvailableInfo()));
  connect(m_view, SIGNAL(instrumentChangedSignal(std::string)), SLOT());
  connect(m_view, SIGNAL(pathChangedSignal(std::string)), SLOT());
  connect(m_view, SIGNAL(runsChanged(std::string)),
          SLOT(handleRunsChanged(std::string)));
}

/**
 * Converts a range of ints as string into a vector of integers
 * @param range :: String in the form "<n>-<m>"
 * @return Vector of all ints between the range
 * @throws std::runtime_error if m > n
 */
std::vector<int> ALCDataLoadingPresenter::unwrapRange(std::string range) {
  std::vector<int> runs;

  auto beginningString = range.substr(0, range.find('-'));
  auto endString = range.substr(range.find('-') + 1);
  auto beginning = std::stoi(beginningString);
  auto end = std::stoi(endString);

  // If beginning longer then and, assume end is short hand e.g. 100-3 -> 100-103
  if (beginningString.length() > endString.length()) {
    for (int i = beginning; i <= beginning+end; ++i)
      runs.emplace_back(i);
  } else {
    // Assumed not using short hand so end must be > beginning
    if (end < beginning)
      throw std::runtime_error(
          "Decreasing range is not allowed, try " + std::to_string(beginning) +
          "-" + std::to_string(end) + " instead."); // Needs error message
    for (int i = beginning; i <= end; ++i)
      runs.emplace_back(i);
  }
  return runs;
}

/**
 * Validates the given expression and returns vector of run numbers
 * Can be a comma separated list e.g. 1,2,3
 * Can be a range e.g. 100-103 which is equally 100-3
 * Can be a mixture e.g. 1,2-5,9,10-12
 * @param runs :: expression to be validated
 * @return Vector of run numbers
 */
std::vector<int>
ALCDataLoadingPresenter::validateAndGetRunsFromExpression(std::string runs) {
  std::vector<int> runNumbers;
  std::stringstream ss(runs);
  std::string token;

  while (std::getline(ss, token, ',')) {
    // Try find -
    auto index = token.find('-');
    if (index == std::string::npos) {
      // Convert staright to in and add to list
      const int number = std::stoi(token);
      runNumbers.emplace_back(number);
    } else {
      // Unwrap range and add each separately
      auto numbers = unwrapRange(token); // Throws if not successful
      for (const auto &number : numbers)
        runNumbers.emplace_back(number);
    }
  }
  return runNumbers;
}

void ALCDataLoadingPresenter::handleRunsChanged(std::string runs) {
  // Validate runs and get as run numbers
  // if valid attempt to load first run
  // if loaded set path
  // if path update available info
  // if fails need error message
  // if success activate load button
  try {
    auto runNumbers = validateAndGetRunsFromExpression(runs);
    m_view->enableLoad(true);
  } catch (std::runtime_error e) {
    m_view->displayError(
        std::string(e.what()) +
        "\n\nCan specify a list of runs by a dash \ne.g. 1-10\nCan specify "
        "specific runs with a comma separated list \ne.g. 1-10, 15, 20-30\n "
        "Range must go in increasing order, \ne.g. 1-10, 15, 20-30");
  }
}

/**
 * Called when the Load button is clicked.
 * Gets last run, passes it to load method if not "auto".
 * If it was "auto", sets up a watcher to automatically reload on new files.
 */
void ALCDataLoadingPresenter::handleLoadRequested() {
  // Get runs
  /*
  // Check not empty
  if (files.empty()) {
    m_view->displayError(
        "Error message:\n" + m_view->getRunsErrorMessage() +
        "\n\nCan specify a list of runs by a dash \ne.g. 1-10\nCan specify "
        "specific runs with a comma separated list \ne.g. 1-10, 15, 20-30\n "
        "Range must go in increasing order, \ne.g. 1-10, 15, 20-30");
    return;
  }

  // Now perform the load
  load(files);
  */
}

/**
 * Remove the run number from a full file path
 * @param file :: [input] full path which contains a run number
 * @return An integer representation of the run number
 */
int ALCDataLoadingPresenter::extractRunNumber(const std::string &file) {
  if (file.empty())
    return -1;

  auto returnVal = file;
  // Strip beginning of path to just the run (e.g. MUSR00015189.nxs)
  std::size_t found = returnVal.find_last_of("/\\");
  returnVal = returnVal.substr(found + 1);

  // Remove all non-digits
  returnVal.erase(std::remove_if(returnVal.begin(), returnVal.end(),
                                 [](auto c) { return !std::isdigit(c); }),
                  returnVal.end());

  // Return run number as int (removes leading 0's)
  return std::stoi(returnVal);
}

/**
 * Load new data and update the view accordingly
 * @param files :: [input] range of files (user-specified or auto generated)
 */
void ALCDataLoadingPresenter::load(const std::vector<std::string> &files) {

  m_loadingData = true;
  m_view->disableAll();

  // Before loading, check custom grouping (if used) is sensible
  const bool groupingOK = checkCustomGrouping();
  if (!groupingOK) {
    m_view->displayError(
        "Custom grouping not valid (bad format or detector numbers)");
    m_view->enableAll();
    return;
  }

  if (files.empty()) {
    m_view->displayError("The list of files to load is empty. ");
    m_view->enableAll();
    m_loadingData = false;
    return;
  }

  try {

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("PlotAsymmetryByLogValue");
    alg->setChild(true); // Don't want workspaces in the ADS

    // Change first last run to WorkspaceNames
    alg->setProperty("WorkspaceNames", files);
    alg->setProperty("LogValue", m_view->log());
    alg->setProperty("Function", m_view->function());
    alg->setProperty("Type", m_view->calculationType());
    alg->setProperty("DeadTimeCorrType", m_view->deadTimeType());
    alg->setProperty("Red", m_view->redPeriod());

    // If time limiting requested, set min/max times
    if (auto timeRange = m_view->timeRange()) {
      double timeMin = (*timeRange).first;
      double timeMax = (*timeRange).second;
      if (timeMin >= timeMax) {
        throw std::invalid_argument("Invalid time limits");
      }
      alg->setProperty("TimeMin", timeMin);
      alg->setProperty("TimeMax", timeMax);
    }

    // If corrections from custom file requested, set file property
    if (m_view->deadTimeType() == "FromSpecifiedFile") {
      alg->setProperty("DeadTimeCorrFile", m_view->deadTimeFile());
    }

    // If custom grouping requested, set forward/backward groupings
    if (m_view->detectorGroupingType() == "Custom") {
      alg->setProperty("ForwardSpectra", m_view->getForwardGrouping());
      alg->setProperty("BackwardSpectra", m_view->getBackwardGrouping());
    }

    // If Subtract checkbox is selected, set green period
    if (m_view->subtractIsChecked()) {
      alg->setProperty("Green", m_view->greenPeriod());
    }

    alg->setPropertyValue("OutputWorkspace", "__NotUsed");

    // Set loading alg equal to alg
    this->m_LoadingAlg = alg;
    // Execute async so we can show progress bar
    Poco::ActiveResult<bool> result(alg->executeAsync());
    while (!result.available()) {
      QCoreApplication::processEvents();
    }
    if (!result.error().empty()) {
      throw std::runtime_error(result.error());
    }

    MatrixWorkspace_sptr tmp = alg->getProperty("OutputWorkspace");
    IAlgorithm_sptr sortAlg = AlgorithmManager::Instance().create("SortXAxis");
    sortAlg->setChild(true); // Don't want workspaces in the ADS
    sortAlg->setProperty("InputWorkspace", tmp);
    sortAlg->setProperty("Ordering", "Ascending");
    sortAlg->setProperty("OutputWorkspace", "__NotUsed__");
    sortAlg->execute();

    m_loadedData = sortAlg->getProperty("OutputWorkspace");

    // If errors are properly caught, shouldn't happen
    assert(m_loadedData);
    // If subtract is not checked, only one spectrum,
    // else four spectra
    if (!m_view->subtractIsChecked()) {
      assert(m_loadedData->getNumberHistograms() == 1);
    } else {
      assert(m_loadedData->getNumberHistograms() == 4);
    }

    // Plot spectrum 0. It is either red period (if subtract is unchecked) or
    // red - green (if subtract is checked)
    m_view->setDataCurve(m_loadedData);

    emit dataChanged();

  } catch (const std::invalid_argument &e) {
    m_view->displayError(e.what());
  } catch (std::exception &e) {
    m_view->displayError(e.what());
  }
  m_view->enableAll();
  m_loadingData = false;
}

void ALCDataLoadingPresenter::updateAvailableInfo() {
  Workspace_sptr loadedWs;
  double firstGoodData = 0, timeZero = 0;

  try //... to load the first run
  {
    IAlgorithm_sptr loadAlg =
        AlgorithmManager::Instance().create("LoadMuonNexus");
    loadAlg->setChild(true); // Don't want workspaces in the ADS

    //// Need first run here to update available info
    // loadAlg->setProperty("Filename", m_view->firstRun());

    // We need logs only but we have to use LoadMuonNexus
    // (can't use LoadMuonLogs as not all the logs would be
    // loaded), so we load the minimum amount of data, i.e., one spectrum
    loadAlg->setPropertyValue("SpectrumMin", "1");
    loadAlg->setPropertyValue("SpectrumMax", "1");
    loadAlg->setPropertyValue("OutputWorkspace", "__NotUsed");
    loadAlg->execute();

    loadedWs = loadAlg->getProperty("OutputWorkspace");
    firstGoodData = loadAlg->getProperty("FirstGoodData");
    timeZero = loadAlg->getProperty("TimeZero");
  } catch (...) {
    m_view->setAvailableLogs(std::vector<std::string>()); // Empty logs list
    m_view->setAvailablePeriods(
        std::vector<std::string>()); // Empty period list
    m_view->setTimeLimits(0, 0);     // "Empty" time limits
    return;
  }

  // Set logs
  MatrixWorkspace_const_sptr ws = MuonAnalysisHelper::firstPeriod(loadedWs);
  std::vector<std::string> logs;

  const auto &properties = ws->run().getProperties();
  for (auto property : properties) {
    logs.emplace_back(property->name());
  }
  m_view->setAvailableLogs(logs);

  // Set periods
  size_t numPeriods = MuonAnalysisHelper::numPeriods(loadedWs);
  std::vector<std::string> periods;
  for (size_t i = 0; i < numPeriods; i++) {
    std::stringstream buffer;
    buffer << i + 1;
    periods.emplace_back(buffer.str());
  }
  m_view->setAvailablePeriods(periods);

  // Set time limits if this is the first data loaded (will both be zero)
  if (auto timeLimits = m_view->timeRange()) {
    if (std::abs(timeLimits->first) < 0.0001 &&
        std::abs(timeLimits->second) < 0.0001) {
      m_view->setTimeLimits(firstGoodData - timeZero, ws->x(0).back());
    }
  }

  // Update number of detectors for this new first run
  m_numDetectors = ws->getInstrument()->getNumberDetectors();
}

MatrixWorkspace_sptr ALCDataLoadingPresenter::exportWorkspace() {
  if (m_loadedData)
    return std::const_pointer_cast<MatrixWorkspace>(m_loadedData);
  return MatrixWorkspace_sptr();
}

void ALCDataLoadingPresenter::setData(const MatrixWorkspace_sptr &data) {

  if (data) {
    // Set the data
    m_loadedData = data;
    // Plot the data
    m_view->setDataCurve(m_loadedData);

  } else {
    std::invalid_argument("Cannot load an empty workspace");
  }
}

/**
 * If custom grouping is supplied, check all detector numbers are valid
 * @returns :: True if grouping OK, false if bad
 */
bool ALCDataLoadingPresenter::checkCustomGrouping() {
  bool groupingOK = true;
  if (m_view->detectorGroupingType() == "Custom") {
    auto detectors = Mantid::Kernel::Strings::parseRange(
        isCustomGroupingValid(m_view->getForwardGrouping(), groupingOK));
    const auto backward = Mantid::Kernel::Strings::parseRange(
        isCustomGroupingValid(m_view->getBackwardGrouping(), groupingOK));
    if (!groupingOK) {
      return false;
    }
    detectors.insert(detectors.end(), backward.begin(), backward.end());
    if (std::any_of(detectors.cbegin(), detectors.cend(),
                    [this](const auto det) {
                      return det < 0 || det > static_cast<int>(m_numDetectors);
                    })) {
      groupingOK = false;
    }
  }
  return groupingOK;
}
/**
 * Check basic group string is valid
 * i.e. does not contain letters or start with , or -
 * @param group :: the string of the grouping
 * @param isValid :: bool to say if the string is valid
 * @returns :: True if grouping OK, false if bad
 */
std::string
ALCDataLoadingPresenter::isCustomGroupingValid(const std::string &group,
                                               bool &isValid) {
  if (!std::isdigit(group[0]) ||
      std::any_of(std::begin(group), std::end(group), ::isalpha)) {
    isValid = false;
    return "";
  }
  return group;
}
/**
 * If currently loading data
 * @returns :: True if currently in load() method
 */
bool ALCDataLoadingPresenter::isLoading() const { return m_loadingData; }
/**
 * Cancels current loading algorithm
 */
void ALCDataLoadingPresenter::cancelLoading() const { m_LoadingAlg->cancel(); }

} // namespace CustomInterfaces
} // namespace MantidQt
