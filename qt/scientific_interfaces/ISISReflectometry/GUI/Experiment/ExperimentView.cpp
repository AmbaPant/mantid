#include "ExperimentView.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include <QMessageBox>
#include <QScrollBar>
#include <boost/algorithm/string/join.hpp>

namespace MantidQt {
namespace CustomInterfaces {

namespace {
// TODO: these don't seem to work for spin boxes - is there another way to set
// the backround colour?gg
void showAsInvalid(QDoubleSpinBox &spinBox) {
  /// TODO This works but also changes the font
  spinBox.setStyleSheet("QDoubleSpinBox { background-color: #ffb8ad; }");

  /// This doesn't work
  //auto palette = spinBox.palette();
  //palette.setColor(spinBox.backgroundRole(), QColor("#ffb8ad"));
  // spinBox.setAutoFillBackground(true);
  //parent->setPalette(palette);
}

void showAsValid(QDoubleSpinBox &spinBox) {
  spinBox.setStyleSheet("");
  // auto palette = spinBox.palette();
  // palette.setColor(spinBox.foregroundRole(), Qt::transparent);
  // spinBox.setAutoFillBackground(false);
  // spinBox.setPalette(palette);
}
} // namespace

/** Constructor
 * @param algorithmForTooltips :: [input] An algorithm that will be
 * used to find tooltips for the input properties
 * @param parent :: [input] The parent of this widget
 */
ExperimentView::ExperimentView(
    Mantid::API::IAlgorithm_sptr algorithmForTooltips, QWidget *parent) {
  UNUSED_ARG(parent);
  initLayout();
  registerSettingsWidgets(algorithmForTooltips);
}

void ExperimentView::onRemovePerThetaDefaultsRequested() {
  auto index = m_ui.optionsTable->currentIndex();
  if (index.isValid()) {
    m_notifyee->notifyRemovePerAngleDefaultsRequested(index.row());
  }
}

void ExperimentView::showAllPerAngleOptionsAsValid() {
  for (auto row = 0; row < m_ui.optionsTable->rowCount(); ++row)
    showPerAngleOptionsAsValid(row);
}

void ExperimentView::showPerAngleThetasNonUnique(double tolerance) {
  QMessageBox::critical(
      this, "Invalid theta combination!",
      "Cannot have multiple defaults with theta values less than " +
          QString::number(tolerance) + " apart.");
}

void ExperimentView::showStitchParametersValid() {
  auto palette = stitchOptionsLineEdit().palette();
  palette.setColor(QPalette::Base, Qt::transparent);
  stitchOptionsLineEdit().setPalette(palette);
}

void ExperimentView::showStitchParametersInvalid() {
  auto palette = stitchOptionsLineEdit().palette();
  palette.setColor(QPalette::Base, QColor("#ffb8ad"));
  stitchOptionsLineEdit().setPalette(palette);
}

void ExperimentView::subscribe(ExperimentViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

/**
Initialise the Interface
*/
void ExperimentView::initLayout() {
  m_ui.setupUi(this);
  m_deleteShortcut = Mantid::Kernel::make_unique<QShortcut>(
      QKeySequence(tr("Delete")), m_ui.optionsTable);
  connect(m_deleteShortcut.get(), SIGNAL(activated()), this,
          SLOT(onRemovePerThetaDefaultsRequested()));
  initOptionsTable();
  auto blacklist =
      std::vector<std::string>({"InputWorkspaces", "OutputWorkspace"});
  MantidWidgets::AlgorithmHintStrategy strategy("Stitch1DMany", blacklist);
  createStitchHints(strategy.createHints());

  m_ui.startOverlapEdit->setSpecialValueText("Unset");
  m_ui.endOverlapEdit->setSpecialValueText("Unset");

  // connect(m_ui.getExpDefaultsButton, SIGNAL(clicked()), this,
  //         SLOT(requestExpDefaults()));
  connect(m_ui.summationTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(summationTypeChanged(int)));
  connect(m_ui.addPerAngleOptionsButton, SIGNAL(clicked()), this,
          SLOT(onNewPerThetaDefaultsRowRequested()));
}

void ExperimentView::initializeTableItems(QTableWidget &table) {
  for (auto row = 0; row < table.rowCount(); ++row)
    initializeTableRow(table, row);
}

void ExperimentView::initializeTableRow(QTableWidget &table, int row) {
  m_ui.optionsTable->blockSignals(true);
  for (auto column = 0; column < table.columnCount(); ++column)
    table.setItem(row, column, new QTableWidgetItem());
  m_ui.optionsTable->blockSignals(false);
}

void ExperimentView::initOptionsTable() {
  auto table = m_ui.optionsTable;

  // Set angle and scale columns to a small width so everything fits
  table->resizeColumnsToContents();
  table->setColumnCount(8);
  table->setRowCount(1);
  initializeTableItems(*table);

  auto header = table->horizontalHeader();
  int totalRowHeight = 0;
  for (auto i = 0; i < table->rowCount(); ++i) {
    totalRowHeight += table->rowHeight(i);
  }

  const int padding = 2;
  table->setMinimumHeight(totalRowHeight + header->height() + padding);
}

void ExperimentView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void ExperimentView::connectSettingsChange(QDoubleSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void ExperimentView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onSettingsChanged()));
}

void ExperimentView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this, SLOT(onSettingsChanged()));
}

