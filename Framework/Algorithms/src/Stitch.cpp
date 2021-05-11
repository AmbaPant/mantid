// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/Stitch.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
static const std::string INPUT_WORKSPACE_PROPERTY = "InputWorkspaces";
static const std::string REFERENCE_WORKSPACE_NAME = "ReferenceWorkspace";
static const std::string COMBINATION_BEHAVIOUR = "CombinationBehaviour";
static const std::string SCALE_FACTOR_CALCULATION = "ScaleFactorCalculation";
static const std::string OUTPUT_WORKSPACE_PROPERTY = "OutputWorkspace";

/**
 * @brief Calculates the x-axis extent of a single spectrum workspace
 * Assumes that the histogram bin edges are in ascending order
 * @param ws : the input workspace
 * @return a pair of min-x and max-x
 */
std::pair<double, double> getInterval(const MatrixWorkspace &ws) {
  return std::make_pair(ws.readX(0).front(), ws.readX(0).back());
}

/**
 * @brief Compares two workspaces in terms of their x-coverage
 * @param ws1 : input workspace 1
 * @param ws2 : input workspace 2
 * @return true if ws1 is less than ws2 in terms of its x-interval
 */
bool compareInterval(const MatrixWorkspace_sptr ws1, const MatrixWorkspace_sptr ws2) {
  const auto minmax1 = getInterval(*ws1);
  const auto minmax2 = getInterval(*ws2);
  if (minmax1.first < minmax2.first) {
    return true;
  } else if (minmax1.first > minmax2.first) {
    return false;
  } else {
    return minmax1.second < minmax2.second;
  }
}

/**
 * @brief Returns the overlap of two workspaces in x-axis
 * @param ws1 : input workspace 1
 * @param ws2 : input workspace 2
 * @return the x-axis covered by both
 */
std::pair<double, double> getOverlap(const MatrixWorkspace_sptr ws1, const MatrixWorkspace_sptr ws2) {
  const auto minmax1 = getInterval(*ws1);
  const auto minmax2 = getInterval(*ws2);
  return std::make_pair(std::max(minmax1.first, minmax2.first), std::min(minmax1.second, minmax2.second));
}

/**
 * @brief Calculates the median of a vector
 * @param vec : input vector
 * @return the median
 */
double median(std::vector<double> &vec) {
  if (vec.empty())
    return 0;
  std::sort(vec.begin(), vec.end());
  const size_t s = vec.size();
  if (s % 2 == 0) {
    return 0.5 * (vec[s / 2] + vec[s / 2 - 1]);
  } else {
    return vec[s / 2];
  }
}

} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Stitch)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string Stitch::name() const { return "Stitch"; }

