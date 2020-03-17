// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/MWRunFiles.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentView.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"

#include <QObject>
#include <QSplitter>
#include <QString>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class  MANTIDQT_DIRECT_DLL IALFCustomInstrumentView : public MantidWidgets::IBaseCustomInstrumentView{
public:
  virtual void observeExtractSingleTube(Observer *listner)=0;
  virtual void observeAverageTube(Observer *listner)=0;
  virtual void addSpectrum(std::string wsName) = 0;
  virtual void setupAnalysisPane(MantidWidgets::PlotFitAnalysisPaneView *analysis) = 0;
};

class DLLExport ALFCustomInstrumentView : public  MantidWidgets::BaseCustomInstrumentView 
  {Q_OBJECT

public:
  explicit ALFCustomInstrumentView(const std::string &instrument,
                                   QWidget *parent = nullptr);
  void observeExtractSingleTube(Observer *listner);// override;
  void observeAverageTube(Observer *listner);// override;
  void
  setUpInstrument(const std::string &fileName,
                  std::vector<std::function<bool(std::map<std::string, bool>)>>
                      &binders) override;

  void addObserver(std::tuple<std::string, Observer *> &listener) override;
  void addSpectrum(std::string wsName);// override;
  void setupAnalysisPane(MantidWidgets::PlotFitAnalysisPaneView *analysis);// override;

public slots:
  void extractSingleTube();
  void averageTube();

private:
  Observable *m_extractSingleTubeObservable;
  Observable *m_averageTubeObservable;
  QAction *m_extractAction;
  QAction *m_averageAction;
  MantidWidgets::PlotFitAnalysisPaneView *m_analysisPane;
};
} // namespace CustomInterfaces
} // namespace MantidQt
