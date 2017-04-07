# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, InstrumentValidator,
                        ITableWorkspaceProperty, MatrixWorkspaceProperty, Progress, PropertyMode, WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direct, Direction, FloatBoundedValidator, IntBoundedValidator, IntArrayBoundedValidator,
                           IntMandatoryValidator, Property, StringListValidator, UnitConversion)
from mantid.simpleapi import (AddSampleLog, CalculateFlatBackground, CloneWorkspace, CorrectTOFAxis, CreateEPP, CreateSingleValuedWorkspace,
                              CreateWorkspace, CropWorkspace, Divide, ExtractMonitors, FindEPP, GetEiMonDet, Load, MergeRuns, Minus,
                              NormaliseToMonitor, Scale)
import numpy


def _applyIncidentEnergyCalibration(ws, wsType, eiWS, wsNames, report,
                                    algorithmLogging):
    """Update incident energy and wavelength in the sample logs."""
    originalEnergy = ws.getRun().getLogData('Ei').value
    originalWavelength = ws.getRun().getLogData('wavelength').value
    energy = eiWS.readY(0)[0]
    wavelength = UnitConversion.run('Energy', 'Wavelength', energy, 0, 0, 0, Direct, 5)
    if wsType == common.WS_CONTENT_DETS:
        calibratedWSName = wsNames.withSuffix('incident_energy_calibrated_detectors')
    elif wsType == common.WS_CONTENT_MONS:
        calibratedWSName = wsNames.withSuffix('incident_energy_calibrated_monitors')
    else:
        raise RuntimeError('Unknown workspace content type')
    calibratedWS = CloneWorkspace(InputWorkspace=ws,
                                  OutputWorkspace=calibratedWSName,
                                  EnableLogging=algorithmLogging)
    AddSampleLog(Workspace=calibratedWS,
                 LogName='Ei',
                 LogText=str(energy),
                 LogType='Number',
                 NumberType='Double',
                 LogUnit='meV',
                 EnableLogging=algorithmLogging)
    AddSampleLog(Workspace=calibratedWS,
                 Logname='wavelength',
                 LogText=str(wavelength),
                 LogType='Number',
                 NumberType='Double',
                 LogUnit='Ångström',
                 EnableLogging=algorithmLogging)
    report.notice("Applied Ei calibration to '" + str(ws) + "'.")
    report.notice('Original Ei: {} new Ei: {}.'.format(originalEnergy, energy))
    report.notice('Original wavelength: {} new wavelength {}.'.format(originalWavelength, wavelength))
    return calibratedWS


def _calculateEPP(ws, sigma, wsNames, algorithmLogging):
    eppWSName = wsNames.withSuffix('epp_detectors')
    eppWS = CreateEPP(InputWorkspace=ws,
                      OutputWorkspace=eppWSName,
                      Sigma=sigma,
                      EnableLogging=algorithmLogging)
    return eppWS


def _calibratedIncidentEnergy(detWorkspace, detEPPWorkspace, monWorkspace, monEPPWorkspace,
                              eiCalibrationMon, wsNames, log, algorithmLogging):
    """Return the calibrated incident energy."""
    instrument = detWorkspace.getInstrument().getName()
    eiWorkspace = None
    if instrument in ['IN4', 'IN6']:
        eiCalibrationDets = [i for i in range(detWorkspace.getNumberHistograms())]
        pulseInterval = detWorkspace.getRun().getLogData('pulse_interval').value
        energy = GetEiMonDet(DetectorWorkspace=detWorkspace,
                             DetectorEPPTable=detEPPWorkspace,
                             IndexType='Workspace Index',
                             Detectors=eiCalibrationDets,
                             MonitorWorkspace=monWorkspace,
                             MonitorEppTable=monEPPWorkspace,
                             Monitor=eiCalibrationMon,
                             PulseInterval=pulseInterval,
                             EnableLogging=algorithmLogging)
        eiWSName = wsNames.withSuffix('incident_energy')
        eiWorkspace = CreateSingleValuedWorkspace(OutputWorkspace=eiWSName,
                                                  DataValue=energy,
                                                  EnableLogging=algorithmLogging)
    else:
        log.error('Instrument ' + instrument + ' not supported for incident energy calibration')
    return eiWorkspace