void ExperimentView::onSettingsChanged() {
  m_notifyee->notifySettingsChanged();
}

void ExperimentView::connectSettingsChange(QTableWidget &edit) {
  connect(&edit, SIGNAL(cellChanged(int, int)), this,
          SLOT(onPerAngleDefaultsChanged(int, int)));
}

void ExperimentView::disableAll() { m_ui.expSettingsGrid->setEnabled(false); }

void ExperimentView::enableAll() { m_ui.expSettingsGrid->setEnabled(true); }

void ExperimentView::registerSettingsWidgets(Mantid::API::IAlgorithm_sptr alg) {
  registerExperimentSettingsWidgets(alg);
}

void ExperimentView::registerExperimentSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(*m_ui.optionsTable);
  registerSettingWidget(*m_ui.analysisModeComboBox, "AnalysisMode", alg);
  registerSettingWidget(*m_ui.startOverlapEdit, "StartOverlap", alg);
  registerSettingWidget(*m_ui.endOverlapEdit, "EndOverlap", alg);
  registerSettingWidget(*m_ui.polCorrComboBox, "PolarizationAnalysis", alg);
  registerSettingWidget(*m_ui.CRhoEdit, "Rho", alg);
  registerSettingWidget(*m_ui.CAlphaEdit, "Alpha", alg);
  registerSettingWidget(*m_ui.CApEdit, "Ap", alg);
  registerSettingWidget(*m_ui.CPpEdit, "Pp", alg);
  registerSettingWidget(stitchOptionsLineEdit(), "Params", alg);
  registerSettingWidget(*m_ui.reductionTypeComboBox, "ReductionType", alg);
  registerSettingWidget(*m_ui.summationTypeComboBox, "SummationType", alg);
}

void ExperimentView::summationTypeChanged(int reductionTypeIndex) {
  UNUSED_ARG(reductionTypeIndex);
  m_notifyee->notifySummationTypeChanged();
}

void ExperimentView::enableReductionType() {
  m_ui.reductionTypeComboBox->setEnabled(true);
}

void ExperimentView::disableReductionType() {
  m_ui.reductionTypeComboBox->setEnabled(false);
}

template <typename Widget>
void ExperimentView::registerSettingWidget(Widget &widget,
                                           std::string const &propertyName,
                                           Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(widget);
  setToolTipAsPropertyDocumentation(widget, propertyName, alg);
}

void ExperimentView::setToolTipAsPropertyDocumentation(
    QWidget &widget, std::string const &propertyName,
    Mantid::API::IAlgorithm_sptr alg) {
  widget.setToolTip(QString::fromStdString(
      alg->getPointerToProperty(propertyName)->documentation()));
}

