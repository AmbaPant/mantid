#include "IndirectFitAnalysisTab.h"
#include "ui_ConvFit.h"
#include "ui_IqtFit.h"
#include "ui_JumpFit.h"
#include "ui_MSDFit.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidQtWidgets/Common/PropertyHandler.h"

#include <QString>

#include <algorithm>

using namespace Mantid::API;

namespace {

/**
 * Checks whether the specified algorithm has a property with the specified
 * name. If true, sets this property to the specified value, else returns.
 *
 * @param algorithm     The algorithm whose property to set.
 * @param propertyName  The name of the property to set.
 * @param value         The value to set.
 */
template <typename T>
void setAlgorithmProperty(IAlgorithm_sptr algorithm,
                          const std::string &propertyName, const T &value) {
  if (algorithm->existsProperty(propertyName))
    algorithm->setProperty(propertyName, value);
}

/**
 * Combines the two maps of parameter values, by adding the values from
 * the second into the first, where the parameters (keys) are taken from
 * the first map, and the value doesn't already exist in the first map.
 *
 * @param parameterValues1  The first parameter values map to combine with.
 * @param parameterValues2  The second parameter values map to combine with.
 * @return                  The combined map.
 */
template <typename Map>
Map combineParameterValues(const Map &parameterValues1,
                           const Map &parameterValues2) {
  auto combinedValues = parameterValues1;

  for (const auto &index : parameterValues1.keys()) {
    if (parameterValues2.contains(index)) {
      const auto &values1 = parameterValues1[index];
      const auto &values2 = parameterValues2[index];

      for (const auto &parameterName : values2.keys()) {
        if (!values1.contains(parameterName))
          combinedValues[index][parameterName] = values2[parameterName];
      }
    }
  }

  return combinedValues;
}

/**
 * Reverts the specified changes made to the specified map of values.
 *
 * @param map     The map to apply the reversion to.
 * @param changes The list of changes (map-like structure), detailing
 *                the value before and after the change.
 */
template <typename Map, typename Changes>
void revertChanges(Map &map, const Changes &changes) {
  for (const auto &beforeChange : changes.keys()) {
    const auto &afterChange = changes[beforeChange];

    for (auto &values : map) {
      if (values.contains(afterChange)) {
        const auto value = values[afterChange];
        values.remove(afterChange);
        values[beforeChange] = value;
      }
    }
  }
}

/*
 * Sets the value of each parameter, in a clone of the specified function, to 0.
 *
 * @param function  The function to create a clone of.
 * @return          A clone of the specified function, whose parameter values
 *                  are all set to 0.
 */
IFunction_sptr zeroFunction(IFunction_const_sptr function) {
  auto functionClone = function->clone();
  for (const auto &parameter : functionClone->getParameterNames())
    functionClone->setParameter(parameter, 0.0);
  return functionClone;
}

/*
 * Checks whether the specified functions have the same composition.
 *
 * @param func1 Function to compare.
 * @param func2 Function to compare.
 * @return      True if the specified functions have the same composition,
 *              False otherwise.
 */
bool equivalentFunctions(IFunction_const_sptr func1,
                         IFunction_const_sptr func2) {
  if (!func1 || !func2)
    return false;

  return zeroFunction(func1)->asString() == zeroFunction(func2)->asString();
}

class WidgetAdder : public boost::static_visitor<> {
public:
  WidgetAdder(QWidget *widget) : m_widget(widget) {}

  template <typename Form> void operator()(Form form) const {
    form->properties->addWidget(m_widget);
  }

private:
  QWidget *m_widget;
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Constructor.
 *
 * @param parent :: the parent widget (an IndirectDataAnalysis object).
 */
IndirectFitAnalysisTab::IndirectFitAnalysisTab(QWidget *parent)
    : IndirectDataAnalysisTab(parent) {
  m_fitPropertyBrowser = new MantidWidgets::IndirectFitPropertyBrowser(parent);
  m_fitPropertyBrowser->init();

  connect(m_fitPropertyBrowser, SIGNAL(fitScheduled()), this,
          SLOT(executeSingleFit()));
  connect(m_fitPropertyBrowser, SIGNAL(sequentialFitScheduled()), this,
          SLOT(executeSequentialFit()));

  connect(m_fitPropertyBrowser,
          SIGNAL(parameterChanged(const Mantid::API::IFunction *)), this,
          SLOT(parameterUpdated(const Mantid::API::IFunction *)));
  connect(m_fitPropertyBrowser,
          SIGNAL(parameterChanged(const Mantid::API::IFunction *)), this,
          SLOT(emitParameterChanged(const Mantid::API::IFunction *)));
  connect(m_fitPropertyBrowser,
          SIGNAL(parameterChanged(const Mantid::API::IFunction *)), this,
          SLOT(plotGuess()));

  connect(m_fitPropertyBrowser, SIGNAL(xRangeChanged(double, double)), this,
          SLOT(rangeChanged(double, double)));
  connect(m_fitPropertyBrowser, SIGNAL(xRangeChanged(double, double)), this,
          SLOT(plotGuess()));

  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(updateParameterValues()));
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(emitFunctionChanged()));
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(updatePreviewPlots()));
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(updatePlotOptions()));
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(updatePlotGuess()));
}

