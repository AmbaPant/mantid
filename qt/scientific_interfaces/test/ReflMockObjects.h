// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H

#include "Common/IMessageHandler.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "GUI/Instrument/InstrumentOptionDefaults.h"
#include "GUI/MainWindow/IMainWindowPresenter.h"
#include "GUI/MainWindow/IMainWindowView.h"
#include "GUI/Runs/IReflAutoreduction.h"
#include "GUI/Runs/IReflSearcher.h"
#include "GUI/Runs/ReflSearchModel.h"
#include "GUI/Save/IAsciiSaver.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidQtWidgets/Common/Hint.h"
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::DataProcessor;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/**** Models ****/

class MockReflSearchModel : public ReflSearchModel {
public:
  MockReflSearchModel(std::string const &run, std::string const &description,
                      std::string const &location)
      : ReflSearchModel(ITableWorkspace_sptr(), std::string()),
        m_result(run, description, location) {}
  ~MockReflSearchModel() override {}
  MOCK_CONST_METHOD2(data, QVariant(const QModelIndex &, int role));
  MOCK_METHOD2(setError, void(int, std::string const &));

  SearchResult const &getRowData(int) const override { return m_result; }

private:
  SearchResult m_result;
};

/**** Views ****/

class MockMainWindowView : public IMainWindowView {
public:
  MOCK_METHOD3(askUserString,
               std::string(const std::string &, const std::string &,
                           const std::string &));
  MOCK_METHOD2(askUserYesNo, bool(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserWarning, void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
  ~MockMainWindowView() override{};
};

/**** Presenters ****/

class MockMainWindowPresenter : public IMainWindowPresenter {
public:
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
  MOCK_METHOD1(settingsChanged, void(int));
  bool isProcessing() const override { return false; }

  ~MockMainWindowPresenter() override{};
};

class MockBatchPresenter : public IBatchPresenter {
public:
  MOCK_METHOD0(notifyReductionResumed, void());
  MOCK_METHOD0(notifyReductionPaused, void());
  MOCK_METHOD2(notifyReductionCompletedForGroup,
               void(GroupData const &, std::string const &));
  MOCK_METHOD2(notifyReductionCompletedForRow,
               void(GroupData const &, std::string const &));
  MOCK_METHOD0(notifyAutoreductionResumed, void());
  MOCK_METHOD0(notifyAutoreductionPaused, void());
  MOCK_METHOD0(notifyAutoreductionCompleted, void());

  MOCK_CONST_METHOD1(getOptionsForAngle, OptionsQMap(const double));
  MOCK_CONST_METHOD0(hasPerAngleOptions, bool());
  MOCK_METHOD1(notifyInstrumentChanged, void(const std::string &));
  MOCK_METHOD0(notifySettingsChanged, void());
  MOCK_CONST_METHOD0(isProcessing, bool());
  MOCK_CONST_METHOD0(isAutoreducing, bool());
  MOCK_CONST_METHOD0(requestClose, bool());
};

/**** Progress ****/

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
  ~MockProgressBase() override {}
};

/**** Catalog ****/

class MockICatalogInfo : public Mantid::Kernel::ICatalogInfo {
public:
  MOCK_CONST_METHOD0(catalogName, const std::string());
  MOCK_CONST_METHOD0(soapEndPoint, const std::string());
  MOCK_CONST_METHOD0(externalDownloadURL, const std::string());
  MOCK_CONST_METHOD0(catalogPrefix, const std::string());
  MOCK_CONST_METHOD0(windowsPrefix, const std::string());
  MOCK_CONST_METHOD0(macPrefix, const std::string());
  MOCK_CONST_METHOD0(linuxPrefix, const std::string());
  MOCK_CONST_METHOD0(clone, ICatalogInfo *());
  MOCK_CONST_METHOD1(transformArchivePath, std::string(const std::string &));
  ~MockICatalogInfo() override {}
};

class MockAsciiSaver : public IAsciiSaver {
public:
  MOCK_CONST_METHOD1(isValidSaveDirectory, bool(std::string const &));
  MOCK_CONST_METHOD4(save,
                     void(std::string const &, std::vector<std::string> const &,
                          std::vector<std::string> const &,
                          FileFormatOptions const &));
  virtual ~MockAsciiSaver() = default;
};

class MockReflSearcher : public IReflSearcher {
public:
  MOCK_METHOD1(search, Mantid::API::ITableWorkspace_sptr(const std::string &));
};

/**** Catalog ****/
class MockMessageHandler : public IMessageHandler {
public:
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
};

/**** Autoreduction ****/
class MockReflAutoreduction : public IReflAutoreduction {
public:
  MOCK_CONST_METHOD0(running, bool());
  MOCK_CONST_METHOD1(searchStringChanged, bool(const std::string &));
  MOCK_CONST_METHOD0(searchResultsExist, bool());
  MOCK_METHOD0(setSearchResultsExist, void());

  MOCK_METHOD1(setupNewAutoreduction, bool(const std::string &));
  MOCK_METHOD0(pause, bool());
  MOCK_METHOD0(stop, void());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif /*MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H*/