void ExperimentView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

void ExperimentView::setText(QLineEdit &lineEdit,
                             boost::optional<double> value) {
  if (value)
    setText(lineEdit, value.get());
}

void ExperimentView::setText(QLineEdit &lineEdit, boost::optional<int> value) {
  if (value)
    setText(lineEdit, value.get());
}

void ExperimentView::setText(QLineEdit &lineEdit,
                             boost::optional<std::string> const &text) {
  if (text && !text->empty())
    setText(lineEdit, text);
}

void ExperimentView::setText(QLineEdit &lineEdit, double value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void ExperimentView::setText(QLineEdit &lineEdit, int value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void ExperimentView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

// void ExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<double> value) {
//  if (value)
//    setText(table, propertyName, value.get());
//}
//
// void ExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName, double value) {
//  auto valueAsString = QString::number(value);
//  setText(table, propertyName, valueAsString);
//}
//
// void ExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<std::string> text) {
//  if (text && !text->empty())
//    setText(table, propertyName, text.get());
//}
//
// void ExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             std::string const &text) {
//  auto textAsQString = QString::fromStdString(text);
//  setText(table, propertyName, textAsQString);
//}
//
// void ExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             const QString &value) {
//  // Find the column with this property name
//  const auto columnIt =
//      std::find(m_columnProperties.begin(), m_columnProperties.end(),
//                QString::fromStdString(propertyName));
//  // Do nothing if column was not found
//  if (columnIt == m_columnProperties.end())
//    return;
//
//  const auto column = columnIt - m_columnProperties.begin();
//
//  // Set the value in this column for the first row. (We don't really know
//  // which row(s) the user might want updated so for now keep it simple.)
//  constexpr int row = 0;
//  auto cell = table.item(row, column);
//  if (!cell) {
//    cell = new QTableWidgetItem();
//    table.setItem(row, column, cell);
//  }
//  cell->setText(value);
//}

void ExperimentView::setChecked(QCheckBox &checkBox, bool checked) {
  auto checkedAsCheckState = checked ? Qt::Checked : Qt::Unchecked;
  checkBox.setCheckState(checkedAsCheckState);
}

void ExperimentView::enablePolarizationCorrections() {
  m_ui.polCorrComboBox->setEnabled(true);
  enablePolarizationCorrectionInputs();
}

void ExperimentView::disablePolarizationCorrections() {
  m_ui.polCorrComboBox->setEnabled(false);
  disablePolarizationCorrectionInputs();
  // Set polarization corrections text to 'None' when disabled
  setSelected(*m_ui.polCorrComboBox, "None");
  // Clear all parameters as well
  m_ui.CRhoEdit->clear();
  m_ui.CAlphaEdit->clear();
  m_ui.CApEdit->clear();
  m_ui.CPpEdit->clear();
}

void ExperimentView::enablePolarizationCorrectionInputs() {
  m_ui.CRhoEdit->setEnabled(true);
  m_ui.CAlphaEdit->setEnabled(true);
  m_ui.CApEdit->setEnabled(true);
  m_ui.CPpEdit->setEnabled(true);
}

void ExperimentView::disablePolarizationCorrectionInputs() {
  m_ui.CRhoEdit->setEnabled(false);
  m_ui.CAlphaEdit->setEnabled(false);
  m_ui.CApEdit->setEnabled(false);
  m_ui.CPpEdit->setEnabled(false);
}

void ExperimentView::onPerAngleDefaultsChanged(int row, int column) {
  m_notifyee->notifyPerAngleDefaultsChanged(row, column);
}

/** Add a new row to the transmission runs table **/
void ExperimentView::onNewPerThetaDefaultsRowRequested() {
  m_notifyee->notifyNewPerAngleDefaultsRequested();
}

void ExperimentView::addPerThetaDefaultsRow() {
  auto newRowIndex = m_ui.optionsTable->rowCount();
  // Select the first cell in the new row
  m_ui.optionsTable->insertRow(newRowIndex);
  initializeTableRow(*m_ui.optionsTable, newRowIndex);
  m_ui.optionsTable->setCurrentCell(newRowIndex, 0);
}

void ExperimentView::removePerThetaDefaultsRow(int rowIndex) {
  m_ui.optionsTable->removeRow(rowIndex);
}

std::string ExperimentView::getText(QLineEdit const &lineEdit) const {
  return lineEdit.text().toStdString();
}

std::string ExperimentView::getText(QComboBox const &box) const {
  return box.currentText().toStdString();
}

QString ExperimentView::messageFor(
    InstrumentParameterTypeMissmatch const &typeError) const {
  return QString::fromStdString(typeError.parameterName()) +
         " should hold an " + QString::fromStdString(typeError.expectedType()) +
         " value but does not.\n";
}

template <typename T, typename StringConverter>
std::string toCsv(std::vector<T> const &values, StringConverter toString) {
  std::vector<std::string> valuesAsStrings;
  valuesAsStrings.reserve(values.size());
  std::transform(values.cbegin(), values.cend(),
                 std::back_inserter(valuesAsStrings), toString);
  return boost::algorithm::join(valuesAsStrings, ", ");
}

QString ExperimentView::messageFor(
    std::vector<MissingInstrumentParameterValue> const &missingValues) const {
  auto missingNamesCsv = toCsv(
      missingValues,
      [](const MissingInstrumentParameterValue &missingValue) -> std::string {
        return missingValue.parameterName();
      });

  return QString::fromStdString(missingNamesCsv) +
         QString(missingValues.size() == 1 ? " is" : " are") +
         " not set in the instrument parameter file but should be.\n";
}

void ExperimentView::showOptionLoadErrors(
    std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
    std::vector<MissingInstrumentParameterValue> const &missingValues) {
  auto message = QString(
      "Unable to retrieve default values for the following parameters:\n");

  if (!missingValues.empty())
    message += messageFor(missingValues);

  for (auto &typeError : typeErrors)
    message += messageFor(typeError);

  QMessageBox::warning(
      this, "Failed to load one or more defaults from parameter file", message);
}

QLineEdit &ExperimentView::stitchOptionsLineEdit() const {
  return *static_cast<QLineEdit *>(m_stitchEdit);
}

/** Creates hints for 'Stitch1DMany'
 * @param hints :: Hints as a map
 */
void ExperimentView::createStitchHints(
    const std::vector<MantidWidgets::Hint> &hints) {

  // We want to add the stitch params box next to the stitch
  // label, so first find the label's position
  auto stitchLabelIndex = m_ui.expSettingsGrid->indexOf(m_ui.stitchLabel);
  int row, col, rowSpan, colSpan;
  m_ui.expSettingsGrid->getItemPosition(stitchLabelIndex, &row, &col, &rowSpan,
                                        &colSpan);
  // Create the new edit box and add it to the right of the label
  m_stitchEdit = new MantidWidgets::HintingLineEdit(this, hints);
  m_ui.expSettingsGrid->addWidget(m_stitchEdit, row, col + colSpan, 1, 3);
}

double ExperimentView::getCRho() const { return m_ui.CRhoEdit->value(); }

void ExperimentView::setCRho(double cRho) { m_ui.CRhoEdit->setValue(cRho); }

double ExperimentView::getCAlpha() const { return m_ui.CAlphaEdit->value(); }

void ExperimentView::setCAlpha(double cAlpha) {
  m_ui.CAlphaEdit->setValue(cAlpha);
}

double ExperimentView::getCAp() const { return m_ui.CApEdit->value(); }

void ExperimentView::setCAp(double cAp) { m_ui.CApEdit->setValue(cAp); }

double ExperimentView::getCPp() const { return m_ui.CPpEdit->value(); }

void ExperimentView::setCPp(double cPp) { m_ui.CPpEdit->setValue(cPp); }

std::string ExperimentView::getAnalysisMode() const {
  return getText(*m_ui.analysisModeComboBox);
}

void ExperimentView::setAnalysisMode(std::string const &analysisMode) {
  setSelected(*m_ui.analysisModeComboBox, analysisMode);
}

std::string ExperimentView::getSummationType() const {
  return getText(*m_ui.summationTypeComboBox);
}

void ExperimentView::setSummationType(std::string const &summationType) {
  return setSelected(*m_ui.summationTypeComboBox, summationType);
}

std::string ExperimentView::getReductionType() const {
  return getText(*m_ui.reductionTypeComboBox);
}

void ExperimentView::setReductionType(std::string const &reductionType) {
  return setSelected(*m_ui.reductionTypeComboBox, reductionType);
}

std::string
ExperimentView::textFromCell(QTableWidgetItem const *maybeNullItem) const {
  if (maybeNullItem != nullptr) {
    return maybeNullItem->text().toStdString();
  } else {
    return std::string();
  }
}

std::vector<std::array<std::string, 8>>
ExperimentView::getPerAngleOptions() const {
  auto const &table = *m_ui.optionsTable;
  auto rows = std::vector<std::array<std::string, 8>>();
  rows.reserve(table.rowCount());
  for (auto row = 0; row < table.rowCount(); ++row) {
    rows.emplace_back(std::array<std::string, 8>{
        textFromCell(table.item(row, 0)), textFromCell(table.item(row, 1)),
        textFromCell(table.item(row, 2)), textFromCell(table.item(row, 3)),
        textFromCell(table.item(row, 4)), textFromCell(table.item(row, 5)),
        textFromCell(table.item(row, 6)), textFromCell(table.item(row, 7))});
  }
  return rows;
}

void ExperimentView::showPerAngleOptionsAsInvalid(int row, int column) {
  m_ui.optionsTable->blockSignals(true);
  m_ui.optionsTable->item(row, column)->setBackground(QColor("#ffb8ad"));
  m_ui.optionsTable->blockSignals(false);
}

void ExperimentView::showPerAngleOptionsAsValid(int row) {
  m_ui.optionsTable->blockSignals(true);
  for (auto column = 0; column < m_ui.optionsTable->columnCount(); ++column)
    m_ui.optionsTable->item(row, column)->setBackground(Qt::transparent);
  m_ui.optionsTable->blockSignals(false);
}

double ExperimentView::getTransmissionStartOverlap() const {
  return m_ui.startOverlapEdit->value();
}

void ExperimentView::setTransmissionStartOverlap(double start) {
  m_ui.startOverlapEdit->setValue(start);
}

double ExperimentView::getTransmissionEndOverlap() const {
  return m_ui.endOverlapEdit->value();
}

void ExperimentView::showTransmissionRangeInvalid() {
  showAsInvalid(*m_ui.startOverlapEdit);
  showAsInvalid(*m_ui.endOverlapEdit);
}

void ExperimentView::showTransmissionRangeValid() {
  showAsValid(*m_ui.startOverlapEdit);
  showAsValid(*m_ui.endOverlapEdit);
}

void ExperimentView::setTransmissionEndOverlap(double end) {
  m_ui.endOverlapEdit->setValue(end);
}

void ExperimentView::setPolarizationCorrectionType(std::string const &type) {
  setSelected(*m_ui.polCorrComboBox, type);
}

std::string ExperimentView::getPolarizationCorrectionType() const {
  return getText(*m_ui.polCorrComboBox);
}

std::string ExperimentView::getStitchOptions() const {
  return getText(stitchOptionsLineEdit());
}

void ExperimentView::setStitchOptions(std::string const &stitchOptions) {
  setText(stitchOptionsLineEdit(), stitchOptions);
}

void showOptionLoadErrors(
    std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
    std::vector<MissingInstrumentParameterValue> const &missingValues);

} // namespace CustomInterfaces
} // namespace MantidQt
