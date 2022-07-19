# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from qtpy.QtGui import QRegExpValidator
from mantidqt.utils.qt import load_ui

Ui_calib, _ = load_ui(__file__, "gsas2_tab.ui")


class GSAS2View(QtWidgets.QWidget, Ui_calib):
    sig_enable_controls = QtCore.Signal(bool)
    sig_update_sample_field = QtCore.Signal()

    def __init__(self, parent=None, instrument="ENGINX"):
        super(GSAS2View, self).__init__(parent)
        self.setupUi(self)
        none_one_many_int_float_comma_separated = \
            QRegExpValidator(QtCore.QRegExp(r"^(?:\d+(?:\.\d*)?|\.\d+)(?:,(?:\d+(?:\.\d*)?|\.\d+))*$"),
                             self.override_unitcell_length)
        self.override_unitcell_length.setValidator(none_one_many_int_float_comma_separated)
        none_or_int_list_comma_separated = QRegExpValidator(QtCore.QRegExp(r"^(\d+(,\d+)*)?$/gm"),
                                                            self.override_unitcell_length)
        self.data_indexing_line_edit.setValidator(none_or_int_list_comma_separated)

        self.instrument_group_file_finder.setLabelText("Instrument Group")
        self.instrument_group_file_finder.isForRunFiles(False)
        self.instrument_group_file_finder.setFileExtensions([".prm"])

        self.phase_file_finder.setLabelText("Phase")
        self.phase_file_finder.isForRunFiles(False)
        self.phase_file_finder.setFileExtensions([".CIF", ".cif"])

        self.focused_data_file_finder.setLabelText("Focused Data")
        self.focused_data_file_finder.isForRunFiles(False)
        self.focused_data_file_finder.setFileExtensions([".gss", ".gsa"])

    # ===============
    # Slot Connectors
    # ===============

    def set_refine_clicked(self, slot):
        self.refine_button.clicked.connect(slot)

    def set_instrument_override(self, instrument):
        # self.finder_focus.setInstrumentOverride(instrument)
        # self.finder_vanadium.setInstrumentOverride(instrument)
        pass

    # =================
    # Component Setters
    # =================

    # =================
    # Component Getters
    # =================

    def get_refinement_parameters(self):
        return [self.refinement_method_combobox.currentText(), self.override_unitcell_length.text(),
                self.refine_microstrain_checkbox.isChecked(), self.refine_sigma_one_checkbox.isChecked(),
                self.refine_gamma_y_checkbox.isChecked()]

    def get_load_parameters(self):
        return [self.instrument_group_file_finder.getFirstFilename(), self.phase_file_finder.getFirstFilename(),
                self.focused_data_file_finder.getFirstFilename(), self.data_indexing_line_edit.text()]

    def get_project_name(self):
        return self.project_name_line_edit.text()

    # =================
    # Force Actions
    # =================

    def find_files_load_parameters(self):
        self.instrument_group_file_finder.findFiles(True)
        self.phase_file_finder.findFiles(True)
        self.focused_data_file_finder.findFiles(True)