/**
 * Adds a fit property browser to the specified Indirect Fit Analysis Tab.
 *
 * @param tab The indirect fit analysis tab to add the fit property browser to.
 */
void IndirectFitAnalysisTab::addPropertyBrowserToUI(FitTab tab) {
  boost::apply_visitor(WidgetAdder(m_fitPropertyBrowser), tab);
}

/**
 * @return  The selected background function in this indirect fit analysis tab.
 */
IFunction_sptr IndirectFitAnalysisTab::background() const {
  return m_fitPropertyBrowser->background();
}

/**
 * @return  The selected model function in this indirect fit analysis tab.
 *          The model is specified to be the complete composite function, with
 *          the background removed.
 */
IFunction_sptr IndirectFitAnalysisTab::model() const {
  auto model = m_fitPropertyBrowser->compositeFunction()->clone();
  auto compositeModel = boost::dynamic_pointer_cast<CompositeFunction>(model);

  if (compositeModel) {
    auto index = m_fitPropertyBrowser->backgroundIndex();

    if (index != -1)
      compositeModel->removeFunction(static_cast<size_t>(index));
    return compositeModel;
  }
  return model;
}

/**
 * @return  The function index of the selected background.
 */
int IndirectFitAnalysisTab::backgroundIndex() const {
  return m_fitPropertyBrowser->backgroundIndex();
}

/**
 * @return  The fit type selected in the custom functions combo box, in the fit
 *          property browser.
 */
QString IndirectFitAnalysisTab::selectedFitType() const {
  return m_fitPropertyBrowser->selectedFitType();
}

/**
 * @param functionName  The name of the function.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t IndirectFitAnalysisTab::numberOfCustomFunctions(
    const std::string &functionName) const {
  return m_fitPropertyBrowser->numberOfCustomFunctions(functionName);
}

/**
 * @return  The selected Start-X value in the indirect fit analysis tab.
 */
double IndirectFitAnalysisTab::startX() const {
  return m_fitPropertyBrowser->startX();
}

/**
 * @return  The selected End-X value in the indirect fit analysis tab.
 */
double IndirectFitAnalysisTab::endX() const {
  return m_fitPropertyBrowser->endX();
}

/**
 * @param functionName  The name of the function containing the parameter.
 * @param parameterName The name of the parameter whose value to retrieve.
 * @return              The value of the parameter with the specified name, in
 *                      the function with the specified name.
 */
double
IndirectFitAnalysisTab::parameterValue(const std::string &functionName,
                                       const std::string &parameterName) {
  return m_fitPropertyBrowser->parameterValue(functionName, parameterName);
}

/**
 * @return  True if the selected model is empty, false otherwise.
 */
bool IndirectFitAnalysisTab::emptyModel() const {
  auto modelFunction = model();
  auto compositeModel =
      boost::dynamic_pointer_cast<CompositeFunction>(modelFunction);

  if (compositeModel)
    return compositeModel->nFunctions() == 0;
  else
    return modelFunction->asString() == "";
}

/**
 * @return  The name of the selected background.
 */
QString IndirectFitAnalysisTab::backgroundName() const {
  return m_fitPropertyBrowser->backgroundName();
}

/**
 * @return  True if the current model is the same as the model which was most
 *          recently fit, false otherwise.
 */
bool IndirectFitAnalysisTab::previousFitModelSelected() const {
  return equivalentFunctions(m_fitFunction,
                             m_fitPropertyBrowser->compositeFunction());
}

/**
 * @return  True if a guess plot can be fit, false otherwise.
 */
bool IndirectFitAnalysisTab::canPlotGuess() const {
  return !emptyModel() && inputWorkspace();
}

/**
 * @return  The output workspace name used in the most recent fit.
 */
