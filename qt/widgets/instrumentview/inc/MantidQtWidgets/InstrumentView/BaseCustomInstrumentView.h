// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/MWRunFiles.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

#include <QObject>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW BaseCustomInstrumentView
    : public QSplitter {
  Q_OBJECT

public:
  explicit BaseCustomInstrumentView(const std::string &instrument,
                                    QWidget *parent = nullptr);
  std::string getFile();
  void setRunQuietly(const std::string &runNumber);
  void observeLoadRun(Observer *listener) {
    m_loadRunObservable->attach(listener);
  };
  void warningBox(const std::string &message);
  void setInstrumentWidget(MantidWidgets::InstrumentWidget *instrument) {
    m_instrumentWidget = instrument;
  };
  MantidWidgets::InstrumentWidget *getInstrumentView() {
    return m_instrumentWidget;
  };
  virtual void
  setUpInstrument(const std::string &fileName,
                  std::vector<std::function<bool(std::map<std::string, bool>)>>
                      &instrument);
  virtual void addObserver(std::tuple<std::string, Observer *> &listener) {
    (void)listener;
  };
  void setupInstrumentAnalysisSplitters(QWidget *analysis);
  void setupHelp();
public slots:
  void fileLoaded();
  void openHelp();

protected:
  std::string m_helpPage;

private:
  QWidget *generateLoadWidget();
  void warningBox(const QString &message);
  Observable *m_loadRunObservable;
  API::MWRunFiles *m_files;
  QString m_instrument;
  MantidWidgets::InstrumentWidget *m_instrumentWidget;
  QPushButton *m_help;
};
} // namespace MantidWidgets
} // namespace MantidQt
