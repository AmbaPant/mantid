// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_

#include "IndirectDataAnalysisTab.h"
#include "IndirectFitDataPresenter.h"
#include "IndirectFitOutputOptionsPresenter.h"
#include "IndirectFitOutputOptionsView.h"
#include "IndirectFitPlotPresenter.h"
#include "IndirectFittingModel.h"
#include "IndirectSpectrumSelectionPresenter.h"
#include "IndirectSpectrumSelectionView.h"

#include "MantidQtWidgets/Common/IndirectFitPropertyBrowser.h"

#include <boost/optional.hpp>

#include <QtCore>

#include <memory>
#include <type_traits>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitAnalysisTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectFitAnalysisTab(IndirectFittingModel *model,
                         QWidget *parent = nullptr);

  void setFitDataPresenter(std::unique_ptr<IndirectFitDataPresenter> presenter);
  void setPlotView(IIndirectFitPlotView *view);
  void setSpectrumSelectionView(IndirectSpectrumSelectionView *view);
  void setOutputOptionsView(IIndirectFitOutputOptionsView *view);
  void
  setFitPropertyBrowser(MantidWidgets::IndirectFitPropertyBrowser *browser);

  std::size_t getSelectedDataIndex() const;
  std::size_t getSelectedSpectrum() const;
  bool isRangeCurrentlySelected(std::size_t dataIndex,
                                std::size_t spectrum) const;

  QString selectedFitType() const;

  size_t numberOfCustomFunctions(const std::string &functionName) const;

  void setConvolveMembers(bool convolveMembers);

protected:
  IndirectFittingModel *fittingModel() const;

  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);

  void run() override;

  void setAlgorithmProperties(Mantid::API::IAlgorithm_sptr fitAlgorithm) const;
  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  void runSingleFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  virtual void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  virtual void setRunIsRunning(bool running) = 0;
  virtual void setRunEnabled(bool enable) = 0;

signals:
  void functionChanged();
  void parameterChanged(const Mantid::API::IFunction *);
  void customBoolChanged(const QString &key, bool value);

protected slots:

  void setModelFitFunction();
  void setModelStartX(double startX);
  void setModelEndX(double startX);
  void setDataTableStartX(double startX);
  void setDataTableEndX(double endX);
  void setDataTableExclude(const std::string &exclude);
  void setBrowserWorkspaceIndex(std::size_t spectrum);
  void tableStartXChanged(double startX, std::size_t dataIndex,
                          std::size_t spectrum);
  void tableEndXChanged(double endX, std::size_t dataIndex,
                        std::size_t spectrum);
  void tableExcludeChanged(const std::string &exclude, std::size_t dataIndex,
                           std::size_t spectrum);

  void updateFitOutput(bool error);
  void updateSingleFitOutput(bool error);
  void fitAlgorithmComplete(bool error);

  void singleFit();
  void singleFit(std::size_t dataIndex, std::size_t spectrum);
  void executeFit();

  std::unordered_map<std::string, Mantid::API::IFunction::Attribute>
  getAttributes(Mantid::API::IFunction_sptr const &function,
                std::vector<std::string> const &attributeNames);
  void updateParameterValues();
  void updateParameterValues(
      const std::unordered_map<std::string, ParameterValue> &parameters);
  void updateFitBrowserParameterValues();

  void updateDataReferences();
  void updateResultOptions();

private slots:
  void updatePlotGuess();
  void plotSelectedSpectra();
  void editLocalParameterValues(const QString &parName);

private:
  /// Overidden by child class.
  void setup() override;
  void loadSettings(const QSettings &settings) override;
  virtual void setupFitTab() = 0;
  bool validate() override;

  void connectDataAndPlotPresenters();
  void connectSpectrumAndPlotPresenters();
  void connectFitBrowserAndPlotPresenter();
  void connectDataAndSpectrumPresenters();
  void connectDataAndFitBrowserPresenters();

  void plotSelectedSpectra(std::vector<SpectrumToPlot> const &spectra);
  void plotSpectrum(std::string const &workspaceName, std::size_t const &index,
                    bool errorBars);

  std::string getOutputBasename() const;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const;
  std::vector<std::string> getFitParameterNames() const;

  void enableFitButtons(bool enable);
  void enableOutputOptions(bool enable);
  void setPDFWorkspace(std::string const &workspaceName);

  std::unique_ptr<IndirectFittingModel> m_fittingModel;
  MantidWidgets::IndirectFitPropertyBrowser *m_fitPropertyBrowser;
  std::unique_ptr<IndirectFitDataPresenter> m_dataPresenter;
  std::unique_ptr<IndirectFitPlotPresenter> m_plotPresenter;
  std::unique_ptr<IndirectSpectrumSelectionPresenter> m_spectrumPresenter;
  std::unique_ptr<IndirectFitOutputOptionsPresenter> m_outOptionsPresenter;

  Mantid::API::IAlgorithm_sptr m_fittingAlgorithm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_ */