const std::string &IndirectFitAnalysisTab::outputWorkspaceName() const {
  return m_outputFitName;
}

/**
 * Moves the functions attached to a custom function group, to the end of the
 * model.
 */
void IndirectFitAnalysisTab::moveCustomFunctionsToEnd() {
  m_fitPropertyBrowser->moveCustomFunctionsToEnd();
}

/**
 * Sets the value of the parameter with the specified name, in the function with
 * the specified name.
 *
 * @param functionName  The name of the function containing the parameter.
 * @param parameterName The name of the parameter to set.
 * @param value         The value to set.
 */
void IndirectFitAnalysisTab::setParameterValue(const std::string &functionName,
                                               const std::string &parameterName,
                                               double value) {
  m_fitPropertyBrowser->setParameterValue(functionName, parameterName, value);
}

/**
 * Sets the default peak type for the indirect property browser.
 *
 * @param function  The name of the default peak function to set.
 */
void IndirectFitAnalysisTab::setDefaultPeakType(const std::string &function) {
  m_fitPropertyBrowser->setDefaultPeakType(function);
}

/**
 * Adds a check-box with the specified name, to the fit property browser, which
 * when checked adds the specified functions to the mode and when unchecked,
 * removes them.
 *
 * @param groupName     The name/label of the check-box to add.
 * @param functions     The functions to be added when the check-box is checked.
 * @param defaultValue  The default value of the check-box.
 */
void IndirectFitAnalysisTab::addCheckBoxFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions,
    bool defaultValue) {
  m_fitPropertyBrowser->addCheckBoxFunctionGroup(groupName, functions,
                                                 defaultValue);
}

/**
 * Adds a number spinner with the specified name, to the fit property browser,
 * which specifies how many multiples of the specified functions should be added
 * to the model.
 *
 * @param groupName     The name/label of the spinner to add.
 * @param functions     The functions to be added.
 * @param minimum       The minimum value of the spinner.
 * @param maximum       The maximum value of the spinner.
 * @param defaultValue  The default value of the spinner.
 */
void IndirectFitAnalysisTab::addSpinnerFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions,
    int minimum, int maximum, int defaultValue) {
  m_fitPropertyBrowser->addSpinnerFunctionGroup(groupName, functions, minimum,
                                                maximum, defaultValue);
}

/**
 * Adds an option with the specified name, to the fit type combo-box in the fit
 * property browser, which adds the specified functions to the model.
 *
 * @param groupName The name of the option to be added to the fit type
 *                  combo-box.
 * @param functions The functions added by the option.
 */
void IndirectFitAnalysisTab::addComboBoxFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions) {
  m_fitPropertyBrowser->addComboBoxFunctionGroup(groupName, functions);
}

/**
 * Sets the available background options in this fit analysis tab.
 *
 * @param backgrounds A list of the available backgrounds.
 */
void IndirectFitAnalysisTab::setBackgroundOptions(
    const QStringList &backgrounds) {
  m_fitPropertyBrowser->setBackgroundOptions(backgrounds);
}

/**
 * @param settingKey  The key of the boolean setting whose value to retrieve.
 * @return            The value of the boolean setting with the specified key.
 */
bool IndirectFitAnalysisTab::boolSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->boolSettingValue(settingKey);
}

/**
 * @param settingKey  The key of the integer setting whose value to retrieve.
 * @return            The value of the integer setting with the specified key.
 */
int IndirectFitAnalysisTab::intSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->intSettingValue(settingKey);
}

/**
 * @param settingKey  The key of the double setting whose value to retrieve.
 * @return            The value of the double setting with the specified key.
 */
double
IndirectFitAnalysisTab::doubleSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->doubleSettingValue(settingKey);
}

/**
 * @param settingKey  The key of the enum setting whose value to retrieve.
 * @return            The value of the enum setting with the specified key.
 */
QString
IndirectFitAnalysisTab::enumSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->enumSettingValue(settingKey);
}

/**
 * Adds a boolean custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the boolean setting to add.
 * @param settingName   The display name of the boolean setting to add.
 * @param defaultValue  The default value of the boolean setting.
 */
void IndirectFitAnalysisTab::addBoolCustomSetting(const QString &settingKey,
                                                  const QString &settingName,
                                                  bool defaultValue) {
  m_fitPropertyBrowser->addBoolCustomSetting(settingKey, settingName,
                                             defaultValue);
}

