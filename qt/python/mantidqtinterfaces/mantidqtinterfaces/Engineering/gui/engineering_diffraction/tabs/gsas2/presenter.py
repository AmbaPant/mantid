# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT

PLOT_KWARGS = {"linestyle": "", "marker": "x", "markersize": "3"}


class GSAS2Presenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view

        self.rb_num = None
        self.instrument = "ENGINX"
        self.connect_view_signals()

    def connect_view_signals(self):
        self.view.set_refine_clicked(self.on_refine_clicked)

    def on_refine_clicked(self):
        load_params = self.view.get_load_parameters()
        project_name = self.view.get_project_name()
        refine_params = self.view.get_refinement_parameters()
        success = self.model.run_model(load_params, refine_params, project_name, self.rb_num)
        if success:
            self.clear_plot()
            self.plot_result()

    def set_rb_num(self, rb_num):
        self.rb_num = rb_num

    def set_instrument_override(self, instrument):
        instrument = INSTRUMENT_DICT[instrument]
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    # ========
    # Plotting
    # ========

    def plot_result(self):
        axes = self.view.get_axes()
        for ax in axes:
            self.model.plot_result(ax, PLOT_KWARGS)
        self.view.update_figure()

    def clear_plot(self):
        self.view.clear_figure()
