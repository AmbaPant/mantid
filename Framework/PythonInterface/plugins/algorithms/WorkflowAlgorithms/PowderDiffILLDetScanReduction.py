from __future__ import (absolute_import, division, print_function)

from mantid.kernel import CompositeValidator, Direction, FloatArrayLengthValidator, FloatArrayOrderedPairsValidator, \
    FloatArrayProperty, StringListValidator
from mantid.api import PythonAlgorithm, MultipleFileProperty, Progress, WorkspaceGroupProperty
from mantid.simpleapi import *


class PowderDiffILLDetScanReduction(PythonAlgorithm):
    _progress = None

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction"

    def summary(self):
        return 'Performs powder diffraction data reduction for D2B and D20 (when doing a detector scan).'

    def name(self):
        return "PowderDiffILLDetScanReduction"

    def validateInputs(self):
        issues = dict()

        if not (self.getProperty("Output2DTubes").value or
                self.getProperty("Output2D").value or
                self.getProperty("Output1D").value):
            issues['Output2DTubes'] = 'No output chosen'
            issues['Output2D'] = 'No output chosen'
            issues['Output1D'] = 'No output chosen'

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='Monitor',
                             validator=StringListValidator(['None', 'Monitor']),
                             doc='Normalise to monitor, or skip normalisation.')

        self.declareProperty(name='UseCalibratedData',
                             defaultValue=True,
                             doc='Whether or not to use the calibrated data in the NeXus files.')

        self.declareProperty(name='Output2DTubes',
                             defaultValue=False,
                             doc='Output a 2D workspace of height along tube against tube scattering angle.')

        self.declareProperty(name='Output2D',
                             defaultValue=False,
                             doc='Output a 2D workspace of height along tube against the real scattering angle.')

        self.declareProperty(name='Output1D',
                             defaultValue=True,
                             doc='Output a 1D workspace with counts against scattering angle.')

        self.declareProperty(FloatArrayProperty(name='HeightRange', values=[],
                                                validator=CompositeValidator([FloatArrayOrderedPairsValidator(),
                                                                              FloatArrayLengthValidator(0, 2)])),
                             doc='A pair of values, comma separated, to give the minimum and maximum height range (in m). If not specified '
                                 'the full height range is used.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='Output workspace containing the reduced data.')

    def PyExec(self):
        input_workspaces = self._load()
        instrument_name = input_workspaces[0].getInstrument().getName()
        supported_instruments = ['D2B', 'D20']
        if instrument_name not in supported_instruments:
            self.log.warning('Running for unsupported instrument, use with caution. Supported instruments are: '
                             + str(supported_instruments))

        self._progress.report('Normalising to monitor')
        if self.getPropertyValue('NormaliseTo') == 'Monitor':
            input_workspaces = NormaliseToMonitor(InputWorkspace=input_workspaces, MonitorID=0)

        height_range = ''
        height_range_prop = self.getProperty('HeightRange').value
        if (len(height_range_prop) == 2):
            height_range = ','.join(height_range_prop)

        output_workspaces = []
        output_workspace_name = self.getPropertyValue('OutputWorkspace')

        self._progress.report('Doing Output2DTubes Option')
        if self.getProperty('Output2DTubes').value:
            output2DTubes = SumOverlappingTubes(InputWorkspaces=input_workspaces,
                                                OutputType='2DTubes',
                                                HeightAxis=height_range,
                                                OutputWorkspace=output_workspace_name + '_2DTubes')
            output_workspaces.append(output2DTubes)

        self._progress.report('Doing Output2D Option')
        if self.getProperty('Output2D').value:
            output2D = SumOverlappingTubes(InputWorkspaces=input_workspaces,
                                           OutputType='2D',
                                           CropNegativeScatteringAngles=True,
                                           HeightAxis=height_range,
                                           OutputWorkspace = output_workspace_name + '_2D')
            output_workspaces.append(output2D)

        self._progress.report('Doing Output1D Option')
        if self.getProperty('Output1D').value:
            output1D = SumOverlappingTubes(InputWorkspaces=input_workspaces,
                                           OutputType='1D',
                                           CropNegativeScatteringAngles=True,
                                           HeightAxis=height_range,
                                           OutputWorkspace=output_workspace_name + '_1D')
            output_workspaces.append(output1D)
        DeleteWorkspace('input_workspaces')

        self._progress.report('Finishing up...')

        grouped_output_workspace = GroupWorkspaces(output_workspaces)
        RenameWorkspace(InputWorkspace=grouped_output_workspace, OutputWorkspace=output_workspace_name)
        self.setProperty('OutputWorkspace', output_workspace_name)

    def _load(self):
        """
            Loads the list of runs
            If sum is requested, MergeRuns is called
            @return : the list of the loaded ws names
        """
        runs = self.getPropertyValue('Run')
        to_group = []
        self._progress = Progress(self, start=0.0, end=1.0, nreports=runs.count(',') + 1 + runs.count('+') + 4)

        data_type = 'Raw'
        if self.getProperty('UseCalibratedData').value:
            data_type = 'Calibrated'

        for runs_list in runs.split(','):
            runs_sum = runs_list.split('+')
            if len(runs_sum) == 1:
                run = os.path.basename(runs_sum[0]).split('.')[0]
                self._progress.report('Loading run #' + run)
                LoadILLDiffraction(Filename=runs_sum[0], OutputWorkspace=run, DataType=data_type)
                to_group.append(run)
            else:
                for i, run in enumerate(runs_sum):
                    runnumber = os.path.basename(run).split('.')[0]
                    output_ws_name = 'merged_runs'
                    if i == 0:
                        self._progress.report('Loading run #' + run)
                        LoadILLDiffraction(Filename=run, OutputWorkspace=output_ws_name, DataType=data_type)
                        to_group.append(output_ws_name)
                    else:
                        run = runnumber
                        self._progress.report('Loading run #' + run)
                        LoadILLDiffraction(Filename=run, OutputWorkspace=run, DataType=data_type)
                        MergeRuns(InputWorkspaces=[output_ws_name, run], OutputWorkspace=output_ws_name)
                        DeleteWorkspace(Workspace=run)

        return GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace='input_workspaces')


# Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffILLDetScanReduction)