/**
 * Adds a double custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the double setting to add.
 * @param settingName   The display name of the double setting to add.
 * @param defaultValue  The default value of the double setting.
 */
void IndirectFitAnalysisTab::addDoubleCustomSetting(const QString &settingKey,
                                                    const QString &settingName,
                                                    double defaultValue) {
  m_fitPropertyBrowser->addDoubleCustomSetting(settingKey, settingName,
                                               defaultValue);
}

/**
 * Adds an integer custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the integer setting to add.
 * @param settingName   The display name of the integer setting to add.
 * @param defaultValue  The default value of the integer setting.
 */
void IndirectFitAnalysisTab::addIntCustomSetting(const QString &settingKey,
                                                 const QString &settingName,
                                                 int defaultValue) {
  m_fitPropertyBrowser->addIntCustomSetting(settingKey, settingName,
                                            defaultValue);
}

/**
 * Adds an enum custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the enum setting to add.
 * @param settingName   The display name of the enum setting to add.
 * @param defaultValue  The default value of the enum setting.
 */
void IndirectFitAnalysisTab::addEnumCustomSetting(const QString &settingKey,
                                                  const QString &settingName,
                                                  const QStringList &options) {
  m_fitPropertyBrowser->addEnumCustomSetting(settingKey, settingName, options);
}

/**
 * Adds an optional double custom setting, with the specified key and display
 * name.
 *
 * @param settingKey    The key of the optional double setting to add.
 * @param settingName   The display name of the optional double setting to add.
 * @param optionKey     The key of the setting specifying whether to use this
 *                      this optional setting.
 * @param optionName    The display name of the setting specifying whether to
 *                      use this optional setting.
 * @param defaultValue  The default value of the optional double setting.
 */
void IndirectFitAnalysisTab::addOptionalDoubleSetting(
    const QString &settingKey, const QString &settingName,
    const QString &optionKey, const QString &optionName, bool enabled,
    double defaultValue) {
  m_fitPropertyBrowser->addOptionalDoubleSetting(
      settingKey, settingName, optionKey, optionName, enabled, defaultValue);
}

/**
 * Sets the selected spectrum for this indirect fit analysis tab.
 */
void IndirectFitAnalysisTab::setSelectedSpectrum(int spectrum) {
  disablePlotGuess();
  m_fitPropertyBrowser->setWorkspaceIndex(spectrum);
  IndirectDataAnalysisTab::setSelectedSpectrum(spectrum);
  updateParameterValues();
  updatePreviewPlots();
  updatePlotGuess();
}

/**
 * @return  The default parameter values to be used in this indirect fit
 *          analysis tab.
 */
QHash<QString, double> IndirectFitAnalysisTab::createDefaultValues() const {
  return QHash<QString, double>();
}

/**
 * @return  The parameter values found in the most recent fit.
 */
QHash<QString, double> IndirectFitAnalysisTab::fitParameterValues() const {
  const auto spectrum = selectedSpectrum();
  if (m_parameterValues.contains(spectrum))
    return m_parameterValues[spectrum];
  else
    return QHash<QString, double>();
}

/**
 * @return  The default parameter values as applied to the model.
 */
QHash<QString, double> IndirectFitAnalysisTab::defaultParameterValues() const {
  if (emptyModel())
    return QHash<QString, double>();

  QHash<QString, double> defaultValues;

  const auto function = m_fitPropertyBrowser->getFittingFunction();

  for (const auto &shortParamName : m_defaultPropertyValues.keys()) {
    const auto &value = m_defaultPropertyValues[shortParamName];

    for (const auto &parameter : function->getParameterNames()) {
      const auto parameterName = QString::fromStdString(parameter);

      if (parameterName.endsWith(shortParamName))
        defaultValues[parameterName] = value;
    }
  }
  return defaultValues;
}

/**
 * @return  The values of the parameters in the selected model.
 */
QHash<QString, double> IndirectFitAnalysisTab::parameterValues() const {
  auto values = defaultParameterValues();
  const auto fitValues = fitParameterValues();

  for (const auto &parameter : fitValues.keys())
    values[parameter] = fitValues[parameter];

  return values;
}

/*
 * Sets the default value for the property with the specified name,
 * in the property table of this fit analysis tab.
 *
 * @param propertyName  The name of the property whose default to set.
 * @param propertyValue The default value to set.
 */
void IndirectFitAnalysisTab::setDefaultPropertyValue(
    const QString &propertyName, const double &propertyValue) {
  m_defaultPropertyValues[propertyName] = propertyValue;
}

