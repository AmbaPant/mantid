// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "FunctionModelDataset.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

/** IFunctionBrowser: interface for FunctionBrowser

  Abstract base class to be implemented
*/
class EXPORT_OPT_MANTIDQT_COMMON IFunctionBrowser {
public:
  virtual ~IFunctionBrowser() = default;
  virtual QString getFunctionString() = 0;
  virtual void updateParameters(const Mantid::API::IFunction &fun) = 0;
  virtual void clear() = 0;
  virtual void setErrorsEnabled(bool enabled) = 0;
  virtual void clearErrors() = 0;
  virtual void setFunction(const QString &funStr) = 0;
  virtual void setNumberOfDatasets(int n) = 0;
  virtual void setDatasets(const QStringList &datasetNames) = 0;
  virtual void setDatasets(const QList<FunctionModelDataset> &datasets) = 0;
  virtual Mantid::API::IFunction_sptr getGlobalFunction() = 0;
  virtual void updateMultiDatasetParameters(const Mantid::API::IFunction &fun) = 0;
  virtual void updateMultiDatasetParameters(const Mantid::API::ITableWorkspace &paramTable) = 0;
  virtual bool isLocalParameterFixed(const QString &parName, int i) const = 0;
  virtual double getLocalParameterValue(const QString &parName, int i) const = 0;
  virtual QString getLocalParameterTie(const QString &parName, int i) const = 0;
  virtual int getNumberOfDatasets() const = 0;
  virtual QStringList getDatasetNames() const = 0;
  virtual QStringList getDatasetDomainNames() const = 0;
  virtual void setLocalParameterValue(const QString &parName, int i, double value) = 0;
  virtual void setLocalParameterFixed(const QString &parName, int i, bool fixed) = 0;
  virtual void setLocalParameterTie(const QString &parName, int i, QString tie) = 0;
  virtual void setCurrentDataset(int i) = 0;
  virtual int getCurrentDataset() const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
