# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from qtpy.QtGui import QRegExpValidator
from qtpy.QtCore import Qt
# from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QDockWidget, QMainWindow, QSizePolicy  # QMenu
from os import path

from mantidqt.utils.qt import load_ui
from matplotlib.figure import Figure
from mantidqt.MPLwidgets import FigureCanvas

# from workbench.plotting.toolbar import ToolbarStateManager
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.plot_toolbar import GSAS2PlotToolbar

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
        self.instrument_group_file_finder.setFileExtensions([".prm, .xra"])
        self.instrument_group_file_finder.allowMultipleFiles(True)

        self.phase_file_finder.setLabelText("Phase")
        self.phase_file_finder.isForRunFiles(False)
        self.phase_file_finder.setFileExtensions([".cif"])
        self.phase_file_finder.allowMultipleFiles(True)

        self.focused_data_file_finder.setLabelText("Focused Data")
        self.focused_data_file_finder.isForRunFiles(False)
        self.focused_data_file_finder.setFileExtensions([".gss", ".gsa"])
        self.focused_data_file_finder.allowMultipleFiles(True)

        # Plotting
        self.figure = None
        self.toolbar = None
        self.plot_dock = None
        self.dock_window = None
        self.initial_chart_width = None
        self.initial_chart_height = None
        self.has_first_undock_occurred = 0

        self.setup_figure()
        self.setup_toolbar()

    # ===============
    # Slot Connectors
    # ===============

    def set_refine_clicked(self, slot):
        self.refine_button.clicked.connect(slot)

    # def set_terminate_clicked(self, slot):
    #     self.terminate_button.clicked.connect(slot)

    # def set_plot_index_changed(self, slot):
    #     self.number_output_histograms_combobox.currentTextChanged(slot)

    def set_instrument_override(self, instrument):
        # self.finder_focus.setInstrumentOverride(instrument)
        # self.finder_vanadium.setInstrumentOverride(instrument)
        pass

    def set_default_gss_files(self, filepaths):
        self.set_default_files(filepaths, self.focused_data_file_finder)

    def set_default_prm_files(self, filepaths):
        self.set_default_files(filepaths, self.instrument_group_file_finder)

    def set_default_files(self, filepaths, file_finder):
        if not filepaths:
            return
        file_finder.setUserInput(",".join(filepaths))
        directories = set()
        for filepath in filepaths:
            directory, discard = path.split(filepath)
            directories.add(directory)
        if len(directories) == 1:
            file_finder.setLastDirectory(directory)

    def set_number_histograms(self, number_output_histograms):
        print(number_output_histograms)
        self.number_output_histograms_combobox.clear()
        histogram_indices_items = [str(k) for k in range(1, number_output_histograms+1)]
        self.number_output_histograms_combobox.addItems(histogram_indices_items)
        self.number_output_histograms_combobox.setCurrentIndex(0)

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
        return [self.instrument_group_file_finder.getFilenames(), self.phase_file_finder.getFilenames(),
                self.focused_data_file_finder.getFilenames(), self.data_indexing_line_edit.text()]

    def get_project_name(self):
        return self.project_name_line_edit.text()

    # =================
    # Force Actions
    # =================

    def find_files_load_parameters(self):
        self.instrument_group_file_finder.findFiles(True)
        self.phase_file_finder.findFiles(True)
        self.focused_data_file_finder.findFiles(True)

    # ========
    # Plotting
    # ========

    def setup_figure(self):
        self.figure = Figure()
        self.figure.canvas = FigureCanvas(self.figure)
        # self.figure.canvas.mpl_connect('button_press_event', self.mouse_click)
        self.figure.add_subplot(111, projection="mantid")
        self.toolbar = GSAS2PlotToolbar(self.figure.canvas, self, False)
        self.toolbar.setMovable(False)

        self.dock_window = QMainWindow(self.group_plot)
        self.dock_window.setWindowFlags(Qt.Widget)
        self.dock_window.setDockOptions(QMainWindow.AnimatedDocks)
        self.dock_window.setCentralWidget(self.toolbar)
        self.plot_dock = QDockWidget()
        self.plot_dock.setWidget(self.figure.canvas)
        self.plot_dock.setFeatures(QDockWidget.DockWidgetFloatable | QDockWidget.DockWidgetMovable)
        self.plot_dock.setAllowedAreas(Qt.BottomDockWidgetArea)
        self.plot_dock.setWindowTitle("GSAS II Plot")
        self.plot_dock.topLevelChanged.connect(self.make_undocked_plot_larger)
        self.initial_chart_width, self.initial_chart_height = self.plot_dock.width(), self.plot_dock.height()
        self.plot_dock.setSizePolicy(QSizePolicy(QSizePolicy.MinimumExpanding,
                                                 QSizePolicy.MinimumExpanding))
        self.dock_window.addDockWidget(Qt.BottomDockWidgetArea, self.plot_dock)
        self.vLayout_plot.addWidget(self.dock_window)

    def resizeEvent(self, QResizeEvent):
        self.update_axes_position()

    def make_undocked_plot_larger(self):
        # only make undocked plot larger the first time it is undocked as the undocked size gets remembered
        if self.plot_dock.isFloating() and self.has_first_undock_occurred == 0:
            factor = 1.0
            aspect_ratio = self.initial_chart_width / self.initial_chart_height
            new_height = self.initial_chart_height * factor
            docked_height = self.dock_window.height()
            if docked_height > new_height:
                new_height = docked_height
            new_width = new_height * aspect_ratio
            self.plot_dock.resize(new_width, new_height)
            self.has_first_undock_occurred = 1

        self.update_axes_position()

    def ensure_fit_dock_closed(self):
        if self.plot_dock.isFloating():
            self.plot_dock.close()

    def setup_toolbar(self):
        self.toolbar.sig_home_clicked.connect(self.display_all)

    # =================
    # Component Setters
    # =================

    def clear_figure(self):
        self.figure.clf()
        self.figure.add_subplot(111, projection="mantid")
        self.figure.canvas.draw()

    def update_figure(self):
        self.toolbar.update()
        self.update_legend(self.get_axes()[0])
        self.update_axes_position()
        self.figure.canvas.draw()

    def update_axes_position(self):
        """
        Set axes position so that labels are always visible - it deliberately ignores height of ylabel (and less
        importantly the length of xlabel). This is because the plot window typically has a very small height when docked
        in the UI.
        """
        ax = self.get_axes()[0]
        y0_lab = ax.xaxis.get_tightbbox(renderer=self.figure.canvas.get_renderer()).transformed(
            self.figure.transFigure.inverted()).y0  # vertical coord of bottom left corner of xlabel in fig ref. frame
        x0_lab = ax.yaxis.get_tightbbox(renderer=self.figure.canvas.get_renderer()).transformed(
            self.figure.transFigure.inverted()).x0  # horizontal coord of bottom left corner ylabel in fig ref. frame
        pos = ax.get_position()
        x0_ax = pos.x0 + 0.05 - x0_lab  # move so that ylabel left bottom corner at horizontal coord 0.05
        y0_ax = pos.y0 + 0.05 - y0_lab  # move so that xlabel left bottom corner at vertical coord 0.05
        ax.set_position([x0_ax, y0_ax, 0.95-x0_ax, 0.95-y0_ax])

    def update_legend(self, ax):
        if ax.get_lines():
            ax.make_legend()
            ax.get_legend().set_title("")
        else:
            if ax.get_legend():
                ax.get_legend().remove()

    def display_all(self):
        for ax in self.get_axes():
            if ax.lines:
                ax.relim()
            ax.autoscale()
        self.update_figure()

    # =================
    # Component Getters
    # =================

    def get_axes(self):
        return self.figure.axes

    def get_figure(self):
        return self.figure