/*
 * Removes the default value for the property with the specified name,
 * in the property table of this fit analysis tab.
 *
 * @param propertyName  The name of the property whose default to remove.
 */
void IndirectFitAnalysisTab::removeDefaultPropertyValue(
    const QString &propertyName) {
  m_defaultPropertyValues.remove(propertyName);
}

/*
 * Checks whether the property with the specified name has a default
 * property value.
 *
 * @param propertyName  The name of the property to check for the default of.
 * @return              True if the property with the specified name has a
 *                      default value, false otherwise.
 */
bool IndirectFitAnalysisTab::hasDefaultPropertyValue(
    const QString &propertyName) {
  return m_defaultPropertyValues.contains(propertyName);
}

/**
 * @return  The names of the parameters in the selected model.
 */
QSet<QString> IndirectFitAnalysisTab::parameterNames() {
  QSet<QString> parameterNames;
  auto function = m_fitPropertyBrowser->getFittingFunction();

  for (size_t i = 0u; i < function->nParams(); ++i) {
    const auto &parameter = QString::fromStdString(function->parameterName(i));

    if (m_functionNameChanges.contains(parameter))
      parameterNames.insert(m_functionNameChanges[parameter]);
    else
      parameterNames.insert(parameter);
  }

  return parameterNames;
}

/*
 * Performs necessary state changes when the fit algorithm was run
 * and completed within this interface.
 *
 * @param paramWSName          The name of the workspace containing the fit
 *                             parameter values.
 * @param propertyToParameter  Pre-existing property to parameter map to unite.
 */
void IndirectFitAnalysisTab::fitAlgorithmComplete(
    const std::string &paramWSName) {

  if (AnalysisDataService::Instance().doesExist(paramWSName))
    updateParametersFromTable(paramWSName);

  updatePreviewPlots();

  connect(m_fitPropertyBrowser, SIGNAL(parameterChanged(const IFunction *)),
          this, SLOT(plotGuess()));
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(clearBatchRunnerSlots()));
}

/**
 * Updates the values of the parameters in the model, from the table workspace
 * with the specified name.
 *
 * @param paramWSName The name of the table containing the updated parameter
 *                    values.
 */
void IndirectFitAnalysisTab::updateParametersFromTable(
    const std::string &paramWSName) {
  const auto parameters = parameterNames();
  auto parameterValues = IndirectTab::extractParametersFromTable(
      paramWSName, parameters, minimumSpectrum(), maximumSpectrum());
  revertChanges(parameterValues, m_functionNameChanges);

  if (m_appendResults)
    m_parameterValues =
        combineParameterValues(parameterValues, m_parameterValues);
  else
    m_parameterValues = parameterValues;

  updateParameterValues();
}

/**
 * Handles the event in which the minimum-X value has been selected.
 *
 * @param xMax  The selected minimum-X value.
 */
void IndirectFitAnalysisTab::xMinSelected(double xMin) {
  m_fitPropertyBrowser->setStartX(xMin);
}

/**
 * Handles the event in which the maximum-X value has been selected.
 *
 * @param xMax  The selected maximum-X value.
 */
void IndirectFitAnalysisTab::xMaxSelected(double xMax) {
  m_fitPropertyBrowser->setEndX(xMax);
}

/*
 * Performs necessary state changes when new input data is loaded in
 * this fit analysis tab.
 * - Sets preview plot and input workspaces.
 * - Updates default property values.
 * - Updates property table.
 * - Updates preview plots.
 *
 * @param wsName  The name of the loaded input workspace.
 */
void IndirectFitAnalysisTab::newInputDataLoaded(const QString &wsName) {
  auto inputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());
  m_fitPropertyBrowser->setWorkspaceName(wsName);
  setInputWorkspace(inputWs);
  m_defaultPropertyValues = createDefaultValues();
  m_fitPropertyBrowser->updateParameterValues(defaultParameterValues());
  setPreviewPlotWorkspace(inputWs);
  m_parameterValues.clear();
  m_fitFunction.reset();
  m_outputFitName = "";
  blockSignals(true);
  updatePreviewPlots();
  blockSignals(false);
}

/*
 * Clears all slots connected to the batch runners signals.
 */
void IndirectFitAnalysisTab::clearBatchRunnerSlots() {
  m_batchAlgoRunner->disconnect();
}

/**
 * Updates the parameter values in the fit property browser.
 */