def _createFlatBkg(ws, wsType, windowWidth, wsNames, algorithmLogging):
    """Return a flat background workspace."""
    if wsType == common.WS_CONTENT_DETS:
        bkgWSName = wsNames.withSuffix('flat_bkg_for_detectors')
    else:
        bkgWSName = wsNames.withSuffix('flat_bkg_for_monitors')
    bkgWS = CalculateFlatBackground(InputWorkspace=ws,
                                    OutputWorkspace=bkgWSName,
                                    Mode='Moving Average',
                                    OutputMode='Return Background',
                                    SkipMonitors=False,
                                    NullifyNegativeValues=False,
                                    AveragingWindowWidth=windowWidth,
                                    EnableLogging=algorithmLogging)
    firstBinStart = bkgWS.dataX(0)[0]
    firstBinEnd = bkgWS.dataX(0)[1]
    bkgWS = CropWorkspace(InputWorkspace=bkgWS,
                          OutputWorkspace=bkgWS,
                          XMin=firstBinStart,
                          XMax=firstBinEnd,
                          EnableLogging=algorithmLogging)
    return bkgWS


def _fitElasticChannel(ys, wsNames, wsCleanup, algorithmLogging):
    """Return index to the peak position of ys."""
    xs = numpy.array([i for i in range(len(ys))])
    l2SumWSName = wsNames.withSuffix('summed_detectors_at_l2')
    l2SumWS = CreateWorkspace(OutputWorkspace=l2SumWSName,
                              DataX=xs,
                              DataY=ys,
                              EnableLogging=algorithmLogging)
    fitWSName = wsNames.withSuffix('summed_detectors_at_l2_fit_results')
    fitWS = FindEPP(InputWorkspace=l2SumWS,
                    OutputWorkspace=fitWSName,
                    EnableLogging=algorithmLogging)
    peakCentre = float(fitWS.cell('PeakCentre', 0))
    wsCleanup.cleanup(l2SumWS)
    wsCleanup.cleanup(fitWS)
    return int(round(peakCentre))


def _fitEPP(ws, wsType, wsNames, algorithmLogging):
    """Return a fitted EPP table for a workspace."""
    if wsType == common.WS_CONTENT_DETS:
        eppWSName = wsNames.withSuffix('epp_detectors')
    else:
        eppWSName = wsNames.withSuffix('epp_monitors')
    eppWS = FindEPP(InputWorkspace=ws,
                    OutputWorkspace=eppWSName,
                    EnableLogging=algorithmLogging)
    return eppWS


def _loadFiles(inputFilename, wsNames, wsCleanup, algorithmLogging):
    """Load files specified by inputFilename, merging them into a single workspace."""
    # TODO Explore ways of loading data file-by-file and merging pairwise.
    #      Should save some memory (IN5!).
    rawWSName = wsNames.withSuffix('raw')
    rawWS = Load(Filename=inputFilename,
                 OutputWorkspace=rawWSName,
                 EnableLogging=algorithmLogging)
    mergedWSName = wsNames.withSuffix('merged')
    mergedWS = MergeRuns(InputWorkspaces=rawWS,
                         OutputWorkspace=mergedWSName,
                         EnableLogging=algorithmLogging)
    wsCleanup.cleanup(rawWS)
    return mergedWS


def _normalizeToMonitor(ws, monWS, monIndex, integrationBegin, integrationEnd,
                        wsNames, wsCleanup, algorithmLogging):
    """Normalize to monitor counts."""
    normalizedWSName = wsNames.withSuffix('normalized_to_monitor')
    normalizationFactorWsName = wsNames.withSuffix('normalization_factor_monitor')
    normalizedWS, normalizationFactorWS = NormaliseToMonitor(InputWorkspace=ws,
                                                             OutputWorkspace=normalizedWSName,
                                                             MonitorWorkspace=monWS,
                                                             MonitorWorkspaceIndex=monIndex,
                                                             IntegrationRangeMin=integrationBegin,
                                                             IntegrationRangeMax=integrationEnd,
                                                             NormFactorWS=normalizationFactorWsName,
                                                             EnableLogging=algorithmLogging)
    wsCleanup.cleanup(normalizationFactorWS)
    return normalizedWS


