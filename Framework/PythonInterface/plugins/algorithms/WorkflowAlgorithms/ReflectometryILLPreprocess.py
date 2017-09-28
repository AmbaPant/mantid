# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, FileAction, ITableWorkspaceProperty, MatrixWorkspaceProperty,
                        MultipleFileProperty, PropertyMode)
from mantid.kernel import (Direction, IntBoundedValidator, Property, StringListValidator)
from mantid.simpleapi import (CalculatePolynomialBackground, CloneWorkspace, ConvertUnits, CreateSingleValuedWorkspace, CropWorkspace,
                              Divide, ExtractMonitors, GroupDetectors, Integration, LoadILLReflectometry, MergeRuns, Minus, mtd,
                              NormaliseToMonitor, Plus, Scale, SumSpectra, Transpose)
import numpy
import os.path
import ReflectometryILL_common as common


class Prop:
    BEAM_POS = 'BeamPosition'
    BKG_METHOD = 'FlatBackground'
    DIRECT_BEAM_POS = 'DirectBeamPosition'
    FLUX_NORM_METHOD = 'FluxNormalisation'
    FOREGROUND_CENTRE = 'ForegroundCentre'
    FOREGROUND_HALF_WIDTH = 'ForegroundHalfWidth'
    INPUT_WS = 'InputWorkspace'
    INSTRUMENT_BKG = 'InstrumentBackground'
    CLEANUP = 'Cleanup'
    LOWER_BKG_OFFSET = 'LowerBackgroundOffset'
    LOWER_BKG_WIDTH = 'LowerBackgroundWidth'
    OUTPUT_WS = 'OutputWorkspace'
    OUTPUT_BEAM_POS = 'OutputBeamPosition'
    RUN = 'Run'
    SLIT_NORM = 'SlitNormalisation'
    SUBALG_LOGGING = 'SubalgorithmLogging'
    SUM_OUTPUT = 'SumOutput'
    UPPER_BKG_OFFSET = 'UpperBackgroundOffset'
    UPPER_BKG_WIDTH = 'UpperBackgroundWidth'


class BkgMethod:
    CONSTANT = ' Background Constant Fit'
    LINEAR = 'Background Linear Fit'
    OFF = 'Background OFF'


class FluxNormMethod:
    MONITOR = 'Normalise To Monitor'
    TIME = 'Normalise To Time'
    OFF = 'Normalisation OFF'


class SubalgLogging:
    OFF = 'Logging OFF'
    ON = 'Logging ON'


class Summation:
    COHERENT = 'Coherent'
    INCOHERENT = 'Incoherent'
    OFF = 'Summation OFF'


class SlitNorm:
    OFF = 'Slit Normalisation OFF'
    ON = 'Slit Normalisation ON'


def normalisationMonitorWorkspaceIndex(ws):
    """Return the spectrum number of the monitor used for normalisation."""
    paramName = 'default-incident-monitor-spectrum'
    instr = ws.getInstrument()
    if not instr.hasParameter(paramName):
        raise RuntimeError('Parameter ' + paramName + ' is missing from the IPF.')
    n = instr.getIntParameter(paramName)[0]
    return ws.getIndexFromSpectrumNumber(n)


