#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_IqtFit.h"

#include <boost/weak_ptr.hpp>

namespace Mantid {
namespace API {
class IFunction;
class CompositeFunction;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IqtFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  IqtFit(QWidget *parent = nullptr);

  Mantid::API::MatrixWorkspace_sptr fitWorkspace() const override;

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected:
  QHash<QString, double> createDefaultValues() const override;
  std::string createSingleFitOutputName() const override;
  std::string createSequentialFitOutputName() const override;
  Mantid::API::IAlgorithm_sptr singleFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  void setMaxIterations(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                        int maxIterations) const override;

protected slots:
  void newDataLoaded(const QString wsName);
  void updatePreviewPlots() override;
  void updatePlotRange() override;
  void specMinChanged(int value);
  void specMaxChanged(int value);
  void startXChanged(double startX) override;
  void endXChanged(double endX) override;
  void singleFit();
  void plotGuess() override;
  void algorithmComplete(bool error) override;
  void updatePlotOptions() override;
  void plotWorkspace();
  void saveResult();
  void fitFunctionChanged();

private:
  void disablePlotGuess() override;
  void enablePlotGuess() override;

  std::string constructBaseName() const;
  std::string fitTypeString() const;
  Mantid::API::IAlgorithm_sptr
  iqtFitAlgorithm(Mantid::API::MatrixWorkspace_sptr inputWs,
                  const size_t &specMin, const size_t &specMax) const;
  Mantid::API::IAlgorithm_sptr
  replaceInfinityAndNaN(Mantid::API::MatrixWorkspace_sptr inputWS) const;

  std::unique_ptr<Ui::IqtFit> m_uiForm;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_ */
