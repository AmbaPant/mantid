# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import subprocess
import shutil
import time
import datetime

import matplotlib.pyplot as plt
import numpy as np
from mantid.geometry import CrystalStructure, ReflectionGenerator, ReflectionConditionFilter
from mantid.simpleapi import CreateWorkspace, logger
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2 import parse_inputs
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings


class GSAS2Model(object):

    def __init__(self):
        self.user_save_directory = None
        self.project_name = None
        self.x_min = None
        self.x_max = None
        self.number_histograms = None

    def call_subprocess(self, command_string):
        shell_output = subprocess.Popen([command_string.replace('"', '\\"')],
                                        shell=True,
                                        stdin=None,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE,
                                        close_fds=True,
                                        universal_newlines=True)
        return shell_output.communicate()

    def format_shell_output(self, title, shell_output_string):
        double_line = "-" * (len(title) + 2) + "\n" + "-" * (len(title) + 2)
        return "\n" * 3 + double_line + "\n " + title + " \n" + double_line + "\n" + shell_output_string + double_line + "\n" * 3

    def find_in_file(self, file_path, marker_string, start_of_value, end_of_value, strip_separator=None):
        value_string = None
        with open(file_path, 'rt', encoding='utf-8') as file:
            full_file_string = file.read().replace('\n', '')
            where_marker = full_file_string.rfind(marker_string)
            if where_marker != -1:
                where_value_start = full_file_string.find(start_of_value, where_marker)
                if where_value_start != -1:
                    where_value_end = full_file_string.find(end_of_value, where_value_start + 1)
                    value_string = full_file_string[where_value_start: where_value_end]
                    if strip_separator:
                        value_string = value_string.strip(strip_separator + " ")
                    else:
                        value_string = value_string.strip(" ")
        return value_string

    def find_phase_names_in_lst(self, file_path):
        phase_names = []
        marker_string = "Phase name"
        start_of_value = ":"
        end_of_value = "="
        strip_separator = ":"
        with open(file_path, 'rt', encoding='utf-8') as file:
            full_file_string = file.read().replace('\n', '')
            where_marker = full_file_string.find(marker_string)
            while where_marker != -1:
                where_value_start = full_file_string.find(start_of_value, where_marker)
                if where_value_start != -1:
                    where_value_end = full_file_string.find(end_of_value, where_value_start + 1)
                    phase_names.append(
                        full_file_string[where_value_start: where_value_end].strip(strip_separator + " "))
                    where_marker = full_file_string.find(marker_string, where_value_end)
                else:
                    where_marker = -1
        return phase_names

    def find_basis_block_in_file(self, file_path, marker_string, start_of_value, end_of_value):
        with open(file_path, 'rt', encoding='utf-8') as file:
            full_file_string = file.read()
            where_marker = full_file_string.find(marker_string)
            value_string = None
            if where_marker != -1:
                where_first_digit = -1
                index = int(where_marker)
                while index < len(full_file_string):
                    if full_file_string[index].isdecimal():
                        where_first_digit = index
                        break
                    index += 1
                if where_first_digit != -1:
                    where_start_of_line = full_file_string.rfind(start_of_value, 0, where_first_digit - 1)
                    if where_start_of_line != -1:
                        where_end_of_block = full_file_string.find(end_of_value, where_start_of_line)
                        # if "loop" not found then assume the end of the file is the end of this block
                        value_string = full_file_string[where_start_of_line: where_end_of_block]
                        for remove_string in ["Biso", "Uiso"]:
                            value_string = value_string.replace(remove_string, "")
        return value_string

    def read_basis(self, phase_file_path):
        basis_string = self.find_basis_block_in_file(phase_file_path, "atom", "\n", "loop")
        split_string_basis = basis_string.split()
        split_string_basis[0] = ''.join([i for i in split_string_basis[0] if not i.isdigit()])
        if len(split_string_basis[0]) == 2:
            split_string_basis[0] = ''.join([split_string_basis[0][0].upper(), split_string_basis[0][1].lower()])
        basis = " ".join(split_string_basis[0:6])  # assuming only one line in the basis block!!
        return basis

    def read_space_group(self, phase_file_path):
        space_group = self.find_in_file(phase_file_path, "_symmetry_space_group_name_H-M", '"', '"', strip_separator='"')
        # Insert "-" to make the space group work
        for character_index, character in enumerate(space_group):
            if character.isdigit():
                split_string = list(space_group)
                split_string.insert(character_index, "-")
                space_group = "".join(split_string)
        return space_group

    def choose_cell_lengths(self, overriding_cell_lengths: str, phase_file_path):
        if overriding_cell_lengths:
            if "," not in overriding_cell_lengths:
                overriding_cell_lengths_list = [float(overriding_cell_lengths)] * 3
            else:
                overriding_cell_lengths_list = [float(x) for x in overriding_cell_lengths.split(",")]

            if len(overriding_cell_lengths_list) != 3:
                raise ValueError(f"The number of Override Cell Length values ({len(overriding_cell_lengths_list)}) "
                                 + f"must be 1 or 3 (and separated by commas).")

            cell_lengths = " ".join([str(overriding_cell_lengths_list[0]),
                                     str(overriding_cell_lengths_list[1]),
                                     str(overriding_cell_lengths_list[2])])
        else:
            cell_length_a = self.find_in_file(phase_file_path, '_cell_length_a', ' ', '_cell_length_b')
            cell_length_b = self.find_in_file(phase_file_path, '_cell_length_b', ' ', '_cell_length_c')
            cell_length_c = self.find_in_file(phase_file_path, '_cell_length_c', ' ', '_cell')
            cell_lengths = " ".join([cell_length_a, cell_length_b, cell_length_c])
        return cell_lengths

    def create_pawley_reflections(self, cell_lengths, space_group, basis, dmin):
        structure = CrystalStructure(cell_lengths, space_group, basis)
        generator = ReflectionGenerator(structure)

        hkls = generator.getUniqueHKLsUsingFilter(dmin, 3.0, ReflectionConditionFilter.StructureFactor)
        dValues = generator.getDValues(hkls)
        pg = structure.getSpaceGroup().getPointGroup()
        # Make list of tuples and sort by d-values, descending, include point group for multiplicity.
        generated_reflections = sorted([[list(hkl), d, len(pg.getEquivalents(hkl))] for hkl, d in zip(hkls, dValues)],
                                       key=lambda x: x[1] - x[0][0] * 1e-6, reverse=True)
        return generated_reflections

    def check_for_output_file(self, temp_save_directory, name_of_project, file_extension, file_descriptor,
                              gsas_error_string):
        gsas_output_filename = name_of_project + file_extension
        if gsas_output_filename not in os.listdir(temp_save_directory):
            raise FileNotFoundError(
                f"GSAS-II call must have failed, as the output {file_descriptor} file was not found.",
                self.format_shell_output(title="Errors from GSAS-II", shell_output_string=gsas_error_string))
        return os.path.join(temp_save_directory, gsas_output_filename)

    def chop_to_limits(self, input_array, x, min_x, max_x):
        input_array[x <= min_x] = np.nan
        input_array[x >= max_x] = np.nan
        return input_array

    def read_gsas_lst_and_print_wR(self, result_filepath, histogram_data_files):
        with open(result_filepath, 'rt', encoding='utf-8') as file:
            result_string = file.read().replace('\n', '')
            for loop_histogram in histogram_data_files:
                where_loop_histogram = result_string.rfind(loop_histogram)
                if where_loop_histogram != -1:
                    where_loop_histogram_wR = result_string.find('Final refinement wR =', where_loop_histogram)
                    if where_loop_histogram_wR != -1:
                        where_loop_histogram_wR_end = result_string.find('%', where_loop_histogram_wR)
                        logger.notice(loop_histogram)
                        logger.notice(result_string[where_loop_histogram_wR: where_loop_histogram_wR_end + 1])

    def load_gsas_histogram(self, temp_save_directory, name_of_project, histogram_index, min_x, max_x):
        result_csv = os.path.join(temp_save_directory, name_of_project + f"_{histogram_index}.csv")
        my_data = np.transpose(np.genfromtxt(result_csv, delimiter=",", skip_header=39))
        # x  y_obs	weight	y_calc	y_bkg	Q
        x_values = my_data[0]
        y_obs = self.chop_to_limits(np.array(my_data[1]), x_values, min_x[histogram_index], max_x[histogram_index])
        y_calc = self.chop_to_limits(np.array(my_data[3]), x_values, min_x[histogram_index], max_x[histogram_index])
        y_diff = y_obs - y_calc
        y_diff -= np.max(np.ma.masked_invalid(y_diff))
        y_bkg = self.chop_to_limits(np.array(my_data[4]), x_values, min_x[histogram_index], max_x[histogram_index])
        y_data = np.concatenate((y_obs, y_calc, y_diff, y_bkg))

        gsas_histogram = CreateWorkspace(OutputWorkspace=f"gsas_histogram_{histogram_index}",
                                         DataX=np.tile(my_data[0], 4), DataY=y_data, NSpec=4)
        return gsas_histogram

    def load_gsas_reflections(self, temp_save_directory, name_of_project, histogram_index, phase_names):
        loaded_reflections = []
        for phase_name in phase_names:
            result_reflections_txt = os.path.join(temp_save_directory,
                                                  name_of_project + f"_reflections_{histogram_index}_{phase_name}.txt")
            loaded_reflections.append(np.loadtxt(result_reflections_txt))
        return loaded_reflections

    def plot_gsas_histogram(self, axis, plot_kwargs, gsas_histogram, reflection_positions, name_of_project, histogram_index, min_x, max_x):
        axis.plot(gsas_histogram, color='#1105f0', label='observed', linestyle='None', marker='+', wkspIndex=0)
        axis.plot(gsas_histogram, color='#246b01', label='calculated', wkspIndex=1)
        axis.plot(gsas_histogram, color='#09acb8', label='difference', wkspIndex=2)
        axis.plot(gsas_histogram, color='#ff0000', label='background', wkspIndex=3)
        axis.set_title(name_of_project + ' GSAS Refinement')
        _, y_max = axis.get_ylim()
        axis.plot(reflection_positions, [-0.10 * y_max] * len(reflection_positions),
                  color='#1105f0', label='reflections', linestyle='None', marker='|', mew=1.5, ms=8)
        axis.axvline(min_x[histogram_index], color='#246b01', linestyle='--')
        axis.axvline(max_x[histogram_index], color='#a80000', linestyle='--')
        axis.legend(fontsize=8.0).set_draggable(True)
        plt.show()

    def clear_input_components(self):
        self.user_save_directory = None
        self.project_name = None
        self.x_min = None
        self.x_max = None
        self.number_histograms = None

    def run_model(self, load_parameters, refinement_parameters, project_name, rb_num):

        success_state = 0
        self.clear_input_components()
        self.project_name = project_name

        '''Inputs Tutorial'''
        # path_to_gsas2 = "/home/danielmurphy/gsas2/"
        # save_directory = "/home/danielmurphy/Downloads/GSASdata/new_outputs/"
        # data_directory = "/home/danielmurphy/Downloads/GSASdata/"
        # refinement_method = "Rietveld"
        # data_files = ["PBSO4.XRA", "PBSO4.CWN"]
        # histogram_indexing = []
        # instrument_files = ["INST_XRY.PRM","inst_d1a.prm"]
        # phase_files = ["PbSO4-Wyckoff.cif"]
        #
        # self.x_min = [16.0, 19.0]
        # self.x_max = [158.4, 153.0]
        #
        # override_cell_lengths = [3.65, 3.65, 3.65] # in presenter force to empty or len3 list of floats
        # refine_background = True
        # refine_microstrain = True
        # refine_sigma_one = False
        # refine_gamma = False
        # d_spacing_min = 1.0
        # refine_unit_cell = True
        # refine_histogram_scale_factor = True  # True by default

        '''Inputs Mantid'''
        # path_to_gsas2 = "/home/danielmurphy/gsas2/"
        # save_directory = "/home/danielmurphy/Downloads/GSASdata/new_outputs/"
        # data_directory = "/home/danielmurphy/Desktop/GSASMantiddata_030322/"
        # refinement_method = "Pawley"
        # data_files = ["Save_gss_305761_307521_bank_1_bgsub.gsa"]  # ["ENGINX_305761_307521_all_banks_TOF.gss"]
        # histogram_indexing = [1]  # assume only indexing when using 1 histogram file
        # instrument_files = ["ENGINX_305738_bank_1.prm"]
        # phase_files = ["FE_GAMMA.cif"]
        #
        # self.x_min = [18401.18]
        # self.x_max = [50000.0]
        #
        # override_cell_lengths = [3.65, 3.65, 3.65] # in presenter force to empty or len3 list of floats
        # refine_background = True
        # refine_microstrain = True
        # refine_sigma_one = False
        # refine_gamma = False
        # d_spacing_min = 1.0
        # refine_unit_cell = True

        '''Inputs GUI'''
        path_to_gsas2 = output_settings.get_path_to_gsas2() + "/"   # "/home/danielmurphy/gsas2/"
        save_dir = os.path.join(output_settings.get_output_path())  #  "/home/danielmurphy/Downloads/GSASdata/new_outputs/"
        # save output
        gsas2_save_dirs = [os.path.join(save_dir, "GSAS2", "")]
        if rb_num:
            gsas2_save_dirs.append(os.path.join(save_dir, "User", rb_num, "GSAS2", ""))
            '''TODO: Decide how to tie calibration grouping to GSAS2 tab,
            can we load it from the focused data? Maybe just in the name of the focused data?'''
            # if calibration.group == GROUP.TEXTURE20 or calibration.group == GROUP.TEXTURE30:
            #     calib_dirs.pop(0)  # only save to RB directory to limit number files saved
        save_directory = gsas2_save_dirs[0]

        data_directory = ""  # "/home/danielmurphy/Desktop/GSASMantiddata_030322/"
        refinement_method = refinement_parameters[0]  # "Pawley"
        data_files = [load_parameters[2]]  # ["Save_gss_305761_307521_bank_1_bgsub.gsa"]  # ["ENGINX_305761_307521_all_banks_TOF.gss"]
        histogram_indexing = [int(x) for x in list(load_parameters[3])]  # [1]  # assume only indexing when using 1 histogram file
        instrument_files = [load_parameters[0]]  # ["ENGINX_305738_bank_1.prm"]
        phase_files = [load_parameters[1]]  # ["FE_GAMMA.cif"]

        self.x_min = [18401.18]
        self.x_max = [50000.0]

        refine_background = True
        refine_microstrain = refinement_parameters[2] # True
        refine_sigma_one = refinement_parameters[3] # False
        refine_gamma = refinement_parameters[4] # False
        d_spacing_min = 1.0
        refine_unit_cell = True

        ''' Pre exec calculations '''
        refine_histogram_scale_factor = True  # True by default

        self.user_save_directory = os.path.join(save_directory, self.project_name)
        temporary_save_directory = os.path.join(save_directory,
                                                datetime.datetime.now().strftime(
                                                    'tmp_EngDiff_GSASII_%Y-%m-%d_%H-%M-%S'))
        make_temporary_save_directory = ("mkdir -p " + temporary_save_directory)
        out_make_temporary_save_directory, err_make_temporary_save_directory = self.call_subprocess(
            make_temporary_save_directory)
        phase_filepath = os.path.join(data_directory, phase_files[0])
        chosen_cell_lengths = self.choose_cell_lengths(refinement_parameters[1],
                                                       phase_filepath)  # [3.65, 3.65, 3.65]  #  I
        # should force to empty, len1 or len3 list of floats
        mantid_pawley_reflections = None
        if refinement_method == 'Pawley':
            mantid_pawley_reflections = self.create_pawley_reflections(
                cell_lengths=chosen_cell_lengths,
                space_group=self.read_space_group(phase_filepath),
                basis=self.read_basis(phase_filepath),
                dmin=d_spacing_min)

        '''Validation'''
        self.number_histograms = len(data_files)
        if histogram_indexing and len(data_files) == 1:
            self.number_histograms = len(histogram_indexing)
        if len(self.x_min) != 0 and len(self.x_min) != self.number_histograms:
            raise ValueError(f"The number of x_min values ({len(self.x_min)}) must equal the"
                             + f"number of histograms ({self.number_histograms}))")

        if histogram_indexing and len(data_files) > 1:
            raise ValueError(f"Histogram indexing is currently only supported, when the "
                             + f"number of data_files ({len(data_files)}) == 1")

        if refinement_method == 'Pawley' and not mantid_pawley_reflections:
            raise ValueError(f"No Pawley Reflections were generated for the phases provided. Not calling GSAS-II.")

        if len(instrument_files) != 1 or len(instrument_files) != self.number_histograms:
            raise ValueError(f'The number of instrument files ({len(instrument_files)}) must be 1 '
                             f'or equal to the number of input histograms {self.number_histograms}')

        '''exec'''

        gsas2_inputs = parse_inputs.Gsas2Inputs(
            path_to_gsas2=path_to_gsas2,
            temporary_save_directory=temporary_save_directory,
            data_directory=data_directory,
            project_name=self.project_name,
            refinement_method=refinement_method,
            refine_background=refine_background,
            refine_microstrain=refine_microstrain,
            refine_sigma_one=refine_sigma_one,
            refine_gamma=refine_gamma,
            refine_histogram_scale_factor=refine_histogram_scale_factor,
            data_files=data_files,
            histogram_indexing=histogram_indexing,
            phase_files=phase_files,
            instrument_files=instrument_files,
            limits=[self.x_min, self.x_max],
            mantid_pawley_reflections=mantid_pawley_reflections,
            override_cell_lengths=[float(x) for x in chosen_cell_lengths.split(" ")],
            refine_unit_cell=refine_unit_cell,
            d_spacing_min=d_spacing_min
        )

        call_gsas2 = (path_to_gsas2 + "bin/python "
                      + "/home/danielmurphy/mantid/qt/python/mantidqtinterfaces/mantidqtinterfaces/Engineering/gui/"
                      + "engineering_diffraction/tabs/gsas2/call_G2sc.py "
                      + parse_inputs.Gsas2Inputs_to_json(gsas2_inputs)
                      )

        start = time.time()
        out_call_gsas2, err_call_gsas2 = self.call_subprocess(call_gsas2)
        gsas_runtime = time.time() - start
        logger.notice(
            self.format_shell_output(title="Commandline output from GSAS-II", shell_output_string=out_call_gsas2))

        self.check_for_output_file(temporary_save_directory, self.project_name, ".gpx", "project file", err_call_gsas2)

        gsas_result_filepath = self.check_for_output_file(temporary_save_directory, self.project_name, ".lst", "result",
                                                          err_call_gsas2)
        logger.notice(f"\nGSAS-II call complete in {gsas_runtime} seconds.\n")

        logger.notice(f"GSAS-II .lst result file found. Opening {self.project_name}.lst")
        self.read_gsas_lst_and_print_wR(gsas_result_filepath, data_files)

        for new_directory in gsas2_save_dirs:
            os.makedirs(new_directory, exist_ok=True)

        save_success_message = f"\n\nOutput GSAS-II files saved in {self.user_save_directory}"

        exist_extra_save_dirs = False
        if len(gsas2_save_dirs) > 1:
            exist_extra_save_dirs = True
            gsas2_save_dirs.pop(0)

        for output_file_index, output_file in enumerate(os.listdir(temporary_save_directory)):
            os.rename(os.path.join(temporary_save_directory, output_file),
                      os.path.join(self.user_save_directory, output_file))
            if exist_extra_save_dirs:
                for extra_save_dir in gsas2_save_dirs:
                    shutil.copy(os.path.join(self.user_save_directory, output_file),
                                os.path.join(extra_save_dir, output_file))
                    if output_file_index == 0:
                        save_success_message += f" and in {extra_save_dir}"

        os.rmdir(temporary_save_directory)
        logger.notice(save_success_message)
        success_state = 1
        return success_state

        # # open GSAS-II project
        # open_project_call = (path_to_gsas2 + "bin/python " + path_to_gsas2 + "GSASII/GSASII.py "
        #                      + os.path.join(self.user_save_directory, self.project_name + ".gpx"))
        # out_open_project_call, err_open_project_call = call_subprocess(open_project_call)

    def plot_result(self, axis, plot_kwargs):
        for index_histograms in range(self.number_histograms):
            gsas_histogram_workspace = self.load_gsas_histogram(self.user_save_directory, self.project_name,
                                                                index_histograms,
                                                                self.x_min, self.x_max)
            phase_names_list = self.find_phase_names_in_lst(
                os.path.join(self.user_save_directory, self.project_name + ".lst"))
            reflections = self.load_gsas_reflections(self.user_save_directory, self.project_name, index_histograms,
                                                     phase_names_list)
            self.plot_gsas_histogram(axis, plot_kwargs, gsas_histogram_workspace, reflections, self.project_name, index_histograms,
                                     self.x_min, self.x_max)