/// Algorithm's version for identification. @see Algorithm::version
int Stitch::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string Stitch::category() const { return "Transforms\\Merging"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string Stitch::summary() const { return "Stitches overlapping spectra from multiple workspaces."; }

/// Validate the input workspaces for compatibility
std::map<std::string, std::string> Stitch::validateInputs() {
  std::map<std::string, std::string> issues;
  const std::vector<std::string> inputs_given = getProperty(INPUT_WORKSPACE_PROPERTY);
  std::vector<std::string> workspaces;
  RunCombinationHelper combHelper;
  try {
    workspaces = combHelper.unWrapGroups(inputs_given);
  } catch (const std::exception &e) {
    issues[INPUT_WORKSPACE_PROPERTY] = std::string(e.what());
    return issues;
  }
  if (workspaces.size() < 2) {
    issues[INPUT_WORKSPACE_PROPERTY] = "Please provide at least 2 workspaces to stitch.";
    return issues;
  }
  try {
    const auto first = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaces.front());
    combHelper.setReferenceProperties(first);
    if (first->getNumberHistograms() > 1) {
      issues[INPUT_WORKSPACE_PROPERTY] = "Input workspaces must have one spectrum each";
      return issues;
    }

  } catch (const std::exception &e) {
    issues[INPUT_WORKSPACE_PROPERTY] =
        std::string("Please provide MatrixWorkspaces as or groups of those as input: ") + e.what();
    return issues;
  }
  if (!isDefault(REFERENCE_WORKSPACE_NAME)) {
    const auto referenceName = getPropertyValue(REFERENCE_WORKSPACE_NAME);
    if (std::find_if(workspaces.cbegin(), workspaces.cend(),
                     [&referenceName](const auto wsName) { return wsName == referenceName; }) == workspaces.cend()) {
      issues[REFERENCE_WORKSPACE_NAME] = "Reference workspace must be one of the input workspaces";
      return issues;
    }
  }
  for (const auto &wsName : workspaces) {
    const auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    // check if all the others are compatible with the first one
    const std::string compatible = combHelper.checkCompatibility(ws, true);
    if (!compatible.empty()) {
      issues[INPUT_WORKSPACE_PROPERTY] += "Workspace " + ws->getName() + " is not compatible: " + compatible + "\n";
    }
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Stitch::init() {
  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(INPUT_WORKSPACE_PROPERTY, std::make_unique<ADSValidator>()),
      "The names of the input workspaces or groups of those as a list. "
      "At least two compatible MatrixWorkspaces are required, having one spectrum each. ");
  declareProperty(REFERENCE_WORKSPACE_NAME, "",
                  "The name of the workspace that will serve as the reference; "
                  "that is, the one that will not be scaled. If left blank, "
                  "stitching will be performed left to right.");
  declareProperty(COMBINATION_BEHAVIOUR, "Interleave",
                  std::make_unique<ListValidator<std::string>>(std::array<std::string, 1>{"Interleave"}));
  declareProperty(SCALE_FACTOR_CALCULATION, "MedianOfRatios",
                  std::make_unique<ListValidator<std::string>>(std::array<std::string, 1>{"MedianOfRatios"}));
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>(OUTPUT_WORKSPACE_PROPERTY, "", Direction::Output),
                  "The output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Stitch::exec() {
  std::vector<MatrixWorkspace_sptr> workspaces;
  const auto referenceName = getPropertyValue(REFERENCE_WORKSPACE_NAME);
  const auto combinationBehaviour = getPropertyValue(COMBINATION_BEHAVIOUR);
  const auto scaleFactorCalculation = getPropertyValue(SCALE_FACTOR_CALCULATION);
  const auto inputs = RunCombinationHelper::unWrapGroups(getProperty(INPUT_WORKSPACE_PROPERTY));
  std::transform(inputs.cbegin(), inputs.cend(), std::back_inserter(workspaces),
                 [](const auto ws) { return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(ws); });
  std::sort(workspaces.begin(), workspaces.end(), compareInterval);
  size_t referenceIndex = 0;
  if (!isDefault(REFERENCE_WORKSPACE_NAME)) {
    const auto ref = std::find_if(workspaces.cbegin(), workspaces.cend(),
                                  [&referenceName](const auto ws) { return ws->getName() == referenceName; });
    referenceIndex = std::distance(workspaces.cbegin(), ref);
  }
  size_t leftIterator = referenceIndex, rightIterator = referenceIndex;
  while (leftIterator > 0) {
    scale(workspaces[leftIterator]->clone(), workspaces[leftIterator - 1]->clone());
    --leftIterator;
  }
  while (rightIterator < workspaces.size() - 1) {
    scale(workspaces[rightIterator]->clone(), workspaces[rightIterator + 1]->clone());
    ++rightIterator;
  }
  MatrixWorkspace_sptr merged = merge(inputs);
  setProperty(OUTPUT_WORKSPACE_PROPERTY, merged);
}

MatrixWorkspace_sptr Stitch::merge(const std::vector<std::string> &inputs) {
  // interleave option is equivalent to concatenation followed by sort X axis
  auto joiner = createChildAlgorithm("ConjoinXRuns");
  joiner->setProperty("InputWorkspaces", inputs);
  joiner->setProperty("OutputWorkspace", "__joined");
  joiner->execute();
  MatrixWorkspace_sptr joined = joiner->getProperty("OutputWorkspace");

  auto sorter = createChildAlgorithm("SortXAxis");
  sorter->setProperty("InputWorkspace", joined);
  sorter->setProperty("OutputWorkspace", "__sorted");
  sorter->execute();
  return sorter->getProperty("OutputWorkspace");
}

void Stitch::scale(MatrixWorkspace_sptr wsToMatch, MatrixWorkspace_sptr wsToScale) {
  const auto overlap = getOverlap(wsToMatch, wsToScale);
  auto cropper = createChildAlgorithm("CropWorkspaceRagged");
  cropper->setProperty("XMin", std::vector<double>({overlap.first}));
  cropper->setProperty("XMax", std::vector<double>({overlap.second}));

  cropper->setProperty("InputWorkspace", wsToMatch);
  cropper->setProperty("OutputWorkspace", "__to_match");
  cropper->execute();
  MatrixWorkspace_sptr croppedToMatch = cropper->getProperty("OutputWorkspace");
  cropper->setProperty("InputWorkspace", wsToScale);
  cropper->setProperty("OutputWorkspace", "__to_scale");
  cropper->execute();
  MatrixWorkspace_sptr croppedToScale = cropper->getProperty("OutputWorkspace");

  MatrixWorkspace_sptr rebinnedToScale;
  if (wsToMatch->isHistogramData()) {
    auto rebinner = createChildAlgorithm("RebinToWorkspace");
    rebinner->setProperty("WorkspaceToMatch", croppedToMatch);
    rebinner->setProperty("WorkspaceToRebin", croppedToScale);
    rebinner->setProperty("OutputWorkspace", "__rebinned");
    rebinner->execute();
    rebinnedToScale = rebinner->getProperty("OutputWorkspace");
  } else {
    auto interpolator = createChildAlgorithm("SplineInterpolation");
    interpolator->setProperty("WorkspaceToMatch", croppedToMatch);
    interpolator->setProperty("WorkspaceToInterpolate", croppedToScale);
    interpolator->setProperty("OutputWorkspace", "__interpolated");
    interpolator->execute();
    rebinnedToScale = interpolator->getProperty("OutputWorkspace");
  }

  auto divider = createChildAlgorithm("Divide");
  divider->setProperty("LHSWorkspace", rebinnedToScale);
  divider->setProperty("RHSWorkspace", croppedToMatch);
  divider->setProperty("OutputWorkspace", "__ratio");
  divider->execute();
  MatrixWorkspace_sptr ratio = divider->getProperty("OutputWorkspace");

  auto scaler = createChildAlgorithm("Scale");
  scaler->setProperty("InputWorkspace", wsToScale);
  scaler->setProperty("OutputWorkspace", "__scaled");
  scaler->setProperty("Factor", 1 / median(ratio->dataY(0)));
  scaler->execute();
  wsToScale = scaler->getProperty("OutputWorkspace");
}

} // namespace Algorithms
} // namespace Mantid
