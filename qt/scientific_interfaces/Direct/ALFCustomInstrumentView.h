// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

class ALFCustomInstrumentView : public MantidWidgets::BaseCustomInstrumentView {
  Q_OBJECT

public:
  explicit ALFCustomInstrumentView(const std::string &instrument,
                                   QWidget *parent = nullptr);
  void observeExtractSingleTube(Observer *listner);
  void observeAverageTube(Observer *listner);

  void
  setUpInstrument(const std::string &fileName,
                  std::vector<std::function<bool(std::map<std::string, bool>)>>
                      &binders) override;

  void addObserver(std::tuple<std::string, Observer *> &listener) override;
  void addSpectrum(const std::string &wsName);
  void setupAnalysisPane(MantidWidgets::PlotFitAnalysisPaneView *analysis);

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