void IndirectFitAnalysisTab::updateParameterValues() {
  const auto spectrum = static_cast<size_t>(selectedSpectrum());

  if (m_parameterValues.contains(spectrum))
    if (previousFitModelSelected())
      m_fitPropertyBrowser->updateParameterValues(m_parameterValues[spectrum]);
    else
      m_fitPropertyBrowser->updateParameterValues(parameterValues());
  else
    m_fitPropertyBrowser->updateParameterValues(defaultParameterValues());
}

/*
 * Saves the result workspace with the specified name, in the default
 * save directory.
 *
 * @param resultName  The name of the workspace to save.
 */
void IndirectFitAnalysisTab::saveResult(const std::string &resultName) {
  // check workspace exists
  const auto wsFound = checkADSForPlotSaveWorkspace(resultName, false);
  // process workspace after checkf
  if (wsFound) {
    QString saveDir = QString::fromStdString(
        Mantid::Kernel::ConfigService::Instance().getString(
            "defaultsave.directory"));
    // Check validity of save path
    QString QresultWsName = QString::fromStdString(resultName);
    const auto fullPath = saveDir.append(QresultWsName).append(".nxs");
    addSaveWorkspaceToQueue(QresultWsName, fullPath);
    m_batchAlgoRunner->executeBatchAsync();
  }
}

/*
 * Plots the result workspace with the specified name, using the specified
 * plot type. Plot type can either be 'None', 'All' or the name of a
 * parameter. In the case of 'None', nothing will be plotted. In the case of
 * 'All', everything will be plotted. In the case of a parameter name, only
 * the spectra created from that parameter will be plotted.
 *
 * @param resultName  The name of the workspace to plot.
 * @param plotType    The plot type specifying what to plot.
 */
void IndirectFitAnalysisTab::plotResult(const std::string &resultName,
                                        const QString &plotType) {
  const auto wsFound = checkADSForPlotSaveWorkspace(resultName, true);
  if (wsFound) {
    MatrixWorkspace_sptr resultWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(resultName);
    QString resultWsQName = QString::fromStdString(resultName);

    // Handle plot result
    if (plotType.compare("All") == 0) {
      const auto specEnd = (int)resultWs->getNumberHistograms();
      for (int i = 0; i < specEnd; ++i)
        IndirectTab::plotSpectrum(resultWsQName, i);
    } else {
      QHash<QString, size_t> labels =
          IndirectTab::extractAxisLabels(resultWs, 1);

      for (const auto &parameter : m_fitPropertyBrowser->getParameterNames()) {
        if (parameter.contains(plotType)) {
          if (labels.contains(parameter))
            IndirectTab::plotSpectrum(resultWsQName, (int)labels[parameter]);
        }
      }
    }
  }
}

/*
 * Fills the specified combo-box, with the possible parameters which
 * can be plot separately.
 *
 * @param comboBox  The combo box to fill.
 */
void IndirectFitAnalysisTab::fillPlotTypeComboBox(QComboBox *comboBox) {
  comboBox->clear();
  comboBox->addItem("All");

  QSet<QString> parameters;
  for (const auto &parameter : m_fitPropertyBrowser->getParameterNames())
    parameters.insert(parameter.right(parameter.lastIndexOf('.')));
  comboBox->addItems(parameters.toList());
}

/*
 * Updates the preview plots in this fit analysis tab, given the name
 * of the output workspace from a fit.
 *
 * @param workspaceName   The name of the workspace to plot.
 * @param fitPreviewPlot  The preview plot widget in which to plot the fit.
 * @param diffPreviewPlot The preview plot widget in which to plot the
 *                        difference between the fit and sample data.
 */
