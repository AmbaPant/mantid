# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing
PLOT_KWARGS = {"linestyle": "", "marker": "x", "markersize": "3"}


class GSAS2Presenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view

        self.rb_num = None
        self.instrument = "ENGINX"
        self.focus_run_observer_gsas2 = GenericObserverWithArgPassing(
            self.view.set_default_gss_files)
        self.prm_filepath_observer_gsas2 = GenericObserverWithArgPassing(
            self.view.set_default_prm_files)
        self.connect_view_signals()

    def connect_view_signals(self):
        self.view.set_refine_clicked(self.on_refine_clicked)
        # self.view.set_terminate_clicked(self.on_terminate_clicked)
        self.view.number_output_histograms_combobox.currentTextChanged.connect(self.on_plot_index_changed)

    def on_refine_clicked(self):
        self.clear_plot()
        load_params = self.view.get_load_parameters()
        project_name = self.view.get_project_name()
        refine_params = self.view.get_refinement_parameters()
        number_output_histograms = self.model.run_model(load_params, refine_params, project_name, self.rb_num)
        self.plot_result(1)
        self.view.set_number_histograms(number_output_histograms)

    def on_plot_index_changed(self, new_plot_index):
        self.plot_result(new_plot_index)

    def set_rb_num(self, rb_num):
        self.rb_num = rb_num

    def set_instrument_override(self, instrument):
        instrument = INSTRUMENT_DICT[instrument]
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    # ========
    # Plotting
    # ========

    def plot_result(self, output_histogram_index):
        self.clear_plot()
        axes = self.view.get_axes()
        output_histogram_index = int(output_histogram_index)
        output_histogram_index -= 1  # convert to zero-based indexing
        for ax in axes:
            self.model.plot_result(output_histogram_index, ax, PLOT_KWARGS)
        self.view.update_figure()

    def clear_plot(self):
        self.view.clear_figure()

    # def on_terminate_clicked(self):
    #     self.model.terminate_gsas2()
    #     print("Terminating")
