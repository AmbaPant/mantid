// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IEXPERIMENTVIEW_H
#define MANTID_ISISREFLECTOMETRY_IEXPERIMENTVIEW_H

#include "DllConfig.h"
#include "GetInstrumentParameter.h"
#include "InstrumentParameters.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include "MantidQtWidgets/Common/Hint.h"
#include <map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IExperimentView

IExperimentView is the base view class for the Reflectometry experiment
settings. It
contains no QT specific functionality as that should be handled by a subclass.
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentViewSubscriber {
public:
  virtual void notifyPerAngleDefaultsChanged(int column, int row) = 0;
  virtual void notifySettingsChanged() = 0;
  virtual void notifySummationTypeChanged() = 0;
  virtual void notifyNewPerAngleDefaultsRequested() = 0;
  virtual void notifyRemovePerAngleDefaultsRequested(int index) = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IExperimentView {
public:
  virtual void subscribe(ExperimentViewSubscriber *notifyee) = 0;
  virtual void
  createStitchHints(const std::vector<MantidWidgets::Hint> &hints) = 0;

  virtual std::string getAnalysisMode() const = 0;
  virtual void setAnalysisMode(std::string const &analysisMode) = 0;

  virtual std::string getSummationType() const = 0;
  virtual void setSummationType(std::string const &summationType) = 0;

  virtual std::string getReductionType() const = 0;
  virtual void setReductionType(std::string const &reductionType) = 0;
  virtual void enableReductionType() = 0;
  virtual void disableReductionType() = 0;

  virtual std::vector<std::array<std::string, 8>>
  getPerAngleOptions() const = 0;
  virtual void showPerAngleOptionsAsInvalid(int row, int column) = 0;
  virtual void showPerAngleOptionsAsValid(int row) = 0;
  virtual void showAllPerAngleOptionsAsValid() = 0;
  virtual void showStitchParametersValid() = 0;
  virtual void showStitchParametersInvalid() = 0;

  virtual void enablePolarizationCorrections() = 0;
  virtual void disablePolarizationCorrections() = 0;
  virtual void enablePolarizationCorrectionInputs() = 0;
  virtual void disablePolarizationCorrectionInputs() = 0;

  virtual double getTransmissionStartOverlap() const = 0;
  virtual void setTransmissionStartOverlap(double start) = 0;
  virtual double getTransmissionEndOverlap() const = 0;
  virtual void setTransmissionEndOverlap(double end) = 0;
  virtual void showTransmissionRangeInvalid() = 0;
  virtual void showTransmissionRangeValid() = 0;

  virtual std::string getPolarizationCorrectionType() const = 0;
  virtual void setPolarizationCorrectionType(std::string const &type) = 0;
  virtual double getCRho() const = 0;
  virtual void setCRho(double cRho) = 0;
  virtual double getCAlpha() const = 0;
  virtual void setCAlpha(double cAlpha) = 0;
  virtual double getCAp() const = 0;
  virtual void setCAp(double cAp) = 0;
  virtual double getCPp() const = 0;
  virtual void setCPp(double cPp) = 0;

  virtual std::string getStitchOptions() const = 0;
  virtual void setStitchOptions(std::string const &stitchOptions) = 0;

  virtual void showOptionLoadErrors(
      std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
      std::vector<MissingInstrumentParameterValue> const &missingValues) = 0;

  virtual void disableAll() = 0;
  virtual void enableAll() = 0;

  virtual void addPerThetaDefaultsRow() = 0;
  virtual void removePerThetaDefaultsRow(int rowIndex) = 0;

  virtual void showPerAngleThetasNonUnique(double tolerance) = 0;

  virtual ~IExperimentView() = default;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IEXPERIMENTVIEW_H */