def _normalizeToTime(ws, wsNames, wsCleanup, algorithmLogging):
    """Normalize to the 'actual_time' sample log."""
    log = ws.run()
    if not log.hasProperty('duration'):
        if not log.hasProperty('actual_time'):
            raise RuntimeError("Cannot normalise to acquisition time: 'duration' missing from sample logs.")
        time = log.getProperty('actual_time').value
    else:
        time = log.getProperty('duration').value
    if time == 0:
        raise RuntimeError("Cannot normalise to acquisition time: time is zero.")
    if time < 0:
        raise RuntimeError("Cannot normalise to acquisition time: time is negative.")
    normalizedWSName = wsNames.withSuffix('normalized_to_time')
    normalizationFactorWsName = wsNames.withSuffix('normalization_factor_time')
    normalizationFactorWS = CreateSingleValuedWorkspace(OutputWorkspace=normalizationFactorWsName,
                                                        DataValue=time,
                                                        EnableLogging=algorithmLogging)
    normalizedWS = Divide(LHSWorkspace=ws,
                          RHSWorkspace=normalizationFactorWS,
                          OutputWorkspace=normalizedWSName,
                          EnableLogging=algorithmLogging)
    wsCleanup.cleanup(normalizationFactorWS)
    return normalizedWS


def _subtractFlatBkg(ws, wsType, bkgWorkspace, bkgScaling, wsNames,
                     wsCleanup, algorithmLogging):
    """Subtract a scaled flat background from a workspace."""
    if wsType == common.WS_CONTENT_DETS:
        subtractedWSName = wsNames.withSuffix('flat_bkg_subtracted_detectors')
        scaledBkgWSName = wsNames.withSuffix('flat_bkg_for_detectors_scaled')
    else:
        subtractedWSName = wsNames.withSuffix('flat_bkg_subtracted_monitors')
        scaledBkgWSName = wsNames.withSuffix('flat_bkg_for_monitors_scaled')
    Scale(InputWorkspace=bkgWorkspace,
          OutputWorkspace=scaledBkgWSName,
          Factor=bkgScaling,
          EnableLogging=algorithmLogging)
    subtractedWS = Minus(LHSWorkspace=ws,
                         RHSWorkspace=scaledBkgWSName,
                         OutputWorkspace=subtractedWSName,
                         EnableLogging=algorithmLogging)
    wsCleanup.cleanup(scaledBkgWSName)
    return subtractedWS


def _sumDetectorsAtDistance(ws, distance, tolerance):
    """Return a sum of the Y values of detectors at distance away from the sample."""
    histogramCount = ws.getNumberHistograms()
    ySums = numpy.zeros(ws.blocksize())
    detectorInfo = ws.detectorInfo()
    sample = ws.getInstrument().getSample()
    for i in range(histogramCount):
        det = ws.getDetector(i)
        sampleToDetector = sample.getDistance(det)
        if abs(distance - sampleToDetector) < tolerance:
            if detectorInfo.isMonitor(i) or detectorInfo.isMasked(i):
                continue
            ySums += ws.readY(i)
    return ySums


