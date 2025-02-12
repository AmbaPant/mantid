# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from qtpy.QtWidgets import (QWidget, QHBoxLayout)
from qtpy.QtCore import Qt

from mantidqt.widgets.sliceviewer.views.dataview import SliceViewerDataView


class RegionSelectorView(QWidget):
    """The view for the data portion of the sliceviewer"""

    def __init__(self, presenter, parent=None, dims_info=None):
        super().__init__(parent)
        self.setWindowFlags(Qt.Window)
        self._data_view = SliceViewerDataView(presenter, dims_info, None, self, None)

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._data_view)
        self.setLayout(layout)

        self.setWindowTitle("Region Selector")

    def set_workspace(self, workspace):
        self._data_view.image_info_widget.setWorkspace(workspace)

    def create_dimensions(self, dims_info):
        self._data_view.create_dimensions(dims_info=dims_info)

    def create_axes_orthogonal(self, redraw_on_zoom):
        self._data_view.create_axes_orthogonal(redraw_on_zoom=redraw_on_zoom)
