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

    def set_refine_clicked(self, slot):
        self.refine_button.clicked.connect(slot)

    def set_instrument_override(self, instrument):
        # self.finder_focus.setInstrumentOverride(instrument)
        # self.finder_vanadium.setInstrumentOverride(instrument)
        pass

    def get_refinement_parameters(self):
        return [self.refinement_method_combobox.currentText(), self.override_unitcell_length.text(),
                self.refine_microstrain_checkbox.isChecked(), self.refine_sigma_one_checkbox.isChecked(),
                self.refine_gamma_y_checkbox.isChecked()]

    def get_load_parameters(self):
        return [self.project_name_line_edit.text(), self.focused_data_path_line_edit.text(),
                self.data_indexing_line_edit.text(), self.phase_file_path_line_edit.text()]
