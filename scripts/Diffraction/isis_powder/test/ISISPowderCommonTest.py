# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as mantid  # Have to import Mantid to setup paths
import unittest

from isis_powder.routines import common, common_enums


class ISISPowderCommonTest(unittest.TestCase):

    def test_cal_map_dict_helper(self):
        missing_key_name = "wrong_key"
        correct_key_name = "right_key"
        expected_val = 123
        dict_with_key = {correct_key_name: expected_val}

        # Check it correctly raises
        with self.assertRaisesRegex(KeyError, "The field '" + missing_key_name + "' is required"):
            common.cal_map_dictionary_key_helper(dictionary=dict_with_key, key=missing_key_name)

        # Check it correctly appends the passed error message when raising
        appended_e_msg = "test append message"
        with self.assertRaisesRegex(KeyError, appended_e_msg):
            common.cal_map_dictionary_key_helper(dictionary=dict_with_key, key=missing_key_name,
                                                 append_to_error_message=appended_e_msg)

        # Check that it correctly returns the key value where it exists
        self.assertEqual(common.cal_map_dictionary_key_helper(dictionary=dict_with_key, key=correct_key_name),
                         expected_val)

        # Check it is not case sensitive
        different_case_name = "tEsT_key"
        dict_with_mixed_key = {different_case_name: expected_val}

        try:
            self.assertEqual(common.cal_map_dictionary_key_helper(dictionary=dict_with_mixed_key,
                                                                  key=different_case_name.lower()), expected_val)
        except KeyError:
            # It tried to use the key without accounting for the case difference
            self.fail("cal_map_dictionary_key_helper attempted to use a key without accounting for case")

    def test_crop_banks_using_crop_list(self):
        bank_list = []
        cropping_value = (0, 1000)  # Crop to 0-1000 microseconds for unit tests
        cropping_value_list = []

        expected_number_of_bins = cropping_value[-1] - cropping_value[0]

        for i in range(0, 3):
            out_name = "crop_banks_in_tof-" + str(i)
            cropping_value_list.append(cropping_value)
            bank_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, XMin=0, XMax=1100, BinWidth=1))

        # Check a list of WS and single cropping value is detected
        with self.assertRaisesRegex(ValueError, "The cropping values were not in a list or tuple type"):
            common.crop_banks_using_crop_list(bank_list=bank_list, crop_values_list=1000)

        # Check a list of cropping values and a single workspace is detected
        with self.assertRaisesRegex(RuntimeError, "Attempting to use list based cropping"):
            common.crop_banks_using_crop_list(bank_list=bank_list[0], crop_values_list=cropping_value_list)

        # What about a mismatch between the number of cropping values and workspaces
        with self.assertRaisesRegex(RuntimeError, "The number of TOF cropping values does not match"):
            common.crop_banks_using_crop_list(bank_list=bank_list[1:], crop_values_list=cropping_value_list)

        # Check we can crop a single workspace from the list
        cropped_single_ws_list = common.crop_banks_using_crop_list(bank_list=[bank_list[0]],
                                                                   crop_values_list=[cropping_value])
        self.assertEqual(cropped_single_ws_list[0].blocksize(), expected_number_of_bins)
        mantid.DeleteWorkspace(Workspace=cropped_single_ws_list[0])

        # Check we can crop a whole list
        cropped_ws_list = common.crop_banks_using_crop_list(bank_list=bank_list[1:],
                                                            crop_values_list=cropping_value_list[1:])
        for ws in cropped_ws_list[1:]:
            self.assertEqual(ws.blocksize(), expected_number_of_bins)
            mantid.DeleteWorkspace(Workspace=ws)

    def test_crop_banks_using_a_single_fractional_cropping_value(self):
        bank_list = []
        cropping_value = (0.05, 0.95)  # Crop to 0-1000 microseconds for unit tests
        x_min = 500
        x_max = 1000

        expected_number_of_bins = x_max * cropping_value[-1] - x_min * (1+cropping_value[0])

        for i in range(0, 3):
            out_name = "crop_banks_in_tof-" + str(i)
            bank_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, XMin=x_min, XMax=x_max, BinWidth=1))

        # Check a list of WS and non list or tuple
        with self.assertRaisesRegex(ValueError, "The cropping values were not in a list or tuple type"):
            common.crop_banks_using_crop_list(bank_list=bank_list, crop_values_list=1000)

        # Check a cropping value and a single workspace is detected
        with self.assertRaisesRegex(RuntimeError, "Attempting to use list based cropping"):
            common.crop_banks_using_crop_list(bank_list=bank_list[0], crop_values_list=cropping_value)

        # Check we can crop a single workspace from the list
        cropped_single_ws_list = common.crop_banks_using_crop_list(bank_list=[bank_list[0]],
                                                                   crop_values_list=cropping_value)
        self.assertEqual(cropped_single_ws_list[0].blocksize(), expected_number_of_bins)
        mantid.DeleteWorkspace(Workspace=cropped_single_ws_list[0])

        # Check we can crop a whole list
        cropped_ws_list = common.crop_banks_using_crop_list(bank_list=bank_list[1:],
                                                            crop_values_list=cropping_value)
        for ws in cropped_ws_list[1:]:
            self.assertEqual(ws.blocksize(), expected_number_of_bins)
            mantid.DeleteWorkspace(Workspace=ws)

    def test_crop_in_tof(self):
        ws_list = []
        x_min = 100
        x_max = 500  # Crop to 0-500 microseconds for unit tests
        expected_number_of_bins = x_max - x_min

        for i in range(0, 3):
            out_name = "crop_banks_in_tof-" + str(i)
            ws_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, XMin=0, XMax=600, BinWidth=1))

        # Crop a single workspace in TOF
        tof_single_ws = common.crop_in_tof(ws_to_crop=ws_list[0], x_min=x_min, x_max=x_max)
        self.assertEqual(tof_single_ws.blocksize(), expected_number_of_bins)
        mantid.DeleteWorkspace(tof_single_ws)

        # Crop a list of workspaces in TOF
        cropped_ws_list = common.crop_in_tof(ws_to_crop=ws_list[1:], x_min=x_min, x_max=x_max)
        for ws in cropped_ws_list:
            self.assertEqual(ws.blocksize(), expected_number_of_bins)
            mantid.DeleteWorkspace(ws)

    def test_crop_in_tof_by_fraction(self):
        ws_list = []
        x_min = 0.2
        x_max = 0.8  # Crop 20% from the front and back.
        expected_number_of_bins = 600 * x_max - 100 * (1 + x_min)

        for i in range(0, 3):
            out_name = "crop_banks_in_tof-" + str(i)
            ws_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, XMin=100, XMax=600, BinWidth=1))

        # Crop a single workspace in TOF
        tof_single_ws = common.crop_in_tof(ws_to_crop=ws_list[0], x_min=x_min, x_max=x_max)
        self.assertEqual(tof_single_ws.blocksize(), expected_number_of_bins)
        mantid.DeleteWorkspace(tof_single_ws)

        # Crop a list of workspaces in TOF
        cropped_ws_list = common.crop_in_tof(ws_to_crop=ws_list[1:], x_min=x_min, x_max=x_max)
        for ws in cropped_ws_list:
            self.assertEqual(ws.blocksize(), expected_number_of_bins)
            mantid.DeleteWorkspace(ws)

    def test_crop_in_tof_by_fraction_fails_when_max_less_than_min(self):
        ws_list = []
        x_min = 0.8
        x_max = 0.799999  # Crop 20% from the front and back.

        for i in range(0, 3):
            out_name = "crop_banks_in_tof-" + str(i)
            ws_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, XMin=100, XMax=600, BinWidth=1))

        # Crop a single workspace in TOF
        self.assertRaises(ValueError, common.crop_in_tof, ws_to_crop=ws_list[0], x_min=x_min, x_max=x_max)

        # Crop a list of workspaces in TOF
        self.assertRaises(ValueError, common.crop_in_tof, ws_to_crop=ws_list[1:], x_min=x_min, x_max=x_max)

    def test_crop_in_tof_coverts_units(self):
        # Checks that crop_in_tof converts to TOF before cropping
        ws_list = []
        x_min = 100
        x_max = 200
        expected_number_of_bins = 20000  # Hard code number of expected bins for dSpacing

        for i in range(0, 3):
            out_name = "crop_banks_in_dSpacing-" + str(i)
            ws_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, XMin=0, XMax=20000, BinWidth=1,
                                                        XUnit="dSpacing"))

        # Crop a single workspace from d_spacing and check the number of bins
        tof_single_ws = common.crop_in_tof(ws_to_crop=ws_list[0], x_min=x_min, x_max=x_max)
        self.assertEqual(tof_single_ws.blocksize(), expected_number_of_bins)
        mantid.DeleteWorkspace(tof_single_ws)

        # Crop a list of workspaces in dSpacing
        cropped_ws_list = common.crop_in_tof(ws_to_crop=ws_list[1:], x_min=x_min, x_max=x_max)
        for ws in cropped_ws_list:
            self.assertEqual(ws.blocksize(), expected_number_of_bins)
            mantid.DeleteWorkspace(ws)

    def test_dictionary_key_helper(self):
        good_key_name = "key_exists"
        bad_key_name = "key_does_not_exist"

        test_dictionary = {good_key_name: 123}

        e_msg = "test message"

        with self.assertRaises(KeyError):
            common.dictionary_key_helper(dictionary=test_dictionary, key=bad_key_name)

        with self.assertRaisesRegex(KeyError, e_msg):
            common.dictionary_key_helper(dictionary=test_dictionary, key=bad_key_name, exception_msg=e_msg)

        self.assertEqual(common.dictionary_key_helper(dictionary=test_dictionary, key=good_key_name), 123)

    def test_dictionary_key_helper_handles_mixed_case(self):
        mixed_case_name = "tEsT_KeY"
        lower_case_name = mixed_case_name.lower()
        expected_val = 456

        mixed_case_dict = {mixed_case_name: expected_val}

        # Check by default it doesn't try to account for key
        with self.assertRaises(KeyError):
            common.dictionary_key_helper(dictionary=mixed_case_dict, key=lower_case_name)

        # Next check if we have the flag set to False it still throws
        with self.assertRaises(KeyError):
            common.dictionary_key_helper(dictionary=mixed_case_dict, key=lower_case_name, case_insensitive=False)

        # Check we actually get the key when we do ask for case insensitive checks
        try:
            val = common.dictionary_key_helper(dictionary=mixed_case_dict, key=lower_case_name, case_insensitive=True)
            self.assertEqual(val, expected_val)
        except KeyError:
            self.fail("dictionary_key_helper did not perform case insensitive lookup")

    def test_extract_ws_spectra(self):
        number_of_expected_banks = 5
        ws_to_split = mantid.CreateSampleWorkspace(XMin=0, XMax=2, BankPixelWidth=1,
                                                   NumBanks=number_of_expected_banks)
        input_name = ws_to_split.name()

        extracted_banks = common.extract_ws_spectra(ws_to_split=ws_to_split)
        self.assertEqual(len(extracted_banks), number_of_expected_banks)
        for i, ws in enumerate(extracted_banks):
            expected_name = input_name + '-' + str(i + 1)
            self.assertEqual(expected_name, ws.name())

    def test_generate_run_numbers(self):
        # Mantid handles most of this for us

        # First check it can handle int types
        test_int_input = 123
        int_input_return = common.generate_run_numbers(run_number_string=test_int_input)
        # Expect the returned type is a list
        self.assertEqual(int_input_return, [test_int_input])

        # Check it can handle 10-12 and is inclusive
        input_string = "10-12"
        expected_values = [10, 11, 12]
        returned_values = common.generate_run_numbers(run_number_string=input_string)
        self.assertEqual(expected_values, returned_values)

        # Check that the underscore syntax used by older pearl_routines scripts is handled
        input_string = "10_12"
        returned_values = common.generate_run_numbers(run_number_string=input_string)
        self.assertEqual(expected_values, returned_values)

        # Check that the comma notation is working
        input_string = "20, 22, 24"
        expected_values = [20, 22, 24]
        returned_values = common.generate_run_numbers(run_number_string=input_string)
        self.assertEqual(expected_values, returned_values)

        # Check we can use a combination of both
        input_string = "30-33, 36, 38-39"
        expected_values = [30, 31, 32, 33, 36, 38, 39]
        returned_values = common.generate_run_numbers(run_number_string=input_string)
        self.assertEqual(expected_values, returned_values)

    def test_generate_spline_vanadium_name(self):
        reference_vanadium_name = "foo_123"
        sample_arg_one = "arg1"
        sample_arg_two = 987

        # Check that it correctly processes unnamed args
        output = common.generate_splined_name(reference_vanadium_name, sample_arg_one, sample_arg_two)
        expected_output = "VanSplined_" + reference_vanadium_name + '_' + sample_arg_one + '_' + str(sample_arg_two)
        expected_output += '.nxs'
        self.assertEqual(expected_output, output)

        # Check it can handle lists to append
        sample_arg_list = ["bar", "baz", 567]

        expected_output = "VanSplined_" + reference_vanadium_name
        for arg in sample_arg_list:
            expected_output += '_' + str(arg)
        expected_output += '.nxs'
        output = common.generate_splined_name(reference_vanadium_name, sample_arg_list)
        self.assertEqual(expected_output, output)

        # Check is can handle mixed inputs
        expected_output = "VanSplined_" + reference_vanadium_name + '_' + sample_arg_one
        for arg in sample_arg_list:
            expected_output += '_' + str(arg)
        expected_output += '_' + str(sample_arg_two) + '.nxs'

        output = common.generate_splined_name(reference_vanadium_name, sample_arg_one, sample_arg_list, sample_arg_two)
        self.assertEqual(expected_output, output)

    def test_generate_run_numbers_fails(self):
        run_input_sting = "text-string"

        with self.assertRaisesRegex(ValueError, "Could not generate run numbers from this input"):
            common.generate_run_numbers(run_number_string=run_input_sting)

        # Check it says what the actual string was
        with self.assertRaisesRegex(ValueError, run_input_sting):
            common.generate_run_numbers(run_number_string=run_input_sting)

    def test_load_current_normalised_workspace(self):
        run_number_single = 100
        run_number_range = "100-101"

        bin_index = 8
        first_run_bin_value = 0.59706224
        second_run_bin_value = 1.48682782

        # Check it handles a single workspace correctly
        single_workspace = common.load_current_normalised_ws_list(run_number_string=run_number_single,
                                                                  instrument=ISISPowderMockInst())
        # Get the only workspace in the list, ask for the 0th spectrum and the value at the 200th bin
        self.assertTrue(isinstance(single_workspace, list))
        self.assertEqual(len(single_workspace), 1)
        self.assertAlmostEqual(single_workspace[0].readY(0)[bin_index], first_run_bin_value)
        mantid.DeleteWorkspace(single_workspace[0])

        # Does it return multiple workspaces when instructed
        multiple_ws = common.load_current_normalised_ws_list(
            run_number_string=run_number_range, instrument=ISISPowderMockInst(),
            input_batching=common_enums.INPUT_BATCHING.Individual)

        self.assertTrue(isinstance(multiple_ws, list))
        self.assertEqual(len(multiple_ws), 2)

        # Check the bins haven't been summed
        self.assertAlmostEqual(multiple_ws[0].readY(0)[bin_index], first_run_bin_value)
        self.assertAlmostEqual(multiple_ws[1].readY(0)[bin_index], second_run_bin_value)
        for ws in multiple_ws:
            mantid.DeleteWorkspace(ws)

        # Does it sum workspaces when instructed
        summed_ws = common.load_current_normalised_ws_list(
            run_number_string=run_number_range, instrument=ISISPowderMockInst(),
            input_batching=common_enums.INPUT_BATCHING.Summed)

        self.assertTrue(isinstance(summed_ws, list))
        self.assertEqual(len(summed_ws), 1)

        # Check bins have been summed
        self.assertAlmostEqual(summed_ws[0].readY(0)[bin_index], (first_run_bin_value + second_run_bin_value))
        mantid.DeleteWorkspace(summed_ws[0])

    def test_load_current_normalised_ws_respects_ext(self):
        run_number = "102"
        file_ext_one = ".s1"
        file_ext_two = ".s2"

        bin_index = 5

        result_ext_one = 1.25270032
        result_ext_two = 1.15126361

        # Check that it respects the ext flag - try the first extension of this name
        returned_ws_one = common.load_current_normalised_ws_list(instrument=ISISPowderMockInst(file_ext=file_ext_one),
                                                                 run_number_string=run_number)
        # Have to store result and delete the ws as they share the same name so will overwrite
        result_ws_one = returned_ws_one[0].readY(0)[bin_index]
        mantid.DeleteWorkspace(returned_ws_one[0])

        returned_ws_two = common.load_current_normalised_ws_list(instrument=ISISPowderMockInst(file_ext=file_ext_two),
                                                                 run_number_string=run_number)
        result_ws_two = returned_ws_two[0].readY(0)[bin_index]
        mantid.DeleteWorkspace(returned_ws_two[0])

        # Ensure it loaded two different workspaces
        self.assertAlmostEqual(result_ws_one, result_ext_one)

        # If this next line fails it means it loaded the .s1 file INSTEAD of the .s2 file
        self.assertAlmostEqual(result_ws_two, result_ext_two)
        self.assertNotAlmostEqual(result_ext_one, result_ext_two)

    def test_rebin_bin_boundary_defaults(self):
        ws = mantid.CreateSampleWorkspace(OutputWorkspace='test_rebin_bin_boundary_default',
                                          Function='Flat background', NumBanks=1, BankPixelWidth=1, XMax=10, BinWidth=1)
        new_bin_width = 0.5
        # Originally had bins at 1 unit each. So binning of 0.5 should give us 2n bins back
        original_number_bins = ws.blocksize()
        original_first_x_val = ws.readX(0)[0]
        original_last_x_val = ws.readX(0)[-1]

        expected_bins = original_number_bins * 2

        ws = common.rebin_workspace(workspace=ws, new_bin_width=new_bin_width)
        self.assertEqual(ws.blocksize(), expected_bins)

        # Check bin boundaries were preserved
        self.assertEqual(ws.readX(0)[0], original_first_x_val)
        self.assertEqual(ws.readX(0)[-1], original_last_x_val)

        mantid.DeleteWorkspace(ws)

    def test_rebin_bin_boundary_specified(self):
        ws = mantid.CreateSampleWorkspace(OutputWorkspace='test_rebin_bin_boundary_specified',
                                          Function='Flat background', NumBanks=1, BankPixelWidth=1, XMax=10, BinWidth=1)
        # Originally we had 10 bins from 0, 10. Resize from 0, 0.5, 5 so we should have the same number of output
        # bins with different boundaries
        new_bin_width = 0.5
        original_number_bins = ws.blocksize()

        expected_start_x = 1
        expected_end_x = 6

        ws = common.rebin_workspace(workspace=ws, new_bin_width=new_bin_width,
                                    start_x=expected_start_x, end_x=expected_end_x)

        # Check number of bins is the same as we halved the bin width and interval so we should have n bins
        self.assertEqual(ws.blocksize(), original_number_bins)

        # Check bin boundaries were changed
        self.assertEqual(ws.readX(0)[0], expected_start_x)
        self.assertEqual(ws.readX(0)[-1], expected_end_x)

        mantid.DeleteWorkspace(ws)

    def test_rebin_workspace_list_defaults(self):
        new_bin_width = 0.5
        number_of_ws = 10

        ws_bin_widths = [new_bin_width] * number_of_ws
        ws_list = []
        for i in range(number_of_ws):
            out_name = "test_rebin_workspace_list_defaults_" + str(i)
            ws_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, Function='Flat background',
                                                        NumBanks=1, BankPixelWidth=1, XMax=10, BinWidth=1))
        # What if the item passed in is not a list
        err_msg_not_list = "was not a list"
        with self.assertRaisesRegex(RuntimeError, err_msg_not_list):
            common.rebin_workspace_list(workspace_list=ws_list, bin_width_list=None)

        with self.assertRaisesRegex(RuntimeError, err_msg_not_list):
            common.rebin_workspace_list(workspace_list=None, bin_width_list=[])

        # What about if the lists aren't the same length
        with self.assertRaisesRegex(ValueError, "does not match the number of banks"):
            incorrect_number_bin_widths = [1] * (number_of_ws - 1)
            common.rebin_workspace_list(workspace_list=ws_list, bin_width_list=incorrect_number_bin_widths)

        # Does it return all the workspaces as a list - another unit test checks the implementation
        output = common.rebin_workspace_list(workspace_list=ws_list, bin_width_list=ws_bin_widths)
        self.assertEqual(len(output), number_of_ws)

        for ws in output:
            mantid.DeleteWorkspace(ws)

    def test_rebin_workspace_list_x_start_end(self):
        new_start_x = 1
        new_end_x = 5
        new_bin_width = 0.5
        number_of_ws = 10

        ws_bin_widths = [new_bin_width] * number_of_ws
        start_x_list = [new_start_x] * number_of_ws
        end_x_list = [new_end_x] * number_of_ws

        ws_list = []
        for i in range(number_of_ws):
            out_name = "test_rebin_workspace_list_defaults_" + str(i)
            ws_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, Function='Flat background',
                                                        NumBanks=1, BankPixelWidth=1, XMax=10, BinWidth=1))

        # Are the lengths checked
        incorrect_length = [1] * (number_of_ws - 1)
        with self.assertRaisesRegex(ValueError, "The number of starting bin values"):
            common.rebin_workspace_list(workspace_list=ws_list, bin_width_list=ws_bin_widths,
                                        start_x_list=incorrect_length, end_x_list=end_x_list)
        with self.assertRaisesRegex(ValueError, "The number of ending bin values"):
            common.rebin_workspace_list(workspace_list=ws_list, bin_width_list=ws_bin_widths,
                                        start_x_list=start_x_list, end_x_list=incorrect_length)

        output_list = common.rebin_workspace_list(workspace_list=ws_list, bin_width_list=ws_bin_widths,
                                                  start_x_list=start_x_list, end_x_list=end_x_list)
        self.assertEqual(len(output_list), number_of_ws)
        for ws in output_list:
            self.assertEqual(ws.readX(0)[0], new_start_x)
            self.assertEqual(ws.readX(0)[-1], new_end_x)
            mantid.DeleteWorkspace(ws)

    def test_remove_intermediate_workspace(self):
        ws_list = []
        ws_names_list = []

        ws_single_name = "remove_intermediate_ws-single"
        ws_single = mantid.CreateSampleWorkspace(OutputWorkspace=ws_single_name, NumBanks=1, BankPixelWidth=1,
                                                 XMax=10, BinWidth=1)

        for i in range(0, 3):
            out_name = "remove_intermediate_ws_" + str(i)
            ws_names_list.append(out_name)
            ws_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, NumBanks=1, BankPixelWidth=1,
                                                        XMax=10, BinWidth=1))

        # Check single workspaces are removed
        self.assertEqual(True, mantid.mtd.doesExist(ws_single_name))
        common.remove_intermediate_workspace(ws_single)
        self.assertEqual(False, mantid.mtd.doesExist(ws_single_name))

        # Next check lists are handled
        for ws_name in ws_names_list:
            self.assertEqual(True, mantid.mtd.doesExist(ws_name))

        common.remove_intermediate_workspace(ws_list)

        for ws_name in ws_names_list:
            self.assertEqual(False, mantid.mtd.doesExist(ws_name))

    def test_run_normalise_by_current(self):
        initial_value = 17
        prtn_charge = '10.0'
        expected_value = initial_value / float(prtn_charge)

        # Create two workspaces
        ws = mantid.CreateWorkspace(DataX=0, DataY=initial_value)

        # Add Good Proton Charge Log
        mantid.AddSampleLog(Workspace=ws, LogName='gd_prtn_chrg', LogText=prtn_charge, LogType='Number')

        self.assertEqual(initial_value, ws.dataY(0)[0])
        common.run_normalise_by_current(ws)
        self.assertAlmostEqual(expected_value, ws.dataY(0)[0], delta=1e-8)

    def test_spline_workspaces(self):
        ws_list = []
        for i in range(1, 4):
            out_name = "test_spline_vanadium-" + str(i)
            ws_list.append(mantid.CreateSampleWorkspace(OutputWorkspace=out_name, NumBanks=1, BankPixelWidth=1,
                                                        XMax=100, BinWidth=1))

        splined_list = common.spline_workspaces(focused_vanadium_spectra=ws_list, num_splines=10)
        for ws in splined_list:
            self.assertAlmostEqual(ws.dataY(0)[25], 0.28576649, delta=1e-8)
            self.assertAlmostEqual(ws.dataY(0)[50], 0.37745918, delta=1e-8)
            self.assertAlmostEqual(ws.dataY(0)[75], 0.28133096, delta=1e-8)

        for input_ws, splined_ws in zip(ws_list, splined_list):
            mantid.DeleteWorkspace(input_ws)
            mantid.DeleteWorkspace(splined_ws)

    def test_generate_summed_runs_scale_factor(self):
        sample_empty_number = "100"
        ws_file_name = "POL" + sample_empty_number
        original_ws = mantid.Load(ws_file_name)
        original_y = original_ws.readY(0)
        scale_factor = 0.75

        scaled_ws = common.generate_summed_runs(empty_sample_ws_string=sample_empty_number, instrument=ISISPowderMockInst(),
                                                scale_factor=scale_factor)
        scaled_y_values = scaled_ws.readY(0)
        self.assertAlmostEqual(scaled_y_values[2], original_y[2]*scale_factor)
        self.assertAlmostEqual(scaled_y_values[4], original_y[4]*scale_factor)
        self.assertAlmostEqual(scaled_y_values[7], original_y[7]*scale_factor)

        mantid.DeleteWorkspace(original_ws)
        mantid.DeleteWorkspace(scaled_ws)

    def test_subtract_summed_runs(self):
        # Load a vanadium workspace for this test
        sample_empty_number = "100"
        ws_file_name = "POL" + sample_empty_number
        original_ws = mantid.Load(ws_file_name)
        # add a current to ensure subtract_summed_runs gets past initial check
        mantid.AddSampleLog(Workspace=original_ws, LogName='gd_prtn_chrg', LogText="10.0", LogType='Number')
        no_scale_ws = mantid.CloneWorkspace(InputWorkspace=original_ws)
        empty_ws = mantid.CloneWorkspace(InputWorkspace=original_ws)
        empty_ws = empty_ws * 0.3

        returned_ws = common.subtract_summed_runs(ws_to_correct=no_scale_ws,
                                                  empty_sample=empty_ws)
        y_values = returned_ws.readY(0)
        original_y_values = original_ws.readY(0)
        for i in range(returned_ws.blocksize()):
            self.assertAlmostEqual(y_values[i], original_y_values[i]*0.7)

        mantid.DeleteWorkspace(no_scale_ws)
        mantid.DeleteWorkspace(empty_ws)
        mantid.DeleteWorkspace(returned_ws)

    def test_subtract_summed_runs_no_current(self):
        # Load a vanadium workspace for this test
        sample_empty_number = "100"
        ws_file_name = "POL" + sample_empty_number
        original_ws = mantid.Load(ws_file_name)

        ws_to_correct = mantid.CloneWorkspace(InputWorkspace=original_ws)
        empty_ws = mantid.CloneWorkspace(InputWorkspace=original_ws)
        empty_ws = empty_ws * 0.3

        returned_ws = common.subtract_summed_runs(ws_to_correct=ws_to_correct,
                                                  empty_sample=empty_ws)
        y_values = returned_ws.readY(0)
        original_y_values = original_ws.readY(0)
        # subtraction should be skipped leaving original values
        for i in range(returned_ws.blocksize()):
            self.assertAlmostEqual(y_values[i], original_y_values[i])

        mantid.DeleteWorkspace(ws_to_correct)
        mantid.DeleteWorkspace(empty_ws)
        mantid.DeleteWorkspace(returned_ws)

    def test_subtract_summed_runs_throw_on_tof_mismatch(self):
        # Create a sample workspace which will have mismatched TOF range
        sample_ws = mantid.CreateSampleWorkspace()
        mantid.AddSampleLog(Workspace=sample_ws, LogName='gd_prtn_chrg', LogText="10.0", LogType='Number')
        ws_file_name = "POL100" # Load POL100
        empty_ws = mantid.Load(ws_file_name)

        # This should throw as the TOF ranges do not match
        with self.assertRaisesRegex(ValueError, "specified for this file do not have matching binning. Do the "):
            common.subtract_summed_runs(ws_to_correct=sample_ws,
                                        empty_sample=empty_ws)

        mantid.DeleteWorkspace(sample_ws)


class ISISPowderMockInst(object):
    def __init__(self, file_ext=None):
        self._file_ext = file_ext
        self._inst_prefix = "MOCK"

    @staticmethod
    def _get_input_batching_mode(**_):
        # By default return multiple files as it makes something going wrong easier to spot
        return common_enums.INPUT_BATCHING.Individual

    def _get_run_details(self, **_):
        return ISISPowderMockRunDetails(file_ext=self._file_ext)

    @staticmethod
    def _generate_input_file_name(run_number, file_ext=""):
        # Mantid will automatically convert this into either POL or POLARIS
        return "POL" + str(run_number) + file_ext

    @staticmethod
    def _normalise_ws_current(ws_to_correct, **_):
        return ws_to_correct

    def mask_prompt_pulses_if_necessary(self, _):
        pass


class ISISPowderMockRunDetails(object):
    def __init__(self, file_ext):
        self.file_extension = file_ext


if __name__ == "__main__":
    unittest.main()