class ReflectometryILLPreprocess(DataProcessorAlgorithm):

    def category(self):
        """Return the categories of the algrithm."""
        return 'ILL\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryILLPreprocess'

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanupMode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = common.WSCleanup(cleanupMode, self._subalgLogging)
        self._names = common.WSNameSource('ReflectometryILLPreprocess', cleanupMode)

        ws, beamPosWS = self._inputWS()

        self._outputBeamPosition(beamPosWS)

        ws, monWS = self._extractMonitors(ws)

        # TODO Add calibration to water measurement.
        self.log().warning('Skipping water calibration as it is not yet implemented.')

        ws = self._normaliseToSlits(ws)

        ws = self._normaliseToFlux(ws, monWS)
        self._cleanup.cleanup(monWS)

        ws = self._subtractInstrumentBkg(ws)

        ws = self._subtractFlatBkg(ws, beamPosWS)

        ws = self._sumForeground(ws, beamPosWS)
        self._cleanup.cleanup(beamPosWS)

        ws = self._convertToWavelength(ws)

        self._finalize(ws)

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        nonnegativeInt = IntBoundedValidator(lower=0)
        positiveInt = IntBoundedValidator(lower=1)
        self.declareProperty(MultipleFileProperty(Prop.RUN,
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='A list of input numors/files.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.INPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='An input workspace if no Run is specified.')
        self.declareProperty(ITableWorkspaceProperty(Prop.BEAM_POS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='A beam position table corresponding to InputWorkspace.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.OUTPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Output),
                             doc='The preprocessed output workspace')
        self.declareProperty(Prop.SUBALG_LOGGING,
                             defaultValue=SubalgLogging.OFF,
                             validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
                             doc='Enable or disalbe child algorithm logging.')
        self.declareProperty(Prop.CLEANUP,
                             defaultValue=common.WSCleanup.ON,
                             validator=StringListValidator([common.WSCleanup.ON, common.WSCleanup.OFF]),
                             doc='Enable or disable intermediate workspace cleanup.')
        self.declareProperty(Prop.SUM_OUTPUT,
                             defaultValue=Summation.COHERENT,
                             validator=StringListValidator([Summation.COHERENT, Summation.INCOHERENT, Summation.OFF]),
                             doc='Foreground summation method.')
        self.declareProperty(ITableWorkspaceProperty(Prop.DIRECT_BEAM_POS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='A beam position table from a direct beam measurement.')
        self.declareProperty(Prop.SLIT_NORM,
                             defaultValue=SlitNorm.ON,
                             validator=StringListValidator([SlitNorm.ON, SlitNorm.OFF]),
                             doc='Enable or disable slit normalisation.')
        self.declareProperty(Prop.FLUX_NORM_METHOD,
                             defaultValue=FluxNormMethod.MONITOR,
                             validator=StringListValidator([FluxNormMethod.MONITOR, FluxNormMethod.TIME, FluxNormMethod.OFF]),
                             doc='Neutron flux normalisation method.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.INSTRUMENT_BKG,
                                                     defaultValue='',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc='A workspace containing the instrument background measurement.')
        self.declareProperty(Prop.BKG_METHOD,
                             defaultValue=BkgMethod.CONSTANT,
                             validator=StringListValidator([BkgMethod.CONSTANT, BkgMethod.LINEAR, BkgMethod.OFF]),
                             doc='Flat background calculation method for background subtraction.')
        self.declareProperty(Prop.LOWER_BKG_OFFSET,
                             defaultValue=7,
                             validator=nonnegativeInt,
                             doc='Distance of flat background region towards smaller detector angles from the foreground centre, ' +
                                 'in pixels.')
        self.declareProperty(Prop.LOWER_BKG_WIDTH,
                             defaultValue=5,
                             validator=nonnegativeInt,
                             doc='Width of flat background region towards smaller detector angles from the foreground centre, in pixels.')
        self.declareProperty(Prop.UPPER_BKG_OFFSET,
                             defaultValue=7,
                             validator=nonnegativeInt,
                             doc='Distance of flat background region towards larger detector angles from the foreground centre, in pixels.')
        self.declareProperty(Prop.UPPER_BKG_WIDTH,
                             defaultValue=5,
                             validator=nonnegativeInt,
                             doc='Width of flat background region towards larger detector angles from the foreground centre, in pixels.')
        self.declareProperty(Prop.FOREGROUND_CENTRE,
                             defaultValue=Property.EMPTY_INT,
                             validator=positiveInt,
                             doc='Spectrum number of the foreground centre pixel.')
        self.declareProperty(Prop.FOREGROUND_HALF_WIDTH,
                             defaultValue=Property.EMPTY_INT,
                             validator=nonnegativeInt,
                             doc='Number of pixels to include to the foreground region on either side of the centre pixel.')
        self.declareProperty(ITableWorkspaceProperty(Prop.OUTPUT_BEAM_POS,
                                                     defaultValue='',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='Output the beam position table.')

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        # TODO check that either RUN or INPUT_WS is given.
        if self.getProperty(Prop.BKG_METHOD).value != BkgMethod.OFF:
            if self.getProperty(Prop.LOWER_BKG_WIDTH).value == 0 and self.getProperty(Prop.UPPER_BKG_WIDTH).value == 0:
                issues[Prop.BKG_METHOD] = 'Cannot calculate flat background if both upper and lower background widths are zero.'
        return issues

    def _convertToWavelength(self, ws):
        """Convert the X units of ws to wavelength."""
        # TODO ws may already be in wavelength. In that case this step can be omitted.
        wavelengthWSName = self._names.withSuffix('in_wavelength')
        wavelengthWS = ConvertUnits(
            InputWorkspace=ws,
            OutputWorkspace=wavelengthWSName,
            Target='Wavelength',
            EMode='Elastic',
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return wavelengthWS

    def _extractMonitors(self, ws):
        """Extract monitor spectra from ws to another workspace."""
        detWSName = self._names.withSuffix('detectors')
        monWSName = self._names.withSuffix('monitors')
        ExtractMonitors(InputWorkspace=ws,
                        DetectorWorkspace=detWSName,
                        MonitorWorkspace=monWSName,
                        EnableLogging=self._subalgLogging)
        if mtd.doesExist(detWSName)  is None:
            raise RuntimeError('No detectors in the input data.')
        detWS = mtd[detWSName]
        monWS = mtd[monWSName] if mtd.doesExist(monWSName) else None
        self._cleanup.cleanup(ws)
        return (detWS, monWS)

    def _finalize(self, ws):
        """Set OutputWorkspace to ws and clean up."""
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _flatBkgRanges(self, ws, peakPosWS):
        """Return ranges for flat background fitting."""
        peakPos = self._foregroundCentre(ws, peakPosWS)
        peakPos = ws.getSpectrum(peakPos).getSpectrumNo()
        peakHalfWidth = self.getProperty(Prop.FOREGROUND_HALF_WIDTH).value
        if peakHalfWidth == Property.EMPTY_INT:
            peakHalfWidth = 0
        lowerOffset = self.getProperty(Prop.LOWER_BKG_OFFSET).value
        lowerWidth = self.getProperty(Prop.LOWER_BKG_WIDTH).value
        lowerStartIndex = peakPos + peakHalfWidth + lowerOffset
        lowerEndIndex = lowerStartIndex + lowerWidth
        lowerRange = [lowerStartIndex + 0.5, lowerEndIndex + 0.5]
        upperOffset = self.getProperty(Prop.UPPER_BKG_OFFSET).value
        upperWidth = self.getProperty(Prop.UPPER_BKG_WIDTH).value
        upperEndIndex = peakPos - peakHalfWidth - upperOffset
        upperStartIndex = upperEndIndex - upperWidth
        upperRange = [upperStartIndex - 0.5, upperEndIndex - 0.5]
        return upperRange + lowerRange

    def _foregroundCentre(self, ws, beamPosWS):
        """Return the detector id of the foreground centre pixel."""
        if self.getProperty(Prop.FOREGROUND_CENTRE).isDefault:
            return int(numpy.rint(beamPosWS.cell('FittedPeakCentre', 0)))
        spectrumNo = self.getProperty(Prop.FOREGROUND_CENTRE).value
        return ws.getIndexFromSpectrumNumber(spectrumNo)

    def _groupForeground(self, ws, beamPosWS):
        """Group detectors in the foreground region."""
        if self.getProperty(Prop.FOREGROUND_HALF_WIDTH).isDefault:
            return ws
        hw = self.getProperty(Prop.FOREGROUND_HALF_WIDTH).value
        beamPos = self._foregroundCentre(ws, beamPosWS)
        groupIndices = [i for i in range(beamPos - hw, beamPos + hw + 1)]
        foregroundWSName = self._names.withSuffix('foreground_grouped')
        foregroundWS = GroupDetectors(InputWorkspace=ws,
                                      OutputWorkspace=foregroundWSName,
                                      WorkspaceIndexList=groupIndices,
                                      EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return foregroundWS

    def _inputWS(self):
        """Return a raw input workspace and beam position table as tuple."""
        inputFiles = self.getProperty(Prop.RUN).value
        if len(inputFiles) > 0:
            flattened = list()
            for f in inputFiles:
                # Flatten input files into a single list
                if isinstance(f, str):
                    flattened.append(f)
                else:
                    # f is a list; concatenate.
                    flattened += f
            dbPosWS = ''
            if not self.getProperty(Prop.DIRECT_BEAM_POS).isDefault:
                dbPosWS = self.getProperty(Prop.DIRECT_BEAM_POS).value
            filename = flattened.pop(0)
            numor = os.path.basename(filename).split('.')[0]
            firstWSName = self._names.withSuffix('raw-' + numor)
            beamPosWSName = self._names.withSuffix('beamPos-' + numor)
            LoadILLReflectometry(Filename=filename,
                                 OutputWorkspace=firstWSName,
                                 BeamPosition=dbPosWS,
                                 XUnit='TimeOfFlight',
                                 OutputBeamPosition=beamPosWSName,
                                 EnableLogging=self._subalgLogging)
            mergedWS = mtd[firstWSName]
            beamPosWS = mtd[beamPosWSName]
            self._cleanup.cleanupLater(beamPosWS)
            mergedWSName = self._names.withSuffix('merged')
            for i, filename in enumerate(flattened):
                numor = os.path.basename(filename).split('.')[0]
                rawWSName = self._names.withSuffix('raw-' + numor)
                LoadILLReflectometry(Filename=filename,
                                     OutputWorkspace=rawWSName,
                                     BeamPosition=dbPosWS,
                                     XUnit='TimeOfFlight',
                                     EnableLogging=self._subalgLogging)
                rawWS = mtd[rawWSName]
                mergedWS = MergeRuns(InputWorkspace=[mergedWS, rawWS],
                                     OutputWorkspace=mergedWSName,
                                     EnableLoggign=self._subalgLogging)
                if i == 0:
                    self._cleanup.cleanup(firstWSName)
                self._cleanup.cleanup(rawWS)
            return (mergedWS, beamPosWS)
        ws = self.getProperty(Prop.INPUT_WS).value
        self._cleanup.protect(ws)
        beamPosWS = self.getProperty(Prop.BEAM_POS).value
        self._cleanup.protect(beamPosWS)
        return (ws, beamPosWS)

    def _normaliseToFlux(self, detWS, monWS):
        """Normalise ws to monitor counts or counting time."""
        method = self.getProperty(Prop.FLUX_NORM_METHOD).value
        if method == FluxNormMethod.MONITOR:
            if monWS is None:
                raise RuntimeError('Cannot normalise to monitor data: no monitors in input data.')
            normalisedWSName = self._names.withSuffix('normalised_to_monitor')
            monIndex = normalisationMonitorWorkspaceIndex(monWS)
            monXs = monWS.readX(0)
            minX = monXs[0]
            maxX = monXs[-1]
            NormaliseToMonitor(InputWorkspace=detWS,
                               OutputWorkspace=normalisedWSName,
                               MonitorWorkspace=monWS,
                               MonitorWorkspaceIndex=monIndex,
                               IntegrationRangeMin=minX,
                               IntegrationRangeMax=maxX,
                               EnableLogging=self._subalgLogging)
            normalisedWS = mtd[normalisedWSName]
            self._cleanup.cleanup(detWS)
            return normalisedWS
        elif method == FluxNormMethod.TIME:
            t = detWS.run().getProperty('duration').value
            normalisedWSName = self._names.withSuffix('normalised_to_time')
            scaledWS = Scale(InputWorkspace=detWS,
                             OutputWorkspace=normalisedWSName,
                             Factor=1.0 / t,
                             EnableLogging=self._subalgLogging)
            self._cleanup.cleanup(detWS)
            return scaledWS
        return detWS

    def _normaliseToSlits(self, ws):
        """Normalise ws to slit opening."""
        if self.getProperty(Prop.SLIT_NORM).value == SlitNorm.OFF:
            return ws
        r = ws.run()
        slit2width = r.get('VirtualSlitAxis.s2w_actual_width')
        slit3width = r.get('VirtualSlitAxis.s3w_actual_width')
        if slit2width is None or slit3width is None:
            self.log().warning('Slit information not found in sample logs. Slit normalisation disabled.')
            return ws
        f = slit2width.value * slit3width.value
        normalisedWSName = self._names.withSuffix('normalised_to_slits')
        normalisedWS = Scale(InputWorkspace=ws,
                             OutputWorkspace=normalisedWSName,
                             Factor=1.0 / f,
                             EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return normalisedWS

    def _outputBeamPosition(self, ws):
        """Set ws as OUTPUT_BEAM_POS, if desired."""
        if not self.getProperty(Prop.OUTPUT_BEAM_POS).isDefault:
            self.setProperty(Prop.OUTPUT_BEAM_POS, ws)

    def _subtractFlatBkg(self, ws, peakPosWS):
        """Return a workspace where a flat background has been subtracted from ws."""
        method = self.getProperty(Prop.BKG_METHOD).value
        if method == BkgMethod.OFF:
            return ws

        clonedWSName = self._names.withSuffix('cloned_for_flat_bkg')
        clonedWS = CloneWorkspace(InputWorkspace=ws,
                                  OutputWorkspace=clonedWSName,
                                  EnableLogging=self._subalgLogging)
        transposedWSName = self._names.withSuffix('transposed_clone')
        transposedWS = Transpose(InputWorkspace=clonedWS,
                                 OutputWorkspace=transposedWSName,
                                 EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(clonedWS)
        ranges = self._flatBkgRanges(ws, peakPosWS)
        polynomialDegree = 0 if self.getProperty(Prop.BKG_METHOD).value == BkgMethod.CONSTANT else 1
        transposedBkgWSName = self._names.withSuffix('transposed_flat_background')
        transposedBkgWS = CalculatePolynomialBackground(InputWorkspace=transposedWS,
                                                        OutputWorkspace=transposedBkgWSName,
                                                        Degree=polynomialDegree,
                                                        XRanges=ranges,
                                                        EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(transposedWS)
        bkgWSName = self._names.withSuffix('flat_background')
        bkgWS = Transpose(InputWorkspace=transposedBkgWS,
                          OutputWorkspace=bkgWSName,
                          EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(transposedBkgWS)
        subtractedWSName = self._names.withSuffix('flat_background_subtracted')
        subtractedWS = Minus(LHSWorkspace=ws,
                             RHSWorkspace=bkgWS,
                             OutputWorkspace=subtractedWSName,
                             EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(bkgWS)
        return subtractedWS

    def _subtractInstrumentBkg(self, ws):
        """Return a workspace where instrument background has been subtracted from ws."""
        if self.getProperty(Prop.INSTRUMENT_BKG).isDefault:
            return ws
        bkgWS = self.getProperty(Prop.INSTRUMENT_BKG).value
        subtractedWSName = self._names.withSuffix('instr_bkg_subtracted')
        subtractedWS = Minus(LHSWorkspace=ws,
                             RHSWorkspace=bkgWS,
                             OutputWorkspace=subtractedWSName,
                             EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return subtractedWS

    def _sumForeground(self, ws, peakPosWS):
        """Select and apply foreground summation."""
        method = self.getProperty(Prop.SUM_OUTPUT).value
        if method == Summation.OFF:
            return ws
        elif method == Summation.COHERENT:
            return self._groupForeground(ws, peakPosWS)
        else:
            raise RuntimeError('Selected output summation method is not implemented.')


AlgorithmFactory.subscribe(ReflectometryILLPreprocess)
