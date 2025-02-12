# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
#
#
"""
Functionality for dealing with legends on plots
"""
import matplotlib
from matplotlib.patches import BoxStyle

from mantid.kernel import ConfigService
from mantid.plots.utility import convert_color_to_hex, legend_set_draggable


class LegendProperties(dict):
    def __getattr__(self, item):
        return self[item]

    @classmethod
    def from_legend(cls, legend):
        if not (legend and legend.get_texts()):
            return None

        props = dict()

        props['visible'] = legend.get_visible()

        title = legend.get_title()
        if isinstance(title.get_text(), str):
            props['title'] = title.get_text()
        else:
            props['title'] = None

        # For Matplotlib <3.2 we have to remove the 'None' string from the title
        # which is generated by set_text() in text.py
        if props['title'] == 'None':
            props['title'] = ''

        props['title_font'] = title.get_fontname()
        props['title_size'] = title.get_fontsize()
        props['title_color'] = convert_color_to_hex(title.get_color())

        props['box_visible'] = legend.get_frame_on()

        box = legend.get_frame()
        props['background_color'] = convert_color_to_hex(box.get_facecolor())
        props['edge_color'] = convert_color_to_hex(box.get_edgecolor())
        props['transparency'] = box.get_alpha()

        text = legend.get_texts()[0]
        props['entries_font'] = text.get_fontname()
        props['entries_size'] = text.get_fontsize()
        props['entries_color'] = convert_color_to_hex(text.get_color())

        props['marker_size'] = legend.handlelength
        props['shadow'] = legend.shadow

        boxstyle = legend.legendPatch.get_boxstyle()
        if isinstance(boxstyle, BoxStyle.Round):
            props['round_edges'] = True
        else:
            props['round_edges'] = False

        props['columns'] = legend._ncol
        props['column_spacing'] = legend.columnspacing
        props['label_spacing'] = legend.labelspacing

        position = legend._legend_handle_box.get_children()[0].align
        if position == "baseline":
            props['marker_position'] = "Left of Entries"
        else:
            props['marker_position'] = "Right of Entries"

        props['markers'] = legend.numpoints
        props['border_padding'] = legend.borderpad
        props['marker_label_padding'] = legend.handletextpad

        return cls(props)

    @classmethod
    def from_view(cls, view):
        props = dict()
        props['visible'] = not view.hide_legend_check_box.isChecked()
        props['title'] = view.get_title()
        props['background_color'] = view.get_background_color()
        props['edge_color'] = view.get_edge_color()
        props['transparency'] = (100 - float(view.get_transparency_spin_box_value())) / 100
        props['entries_font'] = view.get_entries_font()
        props['entries_size'] = view.get_entries_size()
        props['entries_color'] = view.get_entries_color()
        props['title_font'] = view.get_title_font()
        props['title_size'] = view.get_title_size()
        props['title_color'] = view.get_title_color()
        props['marker_size'] = view.get_marker_size()
        props['box_visible'] = not view.get_hide_box()
        return cls(props)

    @classmethod
    def from_view_advanced(cls, view):
        props = dict()
        props['shadow'] = view.get_shadow()
        props['round_edges'] = view.get_round_edges()
        props['columns'] = view.get_number_of_columns()
        props['column_spacing'] = view.get_column_spacing()
        props['label_spacing'] = view.get_label_spacing()
        props['marker_position'] = view.get_marker_position()
        props['markers'] = view.get_number_of_markers()
        props['border_padding'] = view.get_border_padding()
        props['marker_label_padding'] = view.get_marker_label_padding()
        return cls(props)

    @classmethod
    def create_legend(cls, props, ax):
        # Imported here to prevent circular import.
        from mantid.plots.datafunctions import get_legend_handles

        loc = ConfigService.getString('plots.legend.Location')
        font_size = float(ConfigService.getString('plots.legend.FontSize'))
        if not props:
            legend_set_draggable(ax.legend(handles=get_legend_handles(ax), loc=loc,
                                           prop={'size': font_size}),
                                 True)
            return

        if 'loc' in props.keys():
            loc = props['loc']

        if int(matplotlib.__version__[0]) >= 2:
            legend = ax.legend(handles=get_legend_handles(ax),
                               ncol=props['columns'],
                               prop={'size': props['entries_size']},
                               numpoints=props['markers'],
                               markerfirst=props['marker_position'] == "Left of Entries",
                               frameon=props['box_visible'],
                               fancybox=props['round_edges'],
                               shadow=props['shadow'],
                               framealpha=props['transparency'],
                               facecolor=props['background_color'],
                               edgecolor=props['edge_color'],
                               title=props['title'],
                               borderpad=props['border_padding'],
                               labelspacing=props['label_spacing'],
                               handlelength=props['marker_size'],
                               handletextpad=props['marker_label_padding'],
                               columnspacing=props['column_spacing'],
                               loc=loc)
        else:
            legend = ax.legend(handles=get_legend_handles(ax),
                               ncol=props['columns'],
                               prop={'size': props['entries_size']},
                               numpoints=props['markers'],
                               markerfirst=props['marker_position'] == "Left of Entries",
                               frameon=props['box_visible'],
                               fancybox=props['round_edges'],
                               shadow=props['shadow'],
                               framealpha=props['transparency'],
                               title=props['title'],
                               borderpad=props['border_padding'],
                               labelspacing=props['label_spacing'],
                               handlelength=props['marker_size'],
                               handletextpad=props['marker_label_padding'],
                               columnspacing=props['column_spacing'],
                               loc=loc)

        title = legend.get_title()
        title.set_fontname(props['title_font'])
        title.set_fontsize(props['title_size'])
        title.set_color(props['title_color'])

        for text in legend.get_texts():
            text.set_fontname(props['entries_font'])
            text.set_fontsize(props['entries_size'])
            text.set_color(props['entries_color'])

        legend.set_visible(props['visible'])
        legend_set_draggable(legend, True)