void IndirectFitAnalysisTab::updatePlot(
    const std::string &workspaceName,
    MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {

  if (previousFitModelSelected())
    IndirectDataAnalysisTab::updatePlot(workspaceName, fitPreviewPlot,
                                        diffPreviewPlot);
  else
    IndirectDataAnalysisTab::updatePlot("", fitPreviewPlot, diffPreviewPlot);
}

/**
 * @return  The output workspace name to use for a sequential fit.
 */
std::string IndirectFitAnalysisTab::createSequentialFitOutputName() const {
  return createSingleFitOutputName();
}

/**
 * @return The current single fit algorithm for this indirect fit analysis tab.
 */
IAlgorithm_sptr IndirectFitAnalysisTab::singleFitAlgorithm() const {
  auto algorithm = AlgorithmManager::Instance().create("Fit");
  algorithm->setProperty("WorkspaceIndex",
                         m_fitPropertyBrowser->workspaceIndex());
  return algorithm;
}

/**
 * @return The current sequential fit algorithm for this indirect fit analysis
 *         tab.
 */
IAlgorithm_sptr IndirectFitAnalysisTab::sequentialFitAlgorithm() const {
  return singleFitAlgorithm();
}

/**
 * Executes the single fit algorithm defined in this indirect fit analysis tab.
 */
void IndirectFitAnalysisTab::executeSingleFit() {
  m_outputFitName = createSingleFitOutputName();
  runFitAlgorithm(singleFitAlgorithm());
}

/**
 * Executes the sequential fit algorithm defined in this indirect fit analysis
 * tab.
 */
void IndirectFitAnalysisTab::executeSequentialFit() {
  m_outputFitName = createSequentialFitOutputName();
  runFitAlgorithm(sequentialFitAlgorithm());
}

/**
 * @return  The fit function defined in this indirect fit analysis tab.
 */
IFunction_sptr IndirectFitAnalysisTab::fitFunction() const {
  if (!emptyModel())
    return m_fitPropertyBrowser->getFittingFunction();
  else
    return nullptr;
}

/**
 * @param function  The function in the fit property browser.
 * @return          A map from the name of a function in the fit property
 *                  browser, to the name of a function in the selected model.
 */
QHash<QString, QString>
IndirectFitAnalysisTab::functionNameChanges(IFunction_sptr function) const {
  return QHash<QString, QString>();
}

/**
 * @return  The workspace containing the data to be fit.
 */
MatrixWorkspace_sptr IndirectFitAnalysisTab::fitWorkspace() const {
  return boost::dynamic_pointer_cast<MatrixWorkspace>(
      m_fitPropertyBrowser->getWorkspace());
}

/**
 * Sets the MaxIterations property of the specified algorithm, to the specified
 * integer value.
 *
 * @param fitAlgorithm  The fit algorithm whose MaxIterations property to set.
 * @param maxIterations The value to set.
 */
void IndirectFitAnalysisTab::setMaxIterations(IAlgorithm_sptr fitAlgorithm,
                                              int maxIterations) const {
  setAlgorithmProperty(fitAlgorithm, "MaxIterations", maxIterations);
}

/*
 * Runs the specified fit algorithm and calls the algorithmComplete
 * method of this fit analysis tab once completed.
 *
 * @param fitAlgorithm      The fit algorithm to run.
 */
void IndirectFitAnalysisTab::runFitAlgorithm(IAlgorithm_sptr fitAlgorithm) {
  disconnect(m_fitPropertyBrowser, SIGNAL(parameterChanged(const IFunction *)),
             this, SLOT(plotGuess()));

  fitAlgorithm->setProperty("InputWorkspace", fitWorkspace());

  setAlgorithmProperty(fitAlgorithm, "Function", fitFunction()->asString());
  setAlgorithmProperty(fitAlgorithm, "StartX", m_fitPropertyBrowser->startX());
  setAlgorithmProperty(fitAlgorithm, "EndX", m_fitPropertyBrowser->endX());
  setAlgorithmProperty(fitAlgorithm, "Minimizer",
                       m_fitPropertyBrowser->minimizer(true));
  setMaxIterations(fitAlgorithm, m_fitPropertyBrowser->maxIterations());
  setAlgorithmProperty(fitAlgorithm, "Convolve",
                       m_fitPropertyBrowser->convolveMembers());
  setAlgorithmProperty(fitAlgorithm, "PeakRadius",
                       m_fitPropertyBrowser->getPeakRadius());

  m_functionNameChanges = functionNameChanges(model());
  m_fitFunction = m_fitPropertyBrowser->getFittingFunction()->clone();
  m_batchAlgoRunner->addAlgorithm(fitAlgorithm);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(clearBatchRunnerSlots()));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Updates the specified combo box, with the available plot options.
 *
 * @param cbPlotType  The combo box.
 */
void IndirectFitAnalysisTab::updatePlotOptions(QComboBox *cbPlotType) {
  cbPlotType->clear();
  auto parameters = model()->getParameterNames();

  QSet<QString> plotOptions;

  for (const auto parameter : parameters) {
    auto plotOption = QString::fromStdString(parameter);
    auto index = plotOption.lastIndexOf(".");
    if (index >= 0)
      plotOption = plotOption.remove(0, index + 1);
    plotOptions << plotOption;
  }

  QStringList plotList;
  if (!parameters.empty())
    plotList << "All";
  plotList.append(plotOptions.toList());

  cbPlotType->addItems(plotList);
}

/*
 * Plots a guess of the fit for the specified function, in the
 * specified preview plot widget.
 *
 * @param previewPlot The preview plot widget in which to plot
 *                    the guess.
 */
void IndirectFitAnalysisTab::plotGuess(
    MantidQt::MantidWidgets::PreviewPlot *previewPlot) {
  auto guessFunction = fitFunction();

  if (inputWorkspace() && guessFunction) {
    auto guessWorkspace =
        createGuessWorkspace(guessFunction, selectedSpectrum());

    // Check whether the guess workspace has enough data points
    // to plot
    if (guessWorkspace->x(0).size() >= 2)
      previewPlot->addSpectrum("Guess", guessWorkspace, 0, Qt::green);
  }
}

/**
 * Enables or disables the plot guess feature in this indirect fit analysiss
 * tab, depending on whether the selected model is empty and whether an input
 * workspace has been set.
 */
void IndirectFitAnalysisTab::updatePlotGuess() {
  if (canPlotGuess())
    enablePlotGuess();
  else
    disablePlotGuess();
  plotGuess();
}

/*
 * Creates a guess workspace, for approximating a fit with the specified
 * function on the input workspace.
 *
 * @param func    The function to fit.
 * @param wsIndex The index of the input workspace to create a guess for.
 * @return        A guess workspace containing the guess data for the fit.
 */
MatrixWorkspace_sptr
IndirectFitAnalysisTab::createGuessWorkspace(IFunction_const_sptr func,
                                             size_t wsIndex) {
  const auto inputWS = inputWorkspace();
  const auto binIndexLow = inputWS->binIndexOf(startX());
  const auto binIndexHigh = inputWS->binIndexOf(endX());
  const auto nData = binIndexHigh - binIndexLow;

  const auto &xPoints = inputWS->points(wsIndex);
  std::vector<double> dataX(nData);
  std::copy(&xPoints[binIndexLow], &xPoints[binIndexLow + nData],
            dataX.begin());
  const auto dataY = computeOutput(func, dataX);

  if (dataY.empty())
    return WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

  IAlgorithm_sptr createWsAlg =
      createWorkspaceAlgorithm("__GuessAnon", 1, dataX, dataY);
  createWsAlg->execute();
  return createWsAlg->getProperty("OutputWorkspace");
}

/*
 * Computes the output vector of applying the specified function to
 * the specified input vector.
 *
 * @param func    The function to apply.
 * @param dataX   Vector of input data.
 * @return        Vector containing values calculated from applying
 *                the specified function to the input data.
 */
std::vector<double>
IndirectFitAnalysisTab::computeOutput(IFunction_const_sptr func,
                                      const std::vector<double> &dataX) {
  if (dataX.empty())
    return std::vector<double>();

  FunctionDomain1DVector domain(dataX);
  FunctionValues outputData(domain);
  func->function(domain, outputData);

  std::vector<double> dataY(dataX.size());
  for (size_t i = 0; i < dataY.size(); i++) {
    dataY[i] = outputData.getCalculated(i);
  }
  return dataY;
}

/*
 * Generates and returns an algorithm for creating a workspace, with
 * the specified name, number of spectra and containing the specified
 * x data and y data.
 *
 * @param workspaceName The name of the workspace to create.
 * @param numSpec       The number of spectra in the workspace to create.
 * @param dataX         The x data to add to the created workspace.
 * @param dataY         The y data to add to the created workspace.
 * @return              An algorithm for creating the workspace.
 */
IAlgorithm_sptr IndirectFitAnalysisTab::createWorkspaceAlgorithm(
    const std::string &workspaceName, int numSpec,
    const std::vector<double> &dataX, const std::vector<double> &dataY) {
  IAlgorithm_sptr createWsAlg =
      AlgorithmManager::Instance().create("CreateWorkspace");
  createWsAlg->initialize();
  createWsAlg->setChild(true);
  createWsAlg->setLogging(false);
  createWsAlg->setProperty("OutputWorkspace", workspaceName);
  createWsAlg->setProperty("NSpec", numSpec);
  createWsAlg->setProperty("DataX", dataX);
  createWsAlg->setProperty("DataY", dataY);
  return createWsAlg;
}

void IndirectFitAnalysisTab::emitFunctionChanged() { emit functionChanged(); }

void IndirectFitAnalysisTab::emitParameterChanged(
    const Mantid::API::IFunction *function) {
  emit parameterChanged(function);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
