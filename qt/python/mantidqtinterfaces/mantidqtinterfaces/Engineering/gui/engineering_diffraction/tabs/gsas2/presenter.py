# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name


class GSAS2Presenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view

        self.connect_view_signals()

    def connect_view_signals(self):
        self.view.set_refine_clicked(self.on_refine_clicked)

    def on_refine_clicked(self):
        load_params = self.view.get_load_parameters()
        refine_params = self.view.get_refinement_parameters()
        self.model.run_model(load_params, refine_params)
