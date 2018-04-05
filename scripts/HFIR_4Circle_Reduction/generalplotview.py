import HFIR_4Circle_Reduction.fourcircle_utility as fourcircle_utility
import HFIR_4Circle_Reduction.ui_general1dviewer as ui_general1dviewer
import os
from PyQt4.QtGui import QMainWindow, QFileDialog


class GeneralPlotWindow(QMainWindow):
    """
    A window for general-purposed graphic (1-D plot) view
    """
    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(GeneralPlotWindow, self).__init__(parent)

        # set up UI
        self.ui = ui_general1dviewer.Ui_MainWindow()
        self.ui.setupUi(self)

        # set up the event handling
        self.ui.pushButton_exportPlot2File.clicked.connect(self.do_export_plot)
        self.ui.pushButton_quit.clicked.connect(self.do_quit)
        self.ui.actionReset.triggered.connect(self.reset_window)

        # class variables
        self._work_dir = os.getcwd()

        return

    def do_export_plot(self):
        """
        export plot
        :return:
        """
        # get directory
        file_name = str(QFileDialog.getSaveFileName(self, caption='File to save the plot',
                                                    directory=self._work_dir,
                                                    filter='Data File(*.dat);;All Files(*.*'))
        if len(file_name) == 0:
            return

        self.ui.graphicsView_plotView.save_current_plot(file_name)

        return

    def do_quit(self):
        """
        close the window
        :return:
        """
        self.close()

        return

    def menu_reset_window(self):
        """
        reset the window, i.e., plot
        :return:
        """
        self.ui.graphicsView_plotView.reset_plots()

        return

    def set_working_dir(self, work_dir):
        """
        :param work_dir: working directory
        :return:
        """
        # check
        fourcircle_utility.check_str('Working directory', work_dir)
        if os.path.exists(work_dir) and os.access(work_dir, os.W_OK) is False:
            raise RuntimeError('Directory {0} is not writable.'.format(work_dir))
        elif not os.path.exists(work_dir):
            raise RuntimeError('Directory {0} does not exist.'.format(work_dir))
        else:
            self._work_dir = work_dir

        return

    def reset_window(self):
        """
        reset the window to the initial state, such that no plot is made on the canvas
        :return:
        """
        self.ui.graphicsView_plotView.reset_plots()

        return