class DirectILLCollectData(DataProcessorAlgorithm):
    """A workflow algorithm for the initial sample, vanadium and empty container reductions."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return 'Workflow\\Inelastic'

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLCollectData'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'An initial step of the reduction workflow for the direct geometry TOF spectrometers at ILL.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Executes the data reduction workflow."""
        progress = Progress(self, 0.0, 1.0, 9)
        report = common.Report()
        subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        # The variables 'mainWS' and 'monWS shall hold the current main
        # data throughout the algorithm.

        # Get input workspace.
        progress.report('Loading inputs')
        mainWS = self._inputWS(wsNames, wsCleanup, subalgLogging)

        # Extract monitors to a separate workspace.
        progress.report('Extracting monitors')
        mainWS, monWS = self._separateMons(mainWS, wsNames, wsCleanup, subalgLogging)

        # Normalisation to monitor/time, if requested.
        progress.report('Normalising to monitor/time')
        monWS = self._flatBkgMon(monWS, wsNames, wsCleanup, subalgLogging)
        monEPPWS = self._createEPPWSMon(monWS, wsNames, wsCleanup, subalgLogging)
        mainWS = self._normalize(mainWS, monWS, monEPPWS, wsNames, wsCleanup, subalgLogging)

        # Time-independent background.
        progress.report('Calculating backgrounds')
        mainWS, bkgWS = self._flatBkgDet(mainWS, wsNames, wsCleanup, subalgLogging)
        wsCleanup.cleanupLater(bkgWS)

        # Find elastic peak positions.
        progress.report('Calculating EPPs')
        detEPPWS = self._createEPPWSDet(mainWS, wsNames, wsCleanup, subalgLogging)
        wsCleanup.cleanupLater(detEPPWS, monEPPWS)

        # Calibrate incident energy, if requested.
        progress.report('Calibrating incident energy')
        mainWS, monWS = self._calibrateEi(mainWS, detEPPWS, monWS, monEPPWS,
                                          wsNames, wsCleanup, report,
                                          subalgLogging)
        wsCleanup.cleanupLater(monWS)

        progress.report('Correcting TOF')
        mainWS = self._correctTOFAxis(mainWS, wsNames, wsCleanup, report, subalgLogging)

        # Find elastic peak positions.
        progress.report('Recalculating EPPs')
        detEPPWS = self._createEPPWSDet(mainWS, wsNames, wsCleanup, subalgLogging)
        wsCleanup.cleanupLater(detEPPWS, monEPPWS)

        self._finalize(mainWS, wsCleanup, report)
        progress.report('Done')

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        # Validators.
        mandatoryPositiveInt = CompositeValidator()
        mandatoryPositiveInt.add(IntMandatoryValidator())
        mandatoryPositiveInt.add(IntBoundedValidator(lower=0))
        positiveFloat = FloatBoundedValidator(lower=0)
        positiveInt = IntBoundedValidator(lower=0)
        positiveIntArray = IntArrayBoundedValidator()
        positiveIntArray.setLower(0)
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))

        # Properties.
        self.declareProperty(FileProperty(name=common.PROP_INPUT_FILE,
                                          defaultValue='',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']),
                             doc='Input file')
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            optional=PropertyMode.Optional,
            direction=Direction.Input),
            doc='Input workspace.')
        self.declareProperty(WorkspaceProperty(name=common.PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='The output of the algorithm.')
        self.declareProperty(name=common.PROP_CLEANUP_MODE,
                             defaultValue=common.CLEANUP_ON,
                             validator=StringListValidator([
                                 common.CLEANUP_ON,
                                 common.CLEANUP_OFF]),
                             direction=Direction.Input,
                             doc='What to do with intermediate workspaces.')
        self.declareProperty(name=common.PROP_SUBALG_LOGGING,
                             defaultValue=common.SUBALG_LOGGING_OFF,
                             validator=StringListValidator([
                                 common.SUBALG_LOGGING_OFF,
                                 common.SUBALG_LOGGING_ON]),
                             direction=Direction.Input,
                             doc='Enable or disable subalgorithms to ' +
                                 'print in the logs.')
        self.declareProperty(ITableWorkspaceProperty(
            name=common.PROP_EPP_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Table workspace containing results from the FindEPP ' +
                'algorithm.')
        self.declareProperty(name=common.PROP_EPP_METHOD,
                             defaultValue=common.EPP_METHOD_FIT,
                             validator=StringListValidator([
                                 common.EPP_METHOD_FIT,
                                 common.EPP_MEHTOD_CALCULATE]),
                             direction=Direction.Input,
                             doc='Method to create the EPP table for detectors (monitor is awlays fitted) if '
                                 + common.PROP_EPP_WS + ' is not given.')
        self.declareProperty(name=common.PROP_EPP_SIGMA,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Nominal sigma for the EPP table when ' + common.PROP_EPP_METHOD +
                                 ' is set to ' + common.EPP_MEHTOD_CALCULATE +
                                 ' (default: 10 times the first bin width).')
        self.declareProperty(name=common.PROP_ELASTIC_CHANNEL_MODE,
                             defaultValue=common.ELASTIC_CHANNEL_SAMPLE_LOG,
                             validator=StringListValidator([
                                 common.ELASTIC_CHANNEL_SAMPLE_LOG,
                                 common.ELASTIC_CHANNEL_FIT]),
                             direction=Direction.Input,
                             doc='How to acquire the elastic channel.')
        self.declareProperty(MatrixWorkspaceProperty(
                             name=common.PROP_ELASTIC_CHANNEL_WS,
                             defaultValue='',
                             direction=Direction.Input,
                             optional=PropertyMode.Optional),
                             doc='A single value workspace containing the elatic channel index. Overrides '
                                 + common.PROP_ELASTIC_CHANNEL_MODE + '.')
        self.declareProperty(name=common.PROP_MON_INDEX,
                             defaultValue=Property.EMPTY_INT,
                             validator=positiveInt,
                             direction=Direction.Input,
                             doc='Index of the incident monitor, if not specified in instrument parameters.')
        self.declareProperty(name=common.PROP_INCIDENT_ENERGY_CALIBRATION,
                             defaultValue=common.INCIDENT_ENERGY_CALIBRATION_ON,
                             validator=StringListValidator([
                                 common.INCIDENT_ENERGY_CALIBRATION_ON,
                                 common.INCIDENT_ENERGY_CALIBRATION_OFF]),
                             direction=Direction.Input,
                             doc='Enable or disable incident energy ' +
                                 'calibration on IN4 and IN6.')
        self.setPropertyGroup(common.PROP_INCIDENT_ENERGY_CALIBRATION, common.PROPGROUP_INCIDENT_ENERGY_CALIBRATION)
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INCIDENT_ENERGY_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='A single-valued workspace holding the calibrated ' +
                'incident energy.')
        self.setPropertyGroup(common.PROP_INCIDENT_ENERGY_WS, common.PROPGROUP_INCIDENT_ENERGY_CALIBRATION)
        self.declareProperty(name=common.PROP_FLAT_BKG_SCALING,
                             defaultValue=1.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Flat background scaling constant')
        self.setPropertyGroup(common.PROP_FLAT_BKG_SCALING, common.PROPGROUP_FLAT_BKG)
        self.declareProperty(name=common.PROP_FLAT_BKG_WINDOW,
                             defaultValue=30,
                             validator=mandatoryPositiveInt,
                             direction=Direction.Input,
                             doc='Running average window width (in bins) ' +
                                 'for flat background.')
        self.setPropertyGroup(common.PROP_FLAT_BKG_WINDOW, common.PROPGROUP_FLAT_BKG)
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_FLAT_BKG_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Workspace from which to get flat background data.')
        self.setPropertyGroup(common.PROP_FLAT_BKG_WS, common.PROPGROUP_FLAT_BKG)
        self.declareProperty(name=common.PROP_NORMALISATION,
                             defaultValue=common.NORM_METHOD_MON,
                             validator=StringListValidator([
                                 common.NORM_METHOD_MON,
                                 common.NORM_METHOD_TIME,
                                 common.NORM_METHOD_OFF]),
                             direction=Direction.Input,
                             doc='Normalisation method.')
        self.setPropertyGroup(common.PROP_NORMALISATION, common.PROPGROUP_MON_NORMALISATION)
        self.declareProperty(name=common.PROP_MON_PEAK_SIGMA_MULTIPLIER,
                             defaultValue=3.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc="Width of the monitor peak in multiples " +
                                 " of 'Sigma' in monitor's EPP table.")
        self.setPropertyGroup(common.PROP_MON_PEAK_SIGMA_MULTIPLIER, common.PROPGROUP_MON_NORMALISATION)
        # Rest of the output properties.
        self.declareProperty(WorkspaceProperty(
            name=common.PROP_OUTPUT_ELASTIC_CHANNEL_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for elastic channel index.')
        self.setPropertyGroup(common.PROP_OUTPUT_ELASTIC_CHANNEL_WS,
                              common.PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(ITableWorkspaceProperty(
            name=common.PROP_OUTPUT_DET_EPP_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for elastic peak positions.')
        self.setPropertyGroup(common.PROP_OUTPUT_DET_EPP_WS,
                              common.PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(WorkspaceProperty(
            name=common.PROP_OUTPUT_INCIDENT_ENERGY_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for calibrated inciden energy.')
        self.setPropertyGroup(common.PROP_OUTPUT_INCIDENT_ENERGY_WS,
                              common.PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(WorkspaceProperty(
            name=common.PROP_OUTPUT_FLAT_BKG_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for flat background.')
        self.setPropertyGroup(common.PROP_OUTPUT_FLAT_BKG_WS,
                              common.PROPGROUP_OPTIONAL_OUTPUT)

    def validateInputs(self):
        """Check for issues with user input."""
        issues = dict()

        fileGiven = not self.getProperty(common.PROP_INPUT_FILE).isDefault
        wsGiven = not self.getProperty(common.PROP_INPUT_WS).isDefault
        # Validate that an input exists
        if fileGiven == wsGiven:
            issues[common.PROP_INPUT_FILE] = \
                'Must give either an input file or an input workspace.'
        if not wsGiven and self.getProperty(common.PROP_INPUT_WS).value:
            issues[common.PROP_INPUT_WS] = 'Input workspace has to be in the ADS.'
        return issues

    def _calibrateEi(self, mainWS, detEPPWS, monWS, monEPPWS, wsNames, wsCleanup, report, subalgLogging):
        """Perform and apply incident energy calibration."""
        eiCalibration = self.getProperty(common.PROP_INCIDENT_ENERGY_CALIBRATION).value
        if eiCalibration == common.INCIDENT_ENERGY_CALIBRATION_ON:
            if self.getProperty(common.PROP_INCIDENT_ENERGY_WS).isDefault:
                monIndex = self._monitorIndex(monWS)
                eiCalibrationWS = _calibratedIncidentEnergy(mainWS, detEPPWS, monWS, monEPPWS, monIndex, wsNames,
                                                            self.log(), subalgLogging)
            else:
                eiCalibrationWS = self.getProperty(common.PROP_INCIDENT_ENERGY_WS).value
                wsCleanup.protect(eiCalibrationWS)
            if eiCalibrationWS:
                eiCalibratedDetWS = _applyIncidentEnergyCalibration(mainWS, common.WS_CONTENT_DETS, eiCalibrationWS, wsNames,
                                                                    report, subalgLogging)
                wsCleanup.cleanup(mainWS)
                mainWS = eiCalibratedDetWS
                eiCalibratedMonWS = _applyIncidentEnergyCalibration(monWS, common.WS_CONTENT_MONS, eiCalibrationWS, wsNames, report,
                                                                    subalgLogging)
                wsCleanup.cleanup(monWS)
                monWS = eiCalibratedMonWS
            if not self.getProperty(
                    common.PROP_OUTPUT_INCIDENT_ENERGY_WS).isDefault:
                self.setProperty(common.PROP_OUTPUT_INCIDENT_ENERGY_WS, eiCalibrationWS)
            wsCleanup.cleanup(eiCalibrationWS)
        return mainWS, monWS

    def _correctTOFAxis(self, mainWS, wsNames, wsCleanup, report, subalgLogging):
        """Adjust the TOF axis to get the elastic channel correct."""
        try:
            l2 = float(mainWS.getInstrument().getStringParameter('l2')[0])
        except IndexError:
            self.log().warning("No 'l2' instrument parameter defined. TOF axis will not be adjusted")
            return mainWS
        if not self.getProperty(common.PROP_ELASTIC_CHANNEL_WS).isDefault:
            indexWS = self.getProperty(common.PROP_ELASTIC_CHANNEL_WS).value
            index = int(indexWS.readY(0)[0])
        else:
            mode = self.getProperty(common.PROP_ELASTIC_CHANNEL_MODE).value
            if mode == common.ELASTIC_CHANNEL_SAMPLE_LOG:
                if not mainWS.run().hasProperty('Detector.elasticpeak'):
                    self.log().warning('No ' + common.PROP_ELASTIC_CHANNEL +
                                       ' given. TOF axis will not be adjusted.')
                    return mainWS
                index = mainWS.run().getLogData('Detector.elasticpeak').value
            else:
                ys = _sumDetectorsAtDistance(mainWS, l2, 1e-5)
                index = _fitElasticChannel(ys, wsNames, wsCleanup, subalgLogging)
        correctedWSName = wsNames.withSuffix('tof_axis_corrected')
        correctedWS = CorrectTOFAxis(InputWorkspace=mainWS,
                                     OutputWorkspace=correctedWSName,
                                     IndexType='Workspace Index',
                                     ElasticBinIndex=index,
                                     L2=l2,
                                     EnableLogging=subalgLogging)
        report.notice('Elastic channel index {0} was used for TOF axis adjustment.'.format(index))
        if not self.getProperty(common.PROP_OUTPUT_ELASTIC_CHANNEL_WS).isDefault:
            indexOutputWSName = wsNames.withSuffix('elastic_channel_output')
            indexOutputWS = CreateSingleValuedWorkspace(OutputWorkspace=indexOutputWSName,
                                                        DataValue=index,
                                                        EnableLogging=subalgLogging)
            self.setProperty(common.PROP_OUTPUT_ELASTIC_CHANNEL_WS, indexOutputWS)
            wsCleanup.cleanup(indexOutputWS)
        wsCleanup.cleanup(mainWS)
        return correctedWS

    def _createEPPWSDet(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Create an EPP table for a detector workspace."""
        eiCalibration = self.getProperty(common.PROP_INCIDENT_ENERGY_CALIBRATION).value
        noEiCalibration = eiCalibration == common.INCIDENT_ENERGY_CALIBRATION_OFF
        yesInputEi = not self.getProperty(common.PROP_INCIDENT_ENERGY_WS).isDefault
        noOutputEPP = self.getProperty(common.PROP_OUTPUT_DET_EPP_WS).isDefault
        if (noEiCalibration or yesInputEi) and noOutputEPP:
            # No epp table needed.
            return None
        detEPPInWS = self.getProperty(common.PROP_EPP_WS).value
        if not detEPPInWS:
            eppMethod = self.getProperty(common.PROP_EPP_METHOD).value
            if eppMethod == common.EPP_METHOD_FIT:
                detEPPWS = _fitEPP(mainWS, common.WS_CONTENT_DETS, wsNames, subalgLogging)
            else:
                sigma = self.getProperty(common.PROP_EPP_SIGMA).value
                if sigma == Property.EMPTY_DBL:
                    sigma = 10.0 * (mainWS.readX(0)[1] - mainWS.readX(0)[0])
                detEPPWS = _calculateEPP(mainWS, sigma, wsNames, subalgLogging)
        else:
            detEPPWS = detEPPInWS
            wsCleanup.protect(detEPPWS)
        if not self.getProperty(common.PROP_OUTPUT_DET_EPP_WS).isDefault:
            self.setProperty(common.PROP_OUTPUT_DET_EPP_WS,
                             detEPPWS)
        return detEPPWS

    def _createEPPWSMon(self, monWS, wsNames, wsCleanup, subalgLogging):
        """Create an EPP table for a monitor workspace."""
        monEPPWS = _fitEPP(monWS, common.WS_CONTENT_MONS, wsNames, subalgLogging)
        return monEPPWS

    def _finalize(self, outWS, wsCleanup, report):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        wsCleanup.cleanup(outWS)
        wsCleanup.finalCleanup()
        report.toLog(self.log())

    def _flatBkgDet(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Subtract flat background from a detector workspace."""
        bkgInWS = self.getProperty(common.PROP_FLAT_BKG_WS).value
        windowWidth = self.getProperty(common.PROP_FLAT_BKG_WINDOW).value
        if not bkgInWS:
            bkgWS = _createFlatBkg(mainWS, common.WS_CONTENT_DETS, windowWidth, wsNames, subalgLogging)
        else:
            bkgWS = bkgInWS
            wsCleanup.protect(bkgWS)
        if not self.getProperty(common.PROP_OUTPUT_FLAT_BKG_WS).isDefault:
            self.setProperty(common.PROP_OUTPUT_FLAT_BKG_WS, bkgWS)
        bkgScaling = self.getProperty(common.PROP_FLAT_BKG_SCALING).value
        bkgSubtractedWS = _subtractFlatBkg(mainWS, common.WS_CONTENT_DETS, bkgWS, bkgScaling, wsNames,
                                           wsCleanup, subalgLogging)
        wsCleanup.cleanup(mainWS)
        return bkgSubtractedWS, bkgWS

    def _flatBkgMon(self, monWS, wsNames, wsCleanup, subalgLogging):
        """Subtract flat background from a monitor workspace."""
        windowWidth = self.getProperty(common.PROP_FLAT_BKG_WINDOW).value
        monBkgWS = _createFlatBkg(monWS, common.WS_CONTENT_MONS, windowWidth, wsNames, subalgLogging)
        monBkgScaling = 1
        bkgSubtractedMonWS = _subtractFlatBkg(monWS, common.WS_CONTENT_MONS, monBkgWS, monBkgScaling, wsNames,
                                              wsCleanup, subalgLogging)
        wsCleanup.cleanup(monBkgWS)
        wsCleanup.cleanup(monWS)
        return bkgSubtractedMonWS

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        """Return the raw input workspace."""
        inputFile = self.getProperty(common.PROP_INPUT_FILE).value
        if inputFile:
            mainWS = _loadFiles(inputFile, wsNames, wsCleanup, subalgLogging)
        else:
            mainWS = self.getProperty(common.PROP_INPUT_WS).value
            wsCleanup.protect(mainWS)
        return mainWS

    def _monitorIndex(self, monWS):
        if self.getProperty(common.PROP_MON_INDEX).isDefault:
            NON_RECURSIVE = False  # Prevent recursive calls in the following.
            if not monWS.getInstrument().hasParameter('default-incident-monitor-spectrum', NON_RECURSIVE):
                raise RuntimeError('default-incident-monitor-spectrum missing in instrument parameters; ' +
                                   common.PROP_MON_INDEX + ' must be specified.')
            monIndex = monWS.getInstrument().getIntParameter('default-incident-monitor-spectrum', NON_RECURSIVE)[0]
            monIndex = common.convertToWorkspaceIndex(monIndex, monWS, common.INDEX_TYPE_SPECTRUM_NUMBER)
        else:
            monIndex = self.getProperty(common.PROP_MON_INDEX).value
            monIndex = common.convertToWorkspaceIndex(monIndex, monWS)
        return monIndex

    def _normalize(self, mainWS, monWS, monEPPWS, wsNames, wsCleanup, subalgLogging):
        """Normalize to monitor or measurement time."""
        normalisationMethod = self.getProperty(common.PROP_NORMALISATION).value
        if normalisationMethod != common.NORM_METHOD_OFF:
            if normalisationMethod == common.NORM_METHOD_MON:
                sigmaMultiplier = \
                    self.getProperty(common.PROP_MON_PEAK_SIGMA_MULTIPLIER).value
                monIndex = self._monitorIndex(monWS)
                eppRow = monEPPWS.row(monIndex)
                if eppRow['FitStatus'] != 'success':
                    self.log().warning('Fitting to monitor data failed. Integrating the intensity over ' +
                                       'the entire TOF range for normalisation.')
                    begin = monWS.dataX(monIndex)[0]
                    end = monWS.dataX(monIndex)[-1]
                else:
                    sigma = eppRow['Sigma']
                    centre = eppRow['PeakCentre']
                    begin = centre - sigmaMultiplier * sigma
                    end = centre + sigmaMultiplier * sigma
                normalizedWS = _normalizeToMonitor(mainWS, monWS, monIndex, begin, end, wsNames, wsCleanup,
                                                   subalgLogging)
            elif normalisationMethod == common.NORM_METHOD_TIME:
                normalizedWS = _normalizeToTime(mainWS, wsNames, wsCleanup, subalgLogging)
            else:
                raise RuntimeError('Unknonwn normalisation method ' + normalisationMethod)
            wsCleanup.cleanup(mainWS)
            return normalizedWS
        return mainWS

    def _separateMons(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Extract monitors to a separate workspace."""
        detWSName = wsNames.withSuffix('extracted_detectors')
        monWSName = wsNames.withSuffix('extracted_monitors')
        detWS, monWS = ExtractMonitors(InputWorkspace=mainWS,
                                       DetectorWorkspace=detWSName,
                                       MonitorWorkspace=monWSName,
                                       EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return detWS, monWS


AlgorithmFactory.subscribe(DirectILLCollectData)
