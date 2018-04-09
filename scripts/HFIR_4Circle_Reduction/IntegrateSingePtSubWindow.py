#pylint: disable=C0103
from __future__ import (absolute_import, division, print_function)
from PyQt4.QtCore import pyqtSignal
import HFIR_4Circle_Reduction.ui_SinglePtIntegrationWindow as window_ui
import HFIR_4Circle_Reduction.guiutility as guiutility
from PyQt4.QtGui import QMainWindow, QFileDialog
import os


class IntegrateSinglePtIntensityWindow(QMainWindow):
    """
    Main window widget to set up parameters to optimize
    """
    # establish signal for communicating from App2 to App1 - must be defined before the constructor
    scanIntegratedSignal = pyqtSignal(dict, name='SinglePtIntegrated')

    def __init__(self, parent=None):
        """
        Initialization
        :param parent:
        :return:
        """
        # init
        super(IntegrateSinglePtIntensityWindow, self).__init__(parent)

        assert parent is not None, 'Parent window cannot be None to set'
        self._parent_window = parent
        self._controller = parent.controller

        # connect signal handler
        self.scanIntegratedSignal.connect(self._parent_window.process_single_pt_scan_intensity)

        # init UI
        self.ui = window_ui.Ui_MainWindow()
        self.ui.setupUi(self)

        # initialize widgets
        self.ui.tableView_summary.setup()
        self.ui.graphicsView_integration1DView.set_parent_window(self)

        # define event handlers for widgets
        self.ui.pushButton_integrteDetectorCounts.clicked.connect(self.do_interate_detector_counts)
        self.ui.pushButton_load2thetaSigmaFile.clicked.connect(self.menu_load_gauss_sigma_file)

        self.ui.pushButton_exportIntensityToFile.clicked.connect(self.do_save_intensity)
        self.ui.pushButton_exportIntensityToTable.clicked.connect(self.do_export_intensity_to_parent)
        self.ui.pushButton_refreshROI.clicked.connect(self.do_refresh_roi)
        self.ui.pushButton_retrieveFWHM.clicked.connect(self.do_retrieve_fwhm)
        self.ui.pushButton_integratePeaks.clicked.connect(self.do_integrate_single_pt)
        self.ui.pushButton_plot.clicked.connect(self.do_plot_integrated_pt)

        # menu bar
        self.ui.menuQuit.triggered.connect(self.do_close)
        self.ui.actionSelect_All.triggered.connect(self.menu_table_select_all)
        self.ui.actionDe_select_All.triggered.connect(self.menu_table_select_none)
        self.ui.actionLoad_Gaussian_Sigma_File.triggered.connect(self.menu_load_gauss_sigma_file)
        self.ui.actionLoad_Peak_Info_File.triggered.connect(self.do_load_peak_integration_table)

        # class variable
        self._working_dir = os.path.expanduser('~')
        self._exp_number = None
        self._roiMutex = False

        # other things to do
        self.do_refresh_roi()

        return

    def do_close(self):
        """
        Quit the window
        :return:
        """
        self.close()

        return

    def do_export_intensity_to_parent(self):
        """
        export the integrated intensity to parent window's peak processing table
        :return:
        """
        # TEST NOW2
        # collect all scan/pt from table
        scan_pt_list = self.ui.tableView_summary.get_scan_pt_list()

        # add to table including calculate peak center in Q-space
        self.scanIntegratedSignal.emit(scan_pt_list)

        return

    def do_interate_detector_counts(self):
        """
        integrate the (selected) scan's detector counts by ROI
        :return:
        """
        # get ROI
        roi_name = str(self.ui.comboBox_roiList.currentText())
        if roi_name is None or roi_name == '':
            guiutility.show_message('A region-of-interest must be chosen in order to integrate detector counts.')
            return

        for row_number in range(self.ui.tableView_summary.rowCount()):
            # integrate counts on detector
            scan_number = self.ui.tableView_summary.get_scan_number(row_number)
            pt_number = 1
            peak_height = self._controller.integrate_detector_image(self._exp_number, scan_number, pt_number, roi_name,
                                                                    fit_gaussian=True)

            # add to table
            self.ui.tableView_summary.set_peak_height(scan_number, pt_number, peak_height, roi_name)
            # print ('[DB...BAT] SinglePt-Gaussian-Intensity = {0}'.format(peak_height))

        # END-FOR (row_number)

        return

    def do_integrate_single_pt(self):
        """
        integrate the 2D data inside region of interest along both axis-0 and axis-1 individually.
        and the result (as 1D data) will be saved to ascii file.
        the X values will be the corresponding pixel index either along axis-0 or axis-1
        :return:
        """
        # get ROI
        roi_name = str(self.ui.comboBox_roiList.currentText())
        if roi_name is None or roi_name == '':
            guiutility.show_message('A region-of-interest must be chosen in order to integrate detector counts.')
            return

        for row_number in range(self.ui.tableView_summary.rowCount()):
            # integrate counts on detector
            scan_number = self.ui.tableView_summary.get_scan_number(row_number)
            pt_number = 1
            # peak_height = self._controller.integrate_detector_image(self._exp_number, scan_number, pt_number, roi_name,
            #                                                         fit_gaussian=True)
            # # add to table
            # self.ui.tableView_summary.set_peak_height(scan_number, pt_number, peak_height)
            # print ('[DB...BAT] SinglePt-Gaussian-Intensity = {0}'.format(peak_height))

            # calculate peak intensity
            ref_fwhm = self.ui.tableView_summary.get_fwhm(row_number)

            intensity = self._controller.calculate_intensity_single_pt(self._exp_number, scan_number, pt_number,
                                                                       roi_name, ref_fwhm=ref_fwhm)

            # add to table
            self.ui.tableView_summary.set_intensity(scan_number, pt_number, intensity)
        # END-FOR

        return

    # TEST NOW
    def do_load_peak_integration_table(self):
        """
        load peak integration table CSV file saved from peak integration table
        :return:
        """
        # get table file name
        table_file = str(QFileDialog.getOpenFileName(self, 'Peak Integration Table', self._working_dir))
        if len(table_file) == 0 or os.path.exists(table_file) is False:
            return

        # load
        status, error_msg = self._controller.load_peak_integration_table(table_file)
        if not status:
            raise RuntimeError(error_msg)

        return

    def do_plot_integrated_pt(self):
        """
        plot integrated Pt with model
        :return:
        """
        # get scan number
        scan_number = int(self.ui.lineEdit_Scan.text())
        roi_name = str(self.ui.comboBox_roiList.currentText())

        # get data
        vec_x, vec_y, model_y = self._controller.get_single_scan_pt_result(self._exp_number, scan_number,
                                                                           pt_number=1, roi_name=roi_name)

        self.ui.graphicsView_integration1DView.add_observed_data(vec_x, vec_y, 'counts...')

        self.ui.graphicsView_integration1DView.add_fit_data(vec_x, model_y, 'model... ')

        return

    def do_refresh_roi(self):
        """
        refresh ROI list from parent
        :return:
        """
        roi_list = self._controller.get_region_of_interest_list()

        # add ROI
        self._roiMutex = True

        self.ui.comboBox_roiList.clear()
        for roi_name in sorted(roi_list):
            self.ui.comboBox_roiList.addItem(roi_name)

        self._roiMutex = False

        return

    def do_save_intensity(self):
        """
        save intensity to file
        :return:
        """
        # get output file
        out_file_name = str(QFileDialog.getSaveFileName(self, 'File to save integrated intensities', self._working_dir))
        if len(out_file_name) == 0:
            return

        self.ui.tableView_summary.save_intensities_to_file(out_file_name)

        return

    # TEST FIXME NOW3 - New feature!
    def do_retrieve_fwhm(self):
        """
        Get FWHM from integrated 'STRONG' peaks according to 2theta value
        :return:
        """
        row_number = self.ui.tableView_summary.rowCount()
        error_messages = ''
        for row_index in range(row_number):
            # check whether FWHM value is set up
            fwhm_i = self.ui.tableView_summary.get_fwhm(row_index)
            if fwhm_i is not None and fwhm_i > 1.E-10:
                continue

            # use interpolation to curve
            two_theta = self.ui.tableView_summary.get_two_theta(row_index)
            # TEST NOW3 - new method calculate_peak_integration_sigma
            try:
                gauss_sigma = self._controller.calculate_peak_integration_sigma(two_theta)
                scan_number = self.ui.tableView_summary.get_scan_number(row_index)
                pt_number = 1
                roi_name = self.ui.tableView_summary.get_region_of_interest_name(row_index)
                self.ui.tableView_summary.set_gaussian_sigma(row_index, gauss_sigma)
                self._controller.set_single_measure_peak_width(self._exp_number, scan_number, roi_name, gauss_sigma, is_fhwm=False)

            except RuntimeError as err:
                # error!
                error_messages += 'Unable to calculate sigma of row {0} due to {1}\n'.format(row_index, err)
                continue
            # END-IF-ELSE

            # The following will be deleted!
            # # get corresponding strong nuclear peak (complete scan number)
            # ref_scans = self.ui.tableView_summary.get_reference_scans(row_index)
            # if ref_scans is None:
            #     # get scan number
            #     scan_number = self.ui.tableView_summary.get_scan_number(row_index)
            #     # get 2theta and set 2theta
            #     two_theta = self._controller.get_sample_log_value(self._exp_number, scan_number, 1,
            #                                                       '2theta')
            #
            #     # locate reference scan number
            #     # consider to use interpolation to curve!
            #     complete_peak_scan_numbers = self._controller.find_scans_by_2theta(self._exp_number,
            #                                                                        two_theta, resolution=0.05,
            #                                                                        excluded_scans=[scan_number])
            #
            #     if len(complete_peak_scan_numbers) == 0:
            #         # no match.  stop!
            #         self.ui.tableView_summary.set_reference_scan_numbers(row_index, 'No match')
            #         continue
            #     else:
            #         self.ui.tableView_summary.set_reference_scan_numbers(row_index, complete_peak_scan_numbers)
            #         reference_scan_number = complete_peak_scan_numbers[0]
            #     # END-IF
            # else:
            #     reference_scan_number = ref_scans[0]
            # # END-IF
            #
            # # get integrated scan's FWHM
            # peak_info = \
            #     self._controller.get_peak_info(self._exp_number, reference_scan_number)
            # if peak_info is not None and peak_info.gaussian_fwhm is not None:
            #     self.ui.tableView_summary.set_fwhm(row_index, peak_info.gaussian_fwhm)
        # END-FOR

        # show error message if necessary
        if len(error_messages) > 0:
            guiutility.show_message(self, error_messages)

        return

    def menu_load_gauss_sigma_file(self):
        """
        load a Gaussian sigma curve for interpolation or matching
        :return:
        """
        # get the column ascii file name
        file_filter = 'Data Files (*.dat);;All Files (*.*)'
        twotheta_sigma_file_name = str(QFileDialog.getOpenFileName(self, self._working_dir,
                                                                   '2theta Gaussian-Sigma File',
                                                                   file_filter))
        if len(twotheta_sigma_file_name) == 0 or twotheta_sigma_file_name == 'None':
            return

        # set the file to controller
        try:
            vec_x, vec_y = self._controller.import_2theta_gauss_sigma_file(twotheta_sigma_file_name)
            # TODO NOW2 - show the figure in the general plot window
        except RuntimeError as run_err:
            guiutility.show_message(self, str(run_err))
            return

        return


    def menu_table_select_all(self):
        """
        select all rows in table
        :return:
        """
        self.ui.tableView_summary.select_all_rows(True)

    def menu_table_select_none(self):
        """
        de-select all rows in the able
        :return:
        """
        self.ui.tableView_summary.select_all_rows(False)

    def add_scans(self, scan_pt_list):
        """
        add scans' information to table, i.e., add line
        :param scan_pt_list:
        :return:
        """
        # check ... blabla
        print('[DB...BAT] Add scan_pt_info: {0}'.format(scan_pt_list))

        for scan_pt_info in scan_pt_list:
            scan_number, pt_number, hkl, two_theta = scan_pt_info
            print ('[DB...BAT] Add scan_pt_info')
            self.ui.tableView_summary.add_scan_pt(scan_number, pt_number, hkl, two_theta)
        # END-FOR

        return

    def add_scan(self, scan_number, pt_number, hkl_str, two_theta):
        """
        add single scan
        :param scan_number:
        :param pt_number:
        :param hkl_str:
        :param two_theta:
        :return:
        """
        self.ui.tableView_summary.add_scan_pt(scan_number, pt_number, hkl_str, two_theta)

    def set_experiment(self, exp_number):
        """
        blabla
        :param exp_number:
        :return:
        """
        # check .. blabla

        self._exp_number = exp_number

        return
